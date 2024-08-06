#pragma once

#include "include/custom_layouts.h"
#include <vector>
#include <QtCharts/QtCharts>

typedef std::pair<float,float> DataPoint;

class Chart : public QChart {
    set_tracked();
    using Super = QChart;
public:
    Chart() : QChart() {};
};

//! This class manages "mipmaps" for line series
class DataSeries {
    set_tracked();
public:
    DataSeries(int target_delta): target_delta(target_delta) {}
    void replace(std::vector<DataPoint>&& data);
    void setColor(const QColor& color);
    void setName(const std::string& name);
    int indexFromPos(float x);
    float interpFromPos(float x);
    float integralFromPos(float a, float b); // TODO: Optimize from last range
    void setVisible(bool bVisible);
    bool isVisible() { return bIsVisible; };
    void UpdateMip(float range, int width, Chart* c, QValueAxis* x, QValueAxis* y, bool bAuto);

    virtual ~DataSeries() { for (auto* mip : mipmaps) delete mip; }

protected:
    int target_delta;
private:
    void CreateMipMap(int mip, std::vector<DataPoint>& list, size_t size);

    QPen pen;
    QColor color;
    std::string name;
    std::vector<class QLineSeries*> mipmaps;
    bool bIsVisible = true;
    int visible_mip = -1;
    std::vector<DataPoint> data;
};
