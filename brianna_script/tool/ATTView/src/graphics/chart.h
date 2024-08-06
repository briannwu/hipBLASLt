#pragma once

#include <vector>
#include <QtCharts/QtCharts>
#include "include/custom_layouts.h"
#include "dataseries.h"

//! Abstract chart class.
class ChartView : public QChartView {
    Q_OBJECT
    set_tracked();
    using Super = QChartView;
public:
    ChartView(): ChartView(nullptr) {};
    ChartView(class QWidget* parent);
    virtual ~ChartView() { for (auto& [_, series] : vseries) if (series) delete series; };
    void AddData(const std::string& name, QColor color, std::vector<DataPoint>&& data, int delta);
    void ResetAxes();
    void UpdateData(const std::string& name, std::vector<DataPoint>&& data);
    QPointF MapToChart(float posx, float posy = 0);

    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void paintEvent(class QPaintEvent* event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void SetBarPos(float x);
    virtual void resizeEvent(QResizeEvent *event) { this->Super::resizeEvent(event); UpdateMip(); }
    virtual void UpdateGraphTable(float mousepos) = 0;

    void UpdateMip();
    void setAutoLod(bool bAutoLod) {
        if(bAutoLod == this->bAutoLod) return;
        this->bAutoLod = bAutoLod;
        UpdateMip();
    };
protected:
    class QScatterSeries* scatter = nullptr;
    std::unordered_map<std::string, class DataSeries*> vseries;
    class Chart* chart = nullptr;

    QPointF clickpos;
    QPointF lastMousePos;

    class QValueAxis* xaxis = nullptr;
    class QValueAxis* yaxis = nullptr;

    float data_xmax = 0;
    float data_ymax = 0;
    float data_xmin = 0;
    float data_ymin = 0;

    bool bMouse1Pressed = false;
    bool bMouse2Pressed = false;
    bool bCtrlPressed = false;
    bool bAutoLod = true;
};
