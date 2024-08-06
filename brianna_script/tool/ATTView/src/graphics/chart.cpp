#include "chart.h"
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>
#include <cmath>
#include <chrono>
#include <sstream>
#include <map>
#include <unordered_map>
#include <cmath>
#include <QScatterSeries>

#define LINE_TOL() 1E-4f
#define GRAPH_BKG_COLOR() QColor(105, 105, 105)


QPointF ChartView::MapToChart(float posx, float posy) { 
    return chart->mapToValue(chart->mapFromScene(mapToScene(QPoint(posx, posy))));
}

void ChartView::SetBarPos(float pos) {
    QList<QPointF> slist;
    for (auto& [name, series] : vseries) {
        if (!series || !series->isVisible()) continue;
        float value = series->interpFromPos(pos);
        slist.append({pos, value});
    }
    scatter->replace(std::move(slist));
}

void ChartView::paintEvent(class QPaintEvent* event) {
    this->Super::paintEvent(event);

    QPainter painter(viewport());
    QPen pen(QBrush(QColor(255,255,255), Qt::HorPattern), Qt::DashLine);
    painter.setPen(pen);

    float mapx = lastMousePos.x();
    painter.drawLine(mapx, chart->pos().y()+10, mapx, height()+chart->pos().y());
}

void ChartView::mouseMoveEvent(QMouseEvent* event) {
    static auto last_time = std::chrono::system_clock::now();
    auto new_time = std::chrono::system_clock::now();
    lastMousePos = event->pos();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(new_time - last_time);
    if (duration.count() > FPS_LIMITER_TIMEOUT()) {
        last_time = new_time;

        QPointF cpos = MapToChart(event->pos().x(), event->pos().y());

        if (cpos.x() >= xaxis->min() && cpos.x() <= xaxis->max() &&
            cpos.y() >= yaxis->min() && cpos.y() <= yaxis->max()) {
            UpdateGraphTable(cpos.x());
            viewport()->repaint();
        }
    }

    if (bMouse2Pressed) {
        chart->scroll(clickpos.x()-event->pos().x(), 0);
        clickpos = event->pos();
        return;
    }

    this->Super::mouseMoveEvent(event);

    if (bMouse1Pressed) {
        QRubberBand* rubberBand = this->findChild<QRubberBand*>();
        QWARNING(rubberBand, "Invalid rubberband", return);
        rubberBand->setGeometry(clickpos.x(), height()/16, event->pos().x()-clickpos.x(), 7*height()/8);
    }
}

void ChartView::mousePressEvent(QMouseEvent* event) {
    clickpos = event->pos();

    if (event->button()==Qt::RightButton) {
        Qt::KeyboardModifiers modif = QApplication::queryKeyboardModifiers();
        if (modif == Qt::ControlModifier) {
            bCtrlPressed = true;
            this->Super::mousePressEvent(event);
        } else {
            bMouse2Pressed = true;
            chart->setAnimationOptions(QChart::NoAnimation);
        }
    }

    if (event->button()==Qt::LeftButton) {
        bMouse1Pressed = true;
        this->Super::mousePressEvent(event);
    }
    UpdateMip();
}

void ChartView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && bMouse1Pressed) {
        bMouse1Pressed = false;
    } else if (event->button() == Qt::RightButton && bMouse2Pressed) {
        bMouse2Pressed = false;
        chart->setAnimationOptions(QChart::SeriesAnimations);
    }

    if (event->button() != Qt::RightButton || bCtrlPressed) {
        this->Super::mouseReleaseEvent(event);
    }
    bCtrlPressed = false;
    UpdateMip();
}

void ChartView::ResetAxes() {
    xaxis->setRange(data_xmin, data_xmax);
    yaxis->setRange(data_ymin, data_ymax+(data_ymax-data_ymin)*2E-2f);
}

void ChartView::UpdateData(const std::string& name, std::vector<DataPoint>&& data) {
    if (!data.size()) return;

    DataSeries* series = vseries[name];
    QWARNING(series, "Invalid series name", return);

    for (int i = 1; i<data.size(); i++)
        QWARNING(data[i-1].first <= data[i].first, "Data not ordered!", return);

    float ymin = data_ymin;
    float ymax = data_ymax;
    for (auto& point : data) {
        ymin = std::min(ymin, point.second);
        ymax = std::max(ymax, point.second);
    }
    float xmin = std::min(data_xmin, data[0].first);
    float xmax = std::max(data_xmax, data.back().first);

    float xtol = std::abs(xmin-data_xmin+xmax-data_xmax);
    float ytol = std::abs(ymin-data_ymin+ymax-data_ymax);
    float xmod = std::abs(xmin-data_xmin)+std::abs(xmax-data_xmax);
    float ymod = std::abs(ymin-data_ymin)+std::abs(ymax-data_ymax);

    data_xmin = xmin;
    data_ymin = ymin;
    data_xmax = xmax;
    data_ymax = ymax;

    series->replace(std::move(data));
    if (xmod > LINE_TOL()*xtol || ymod > LINE_TOL()*ytol);
        ResetAxes();

    //series->UpdateMip();
}

void ChartView::UpdateMip() {
    for(auto& [name, series] : vseries) if (series)
        series->UpdateMip(xaxis->max()-xaxis->min(), width(), chart, xaxis, yaxis, bAutoLod);
}

void ChartView::wheelEvent(QWheelEvent *event) {
    chart->setAnimationOptions(QChart::NoAnimation);

    const float scale = 800.0f;
    const float fraction = std::exp(-event->delta() / scale);

    float max = xaxis->max();
    float min = xaxis->min();

    float mid = MapToChart(lastMousePos.x(), lastMousePos.y()).x();
    float min_space = fraction*(mid - min);
    float max_space = fraction*(max - mid);

    xaxis->setRange(mid-min_space, mid+max_space);
    UpdateMip();
}

void ChartView::AddData(
    const std::string& name,
    QColor color,
    std::vector<DataPoint>&& data,
    int target_delta
) {
    DataSeries* series = new DataSeries(target_delta);
    series->setName("<font color=#000000>"+name+"</font>");
    series->setColor(color);

    if (vseries[name])
        delete vseries[name];
    vseries[name] = series;
    UpdateData(name, std::move(data));
}

ChartView::ChartView(class QWidget* parent): Super(parent) {
    chart = new Chart();
    chart->setAnimationDuration(100);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    //chart->setAnimationOptions(QChart::NoAnimation);
    this->setRubberBand(QChartView::HorizontalRubberBand);
    this->setChart(chart);

    setRenderHint(QPainter::Antialiasing);
    chart->setBackgroundBrush(QBrush(GRAPH_BKG_COLOR()));
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setMargins(QMargins(20,0,10,10));

    xaxis = new QValueAxis(this);
    xaxis->setGridLineVisible(false);
    xaxis->setRange(0, 1);
    xaxis->setLabelsColor(QColor(0,0,0));
    chart->addAxis(xaxis, Qt::AlignBottom);

    yaxis = new QValueAxis(this);
    yaxis->setGridLineVisible(false);
    yaxis->setRange(0, 1);
    yaxis->setLabelsColor(QColor(0,0,0));
    chart->addAxis(yaxis, Qt::AlignLeft);

    scatter = new QScatterSeries();
    chart->addSeries(scatter);
    scatter->attachAxis(xaxis);
    scatter->attachAxis(yaxis);
    scatter->setMarkerSize(9);
}
