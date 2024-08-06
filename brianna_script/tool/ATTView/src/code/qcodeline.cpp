#include "qcodeline.h"
#include <QTimer>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include "mainwindow.h"
#include "wave/waveview.h"
#include "graphics/canvas.h"
#include <sstream>

#define ASMLINE_MAXCHARS 50
#define SHORT_CPPLINE_MAXCHARS 32

static float sq(float x) { return x*x; }

std::unordered_map<int, QCodeline*> QCodeline::line_map;
QCodeline* QCodeline::firstline = nullptr;

QCodeline::QCodeline(
    Super* parent,
    int wave_id,
    int index,
    const std::string& line,
    int line_num,
    const std::vector<int>& cycles,
    const std::string& cppline,
    int64_t cycles_sum,
    int hitcount
): QWidget(parent), line_map_index(index), wave_id(wave_id) {
    this->aslot = new ArrowSlot(this);
    this->line = new QASMLine(line, line_num, index, this);
    //this->hitcount = new QHitcountLabel(cycles.size() ? std::to_string(cycles.size()).c_str() : "");
    this->cycles = new CyclesLabel(cycles, cycles_sum, hitcount);
    this->cppline = new QCppLabel(cppline, cycles_sum);
    
    line_map[line_map_index] = this;
    setMouseTracking(true);
}

void QCodeline::leaveEvent(QEvent* event) {
    bUnderMouse = false;
    repaint();
}
void QCodeline::enterEvent(QEvent* event) {
    bUnderMouse = true;
    EnableQTLayout();
    repaint();
}

void QCodeline::Highlight(bool bIntoView) {
    line->Highlight(false);
    if (bIntoView && MainWindow::window->code_scrollarea)
        MainWindow::window->code_scrollarea->ensureWidgetVisible(this);
}

QCodeline::~QCodeline() {
    line_map[line_map_index] = nullptr;
    if (hlayout) {
        delete hlayout;
    } else {
        if (line) delete line;
        if (aslot) delete aslot;
        //if (hitcount) delete hitcount;
        if (cycles) delete cycles;
        if (cppline) delete cppline;
    }
}

void QCodeline::paintEvent(QPaintEvent* event) {
    QWARNING(firstline != nullptr, "Firstline not initialized!", return);
    const int posy = 3*LINE_HEIGHT/4+CODE_FONT-11;
    this->Super::paintEvent(event);

    QPainter painter(this);
    QFont font = painter.font();
    font.setPointSize(CODE_FONT);
    painter.setFont(font);

    painter.setRenderHint(QPainter::TextAntialiasing);
    line->paint(painter, posy);
    //hitcount->paint(painter, posy);
    cycles->paint(painter, posy);
    cppline->paint(painter, posy);

    if (!hlayout && cppline->GetSourceRef().IsValid()) {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(1);
        QObject::connect(timer, &QTimer::timeout, this, &QCodeline::EnableQTLayout);
        timer->start();
    }
};

//void QCodeline::mouseMoveEvent(class QMouseEvent* event) { EnableQTLayout(); }

void QCodeline::EnableQTLayout() {
    if (hlayout)
        return;

    line->CreateFuncElement();
    //hitcount->CreateFuncElement();
    cycles->CreateFuncElement();
    cppline->CreateFuncElement();
    CreateLayout();
    repaint();
}

void QCodeline::CreateLayout() {
    hlayout = new QHBox(this);
    hlayout->setMargin(1);
    this->setLayout(hlayout);

    hlayout->addWidget(this->aslot, 0);
    hlayout->addWidget(this->line, 1);
    //hlayout->addWidget(this->hitcount, 2);
    hlayout->addWidget(this->cycles, 3);
    hlayout->addWidget(this->cppline, 4);
}

