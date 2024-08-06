#include "token.h"
#include "code/qcodelist.h"
#include "mainwindow.h"
#include <QScrollArea>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <sstream>
#include "wavesection.h"
#include "data/wavemanager.h"

#define TOKEN_POSY() (2+5*bIsMain)
#define TOKEN_HEIGHT() (8*(2+bIsMain))
#define SLOT_OFFSET() (1+bIsMain)
#define WSTATE_POSY() (TOKEN_POSY()+TOKEN_HEIGHT()+(1+bIsMain)*SLOT_OFFSET())
#define WSTATE_HEIGHT() (4+bIsMain)


std::vector<std::string> WaveState::STATE_NAMES = {"EMPTY", "IDLE", "EXEC", "WAIT", "STALL"};
std::vector<QColor> WaveState::STATE_COLORS = {
    QColor(254,254,254),
    QColor(127,127,127),
    QColor(0,254,0),
    QColor(254,254,0),
    QColor(254,0,0)
};

std::vector<std::string> TokenData::INST_TYPE = {
    "NONE",
	"SMEM",
	"SALU",
	"VMEM",
    "FLAT",
    "LDS",
    "VALU",
    "JUMP",
    "NEXT",
    "IMMED"
};

std::vector<std::string> TokenData::STYLE_COLORS = {
    "#ffffff",
    "#ebff00",
    "#99ff99",
    "#feb72a",
    "#80d0e0",
    "#ff831d",
    "#009900",
    "#6030ff",
    "#8020ff",
    "#808080"
};

std::vector<QColor> TokenData::Q_COLORS = {
    QColor(128, 128, 128),
    QColor(255, 255, 0),
    QColor(160, 255, 160),
    QColor(255, 180, 40),
    QColor(128, 224, 255),
    QColor(255, 130, 0),
    QColor(0, 160, 0),
    QColor(96, 32, 255),
    QColor(128, 32, 255),
    QColor(64, 64, 64)
};

static float tonemap(float x) { x /= 255.0f; return 255.0f*cbrt(x*x); }

static QColor whiter(const QColor& a) {
    return QColor(a.red()/2+128, a.green()/2+128, a.blue()/2+128);
}

void TokenData::DrawToken(QPainter& painter, int64_t begintime, bool bIsMain) {
    const QColor& c = GetColor();
    QColor color(tonemap(c.red()), tonemap(c.green()), tonemap(c.blue()));
    int pos = QWaveSection::GetTokenSize(token.clock-begintime);
    int width = QWaveSection::GetTokenSize(token.cycles);

    QPainterPath path;
    QRectF rect(pos, TOKEN_POSY()-2*SLOT_OFFSET()*slot, width, TOKEN_HEIGHT()-SLOT_OFFSET()*slot);
    int corner = 2 + bIsMain + (token.cycles>WaveManager::BaseClock());
    path.addRoundedRect(rect, corner, corner);

    QLinearGradient grad(0, TOKEN_POSY()-2*SLOT_OFFSET()*slot, 0, TOKEN_HEIGHT()-SLOT_OFFSET()*slot);
    grad.setColorAt(0.6, color);
    grad.setColorAt(0, whiter(color));
    QBrush brush(grad);
    painter.fillPath(path, brush);

    painter.setPen(QPen(Qt::black, 0.9));
    painter.drawPath(path);
}

std::string TokenData::ToolTip() const {
    return GetName() + ", cycles: "
    + std::to_string(tip.cycles) + ", clk: " + std::to_string(tip.clock);
};

std::string TokenArray::ToolTip() const {
    std::stringstream str;
    str << "Clock: " << token.clock << "";
    for (int i=0; i<cycles.size(); i++) {
        if (cycles[i])
            str << '\n' << GetName(i) << ": " << cycles[i];
    }
    return str.str();
};

TokenArray& TokenArray::operator+=(const TokenData& td) {
    cycles[td.token.type] += td.token.cycles;
    if (cycles[td.token.type] > max_cycles) {
        max_cycles = cycles[td.token.type];
        this->token.type = td.token.type;
    }
    if (td.slot == 0)
        this->token.cycles += td.token.cycles;
    return *this;
}

TokenArray::TokenArray(const TokenData& td): TokenData(td) {
    this->token.cycles = 0;
    this->slot = 0;
    *this += td;
};

void WaveState::DrawState(QPainter& painter, int64_t pos, bool bIsMain) {
    int width = QWaveSection::GetTokenSize(this->first);
    QColor& color = STATE_COLORS[this->second % STATE_COLORS.size()];
    QPainterPath path;
    path.addRoundedRect(QRectF(pos, WSTATE_POSY(), width, WSTATE_HEIGHT()), 1+bIsMain, 1+bIsMain);

    QLinearGradient grad(0, WSTATE_POSY(), 0, WSTATE_POSY()+WSTATE_HEIGHT());
    grad.setColorAt(0.5, color);
    grad.setColorAt(0, whiter(color));
    QBrush brush(grad);
    painter.fillPath(path, brush);

    painter.setPen(QPen(Qt::black, 0.9));
    painter.drawPath(path);
}

/*
void QToken::Highlight(bool bIntoView) {
    timer.Highlight();
    QObject::connect(timer.timer, &QTimer::timeout, this, &QToken::IncrementHighlight);
    if (bIntoView && MainWindow::window->waveview_scrollarea)
        MainWindow::window->waveview_scrollarea->ensureWidgetVisible(this);
}

void QToken::IncrementHighlight() {
    timer.IncrementHighlight();
    repaint();
} */
