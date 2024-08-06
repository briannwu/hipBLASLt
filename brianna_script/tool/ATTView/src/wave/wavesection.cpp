#include "wavesection.h"
#include <QScrollArea>
#include "data/wavemanager.h"
#include "code/qcodelist.h"
#include "mainwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>

std::unordered_set<QWaveSection*> QWaveSection::all_sections = {};

const int QWaveSection::Width = 2048;
int QWaveSection::mipmap_level = 0;

QWaveSection::QWaveSection(QWidget *parent, bool _main): QWidget(parent), bIsMain(_main) {
    Reset();
    all_sections.insert(this);
    setMouseTracking(true);
    this->setAttribute(Qt::WA_AlwaysShowToolTips,true);
}
QWaveSection::~QWaveSection() { all_sections.erase(this); }
void QWaveSection::Reset() { bInit = false; }


void QWaveSection::UpdateData() {
    if (bInit)
        return;

    bInit = true;
    binned_tokens.clear();
    binned_states.clear();

    if (mipmap_level == 0) {
        binned_tokens.shrink_to_fit();
        binned_states.shrink_to_fit();
        return;
    }
    
    for (int i=0; i < (int)token_data.size(); ) {
        TokenData data = token_data[i];
        TokenArray vector = data;
        i += 1;

        int64_t end_pos = data.token.clock + (4<<mipmap_level);
        while (i < token_data.size() && token_data[i].token.clock < end_pos) {
            vector += token_data[i];
            i += 1;
        }

        binned_tokens.push_back(vector);
    }
    for (int i=0; i < (int)state_data.size(); ) {
        int cycles = state_data[i].first;
        int state = state_data[i].second;
        i += 1;

        while (i < state_data.size() && cycles < (4<<mipmap_level)) {
            cycles += state_data[i].first;
            i += 1;
        }
        binned_states.push_back({cycles, state});
    }
}

static QColor whiter(const QColor& a) {
    return QColor(a.red()/2+128, a.green()/2+128, a.blue()/2+128);
}

void QWaveSection::paintEvent(QPaintEvent* event) {
    UpdateData();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i=0; i < (mipmap_level ? binned_tokens.size() : token_data.size()); i++) {
        TokenData& td = mipmap_level ? binned_tokens[i] : token_data[i];
        td.DrawToken(painter, token_data[0].token.clock, bIsMain);
    }

    int64_t clock = 0;
    for (auto& state : (mipmap_level ? binned_states : state_data)) {
        state.DrawState(painter, GetTokenSize(clock), bIsMain);
        clock += state.first;
    }
}

template<typename Type> static
const Type& SearchToken(const std::vector<Type>& array, int64_t clock, int posy, QWaveSection& s) {
    int pos = 0;
    // Find rightmost token
    while (pos < array.size()-1 && array[pos+1].token.clock <= clock)
        pos++;

    // Handle case of token inside another token
    while(
        pos > 0 &&
        array[pos].slot &&
        array[pos].token.clock+array[pos].token.cycles < clock
    ) pos--;

    // Handle case of mouse below token
    while(
        pos > 0 &&
        array[pos].slot &&
        posy > s.TOKEN_POSY() + s.TOKEN_HEIGHT() - 3*s.SLOT_OFFSET()*array[pos].slot
    ) pos--;

    return array[pos];
}

static std::string SearchStates(const std::vector<WaveState>& array, int64_t clock) {
    if (array.size() == 0)
        return "";

    int pos = 0;
    int64_t current_clock = 0;
    while (pos+1 < array.size() && current_clock+array[pos].first < clock) {
        current_clock += array[pos].first;
        pos ++;
    }

    return array[pos].GetName();
}

void QWaveSection::mouseMoveEvent(QMouseEvent* event) {
    this->Super::mouseMoveEvent(event);
    if (!token_data.size() || (mipmap_level && !binned_tokens.size()))
        return;

    // FPS limiter
    if (ENABLE_FPSLIMITER()) {
        static auto last_time = std::chrono::system_clock::now();
        auto new_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(new_time - last_time);

        if (duration.count() < FPS_LIMITER_TIMEOUT())
            return;
        last_time = new_time;
    }

    const int64_t clock = PosToClock(event->pos().x());
    const int64_t global_clock = clock + token_data[0].token.clock;
    int posy = event->pos().y();

    std::string tooltip;
    if (posy < WSTATE_POSY()) {
        if (mipmap_level == 0)
            tooltip = SearchToken(token_data, global_clock, posy, *this).ToolTip();
        else
            tooltip = SearchToken(binned_tokens, global_clock, posy, *this).ToolTip();
    } else {
        if (mipmap_level == 0)
            tooltip = SearchStates(state_data, clock);
        else
            tooltip = SearchStates(binned_states, clock);
        tooltip += " clk: " + std::to_string(global_clock);
    }
    QToolTip::showText(event->globalPos(), tooltip.c_str());
}

void QWaveSection::mousePressEvent(QMouseEvent* event) {
    this->Super::mousePressEvent(event);
    if (!token_data.size() || (mipmap_level && !binned_tokens.size()))
        return;

    const int64_t clock = PosToClock(event->pos().x());
    const int64_t global_clock = clock + token_data[0].token.clock;
    int posy = event->pos().y();

    MainWindow::window->setChartBarPos(global_clock);

    if (posy < WSTATE_POSY() && posy >= TOKEN_POSY()-2*SLOT_OFFSET()) {
        int code_line;
        if (mipmap_level == 0)
            code_line = SearchToken(token_data, global_clock, posy, *this).token.code_line;
        else
            code_line = SearchToken(binned_tokens, global_clock, posy, *this).token.code_line;
            
        if (code_line > 0)
            QCodeline::HighlightLine(code_line+1, true);
    }
}

int QWaveSection::VIEW_MIN = 0;
int QWaveSection::VIEW_MAX = 1E6;

void QWaveSection::SetViewRange(int vmin, int vmax, bool bForce) {
    if (vmax <= vmin || !all_sections.size())
        return;
    if ((VIEW_MAX == vmax && VIEW_MIN == vmin) && !bForce) // Nothing to do here
        return;

    std::unordered_map<QWaveSection*, bool> state_transitions{};
    int visibleCount = 0;

    for (QWaveSection* section : all_sections)
    if (section && section->token_data.size()) {
        bool bNeedHide =    section->token_data[0].token.clock/1000 > vmax ||
                            section->token_data.back().token.clock/1000 < vmin;
        bool bNeedToBeVisible = !bNeedHide;
        if (bNeedToBeVisible ^ section->bIsVisible)
            state_transitions[section] = bNeedToBeVisible;
        visibleCount += bNeedToBeVisible;
    }

    if (visibleCount == 0) // Make sure we dont have empty waveviews
        return;

    for (auto& [section, bVis] : state_transitions) {
        section->bIsVisible = bVis;
        section->setVisible(bVis);
    }
    for (auto& [section, bVis] : state_transitions)
        section->repaint();

    VIEW_MIN = vmin;
    VIEW_MAX = vmax;
}