void CustomTextWidget::CreateFuncElement() {
    if (this->functional_element != nullptr)
        return;

    QLabel* label = new QLabel(str.c_str(), this);
    this->functional_element = label;
    label->setTextFormat(Qt::PlainText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    QFont font = this->functional_element->font();
    font.setPointSize(CODE_FONT);
    this->functional_element->setFont(font);
}

void QASMLine::Highlight(bool bFast) {
    this->codeline_parent->EnableQTLayout();
    if (bFast) {
        highlightSpeed = 0.0f;
        highlightColor = Color(255,255,255);
    } else {
        highlightSpeed = 0.015f;
        highlightColor = Color(16,255,16);
    }
    timer.Highlight();
    QObject::connect(timer.timer, &QTimer::timeout, this, &QASMLine::IncrementHighlight);
}

void QASMLine::IncrementHighlight() {
    this->timer.IncrementHighlight(highlightSpeed);
    this->repaint();
}

void QASMLine::mousePressEvent(QMouseEvent* e) {
    this->Super::mousePressEvent(e);
    Highlight(true);
    QWaveView::HighlightTokenFromLine(code_line);
};

void QASMLine::mouseReleaseEvent(QMouseEvent* e) {
    this->Super::mouseReleaseEvent(e);
    highlightSpeed = 1.0f;
};

QASMLine::QASMLine(const std::string& line, int asm_line, int code_line, QCodeline* parent):
    code_line(code_line), codeline_parent(parent) {

    auto comment_pos = std::min(line.find(';'), line.find("//")); // Remove comments from ASM line
    if (line.size() <= ASMLINE_MAXCHARS || comment_pos < ASMLINE_MAXCHARS) {
        if (comment_pos != std::string::npos && comment_pos > 0)
            this->setText(line.substr(0, comment_pos-1).c_str());
        else
            this->setText(line.c_str());
    } else {
        this->setText((line.substr(0, ASMLINE_MAXCHARS-3)+"...").c_str());
    }
    std::string tip = line;
    if (comment_pos != std::string::npos)
        tip[comment_pos] = '\n';
    if (asm_line > 0)
        tip = '('+std::to_string(code_line)+'/'+std::to_string(asm_line)+") " + tip;
    this->setToolTip(tip.c_str());
};

void QCppLabel::CreateFuncElement() {
    if (functional_element)
        return;

    if (sourceref.IsValid()) {
        QPushButton* button = new QPushButton(str.c_str(), this);
        button->setFixedHeight(LINE_HEIGHT-2);
        QObject::connect(button, &QPushButton::clicked, this, &QCppLabel::customMousePress);
        this->functional_element = button;
        QFont font = this->functional_element->font();
        font.setPointSize(CODE_FONT);
        this->functional_element->setFont(font);
    } else {
        this->Super::CreateFuncElement();
    }
    this->functional_element->setToolTip(tooltip.c_str());
}

QCppLabel::QCppLabel(const std::string& cppline, int cycles):
    sourceref(cppline, cycles) {
    bool bHashSlash = true;
    std::string short_cppline = cppline;
    while (bHashSlash && short_cppline.size() > SHORT_CPPLINE_MAXCHARS) {
        auto pos = short_cppline.find('/');
        if (pos != std::string::npos)
            short_cppline = short_cppline.substr(pos+1);
        else
            bHashSlash = false;
    }

    if (short_cppline.size() > SHORT_CPPLINE_MAXCHARS)
        short_cppline = "..."+short_cppline.substr(short_cppline.size()-SHORT_CPPLINE_MAXCHARS);

    tooltip = cppline;
    if (sourceref.IsValid())
        tooltip = sourceref.Get() + '\n'+ tooltip;

    this->setText(short_cppline.c_str());

    // TODO: Don't zero out cycles when attrib goes to system header
    //if (sourceref.IsValid() && !sourceref.IsHeader())
};

void QCppLabel::customMousePress(bool b) {
    if (sourceref.IsValid())
        sourceref.LoadRefToSourceTab();
}

void QCppLabel::ResetCppFile() {
    SourceLoader::current_loaded_source = "";
}

void CyclesLabel::setStrategy(const QString& str) {
    static std::unordered_map<std::string, Strategy> strategy_map = {
        {"Sum all", Strategy::SUM_ALL},
        {"Mean all", Strategy::MEAN_ALL},
        {"Sum", Strategy::SUM},
        {"Mean", Strategy::MEAN},
        {"Max", Strategy::MAX},
        {"First", Strategy::FIRST},
        {"Last", Strategy::LAST},
    };

    Strategy st = Strategy::SUM;
    try {
        st = strategy_map[str.toStdString()];
    } catch (std::exception& e) {
        QWARNING(false, "Invalid strategy: " << e.what(), return);
    }

    this->setStrategy(st);
}

void CyclesLabel::setStrategy(Strategy strategy) {
    if (cycles.size() == 0 && (strategy > Strategy::MEAN_ALL || all_hitcount == 0))
        return;

    int64_t value = 0;
    int64_t hit = cycles.size();

    switch (strategy) {
        case Strategy::SUM:
        case Strategy::MEAN:
            for (int v : cycles) value += v;
            break;
        case Strategy::MAX:
            for (int64_t v : cycles) value = std::max(value, v);
            break;
        case Strategy::FIRST:
            value = cycles[0];
            break;
        case Strategy::LAST:
            value = cycles.back();
            break;
        case Strategy::SUM_ALL:
        case Strategy::MEAN_ALL:
            value = all_cycles_sum;
            hit = all_hitcount;
            break;
        default:
            QWARNING(false, "Invalid strategy", return);
    }
    if (strategy == Strategy::MEAN || strategy == Strategy::MEAN_ALL)
        value = (value+hit/2)/std::max(hit, 1l);

    std::stringstream newtext;
    newtext << ' ' << hit;
    newtext << std::string(std::max(2, 25-5*(int)newtext.str().size()/2), ' ');
    newtext << value;
    this->setText(newtext.str().c_str());
    if (QLabel* label = dynamic_cast<QLabel*>(functional_element))
        label->setText(newtext.str().c_str());
}

void QHitcountLabel::paint(class QPainter& painter, int posy) {
    //if (functional_element==nullptr)
    //    painter.drawText(QPointF(QCodeline::firstline->hitcount->pos().x(), posy), str.c_str());
}

void CyclesLabel::paint(class QPainter& painter, int posy) {
    if (functional_element==nullptr)
        painter.drawText(QPointF(QCodeline::firstline->cycles->pos().x(), posy), str.c_str());
}

void QCppLabel::paint(class QPainter& painter, int posy) {
    if (functional_element)
        return;

    int posx = QCodeline::firstline->cppline->pos().x();
    painter.drawText(QPointF(posx, posy), str.c_str());
}

void QASMLine::paint(class QPainter& painter, int posy) {
    int posx = QCodeline::firstline->line->pos().x();
    int sizex = QCodeline::firstline->line->width();

    if (timer.vis < 1) {
        float multvis = sq(sq(sq(std::max(timer.vis, 0.0f))));

        QPainterPath path;
        path.addRoundedRect(QRectF(posx, 1, sizex-1, LINE_HEIGHT-2), 4, 4);
        painter.setPen(Color(0,0,0));

        painter.fillPath(path, highlightColor*(1-multvis) + Color(215,215,215)*multvis);
        //painter.drawPath(path);
    }
    else if (functional_element==nullptr)
        painter.drawText(QPointF(posx, posy), str.c_str());
    else if (codeline_parent->bUnderMouse)
        painter.fillRect(posx, 1, functional_element->width()-2, LINE_HEIGHT-2, QColor(225, 225, 225));
}