#include "waveview.h"
#include <QScrollArea>
#include "data/wavemanager.h"
#include "code/qcodelist.h"
#include "mainwindow.h"
#include <QScrollArea>
#include <QScrollBar>

QWaveView* QWaveView::mainview = nullptr;

QWaveView::QWaveView(QWidget *parent, QScrollArea* sarea): QWidget(parent) {
    Reset();
}

QWaveView::~QWaveView() {
    if (layout) delete layout;
    if (bIsMain)
        mainview = nullptr;
}

void QWaveView::Reset() {
    if (bIsMain)
        mainview = this;
    if (layout)
        delete layout;
    sections = std::vector<QWaveSection*>();

    layout = new QHBox(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    line_to_clock = {};
}

void QWaveView::AddToken(Token token, int slot) {
    Token tooltip = token;
    token.cycles = std::max(token.cycles, (int)WaveManager::BaseClock());

    if (bIsMain && token.code_line)
        line_to_clock[token.code_line+1].push_back(token.clock);

    int64_t first_section_id = (token.clock - begintime) / cycles_per_section;
    int64_t last_section_id = (token.clock + token.cycles - begintime) / cycles_per_section;

    while ((int64_t)sections.size() <= last_section_id) {
        sections.push_back(new QWaveSection(this, bIsMain));
        layout->addWidget(sections.back());
    }

    for (int id = first_section_id; id <= last_section_id && token.cycles > 0; id++) {
        Token stoken = token;
        stoken.cycles = std::min(stoken.cycles, int((id+1)*cycles_per_section + begintime - stoken.clock));
        sections[id]->AddToken(stoken, tooltip, slot);
        token.clock += stoken.cycles;
        token.cycles -= stoken.cycles;
    }
}

void QWaveView::AddTimeline(int64_t clock, int cycles, int state) {
    int first_section_id = (clock - begintime) / cycles_per_section;
    int last_section_id = (clock + cycles - begintime) / cycles_per_section;

    while (int64_t(sections.size()) <= last_section_id) {
        sections.push_back(new QWaveSection(this, bIsMain));
        layout->addWidget(sections.back());
    }

    for (int id = first_section_id; id <= last_section_id && cycles > 0; id++) {
        int scycles = cycles;
        scycles = std::min(scycles, int((id+1)*cycles_per_section + begintime - clock));
        sections.at(id)->AddState(scycles, state);
        clock += scycles;
        cycles -= scycles;
    }
}

void QWaveView::Populate(const WaveManager& manager) {
    Reset();
    this->cycles_per_section = QWaveSection::Width;

    int curslot = 0;
    Token blanktoken = {0, 0, 0, 0, -1};

    begintime = manager.WaveBegin();
    endtime = manager.WaveEnd();
    endtime += (endtime-begintime+QWaveSection::Width-1) % QWaveSection::Width;

    int64_t last_token_cycle = begintime;
    int64_t current_token_cycle = last_token_cycle;

    std::vector<Token> current_tokens;

    for(int tdx = 0; tdx < manager.GetTokens().size(); tdx++) {
        Token token = manager.GetTokens()[tdx];

        if (token.clock > endtime) break;

        if (token.cycles < WaveManager::BaseClock() &&
            tdx < manager.GetTokens().size()-1 &&
            manager.GetTokens()[tdx+1].clock < token.clock+WaveManager::BaseClock()
        ) {
            current_tokens.push_back(token);
            continue;
        }

        int64_t width = token.clock - last_token_cycle;
        if (width < 0) {
            if(token.clock + token.cycles <= last_token_cycle) {
                if(curslot < 3) {
                    if (current_token_cycle > token.clock)
                        curslot += 1;
                    else
                        curslot = 1;
                    AddToken(token, curslot);
                    current_token_cycle = token.clock;
                    last_token_cycle = std::max(current_token_cycle, last_token_cycle);
                }
                continue;
            }
            token.clock = last_token_cycle; // TODO: Replace base token by this
        } else if (width > 0) {
            blanktoken.clock = last_token_cycle;
            blanktoken.cycles = token.clock - last_token_cycle;
            AddToken(blanktoken, 0);
        }
        current_token_cycle = token.clock + std::max(token.cycles, (int)WaveManager::BaseClock());
        last_token_cycle = std::max(current_token_cycle, last_token_cycle);

        AddToken(token, 0);
        for (size_t slot=0; slot < current_tokens.size() && slot < 3; slot++)
            AddToken(current_tokens[slot], slot+1);

        curslot = 0;
        current_tokens = {};
    }

    int64_t current_state_time = begintime;
    for (auto t : manager.GetTimeline()) {
        if(t.second == 0) continue;

        if (current_state_time + t.second > endtime) {
            AddTimeline(current_state_time, endtime-current_state_time, t.first);
            current_state_time = endtime;
            break;
        }

        AddTimeline(current_state_time, t.second, t.first);
        current_state_time += t.second;
    }

    int64_t timel_maxtime = endtime - current_state_time;
    int64_t token_maxtime = endtime - last_token_cycle;

    if (timel_maxtime > 0) AddTimeline(current_state_time, (int)timel_maxtime, 0);
    if (token_maxtime > 0) AddToken({last_token_cycle, 0, (int)token_maxtime, 0, -1}, 0);

    layout->addStretch();
}

void QWaveView::paintEvent(class QPaintEvent* event) {
    this->Super::paintEvent(event);
};

void QWaveView::HighlightTokenFromLine(int code_line) {
    QWaveView& view = GetMainView();

    if (code_line <= 0)
        return;

    int pixel = -1;
    int64_t vistime = view.VisibleBeginTime();
    try {
        int64_t vistime = view.VisibleBeginTime();
        const std::vector<int64_t>& clock_array = view.line_to_clock.at(code_line);
        if (!clock_array.size()) return;

        for (int64_t clock : clock_array) if (clock >= vistime) {
            int pixel = QWaveSection::GetTokenSize(clock-vistime);
            MainWindow::window->waveview_scrollarea->horizontalScrollBar()->setValue(pixel);
            return;
        }
    } catch (std::exception& e) {
        return;
    }
}

int64_t QWaveView::VisibleBeginTime() const {
    if (sections.size() && sections[0])
    for (QWaveSection* section : sections)
    if (section && section->bIsVisible) {
        int64_t section_time = section->BeginTime();
        if (section_time >= 0)
            return section_time;
    }
    return begintime;
};

void QWaveSlots::Reset() {
    if (layout) delete layout;
    layout = new QVBox(this);
}

QWaveSlots::QWaveSlots(QWidget* parent) { Reset(); }

void QWaveSlots::AddSlot(QWaveView* view, const std::string& name) {
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QWidget* new_slot = new QWidget();
    QHBox* box = new QHBox();
    box->setContentsMargins(0,0,0,0);
    box->setMargin(0);
    new_slot->setLayout(box);
    box->addWidget(new QLabel(name.c_str()));
    box->addWidget(view);
    layout->addWidget(new_slot);
    waves.push_back(view);
}
