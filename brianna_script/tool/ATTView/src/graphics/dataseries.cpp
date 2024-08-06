#include "dataseries.h"
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>
#include <cmath>
#include <chrono>
#include <sstream>
#include <fstream>
#include <map>
#include <unordered_map>

#define MIPMAP_THRESHOLD 16
#include <QtCharts/QLineSeries>

class LineSeries : public QLineSeries
{
    set_tracked();
public:
    LineSeries() {};
};

void DataSeries::replace(std::vector<DataPoint>&& delta_data) {
    int cur_mip = 0;
    std::vector<DataPoint> data;
    data.reserve(delta_data.size()+2);

    int64_t last_time = delta_data[0].first;
    for (const auto& p : delta_data) {
        while (p.first - last_time > target_delta*1.001f) {
            last_time += target_delta;
            data.push_back({last_time, 0});
        }
        data.push_back(p);
        last_time = p.first;
    }
    this->data = delta_data;

    size_t data_size = data.size();
    CreateMipMap(0, data, data_size);

    // Create mipmaps
    while (data_size > MIPMAP_THRESHOLD) {
        cur_mip++;
        CreateMipMap(cur_mip, data, data_size);

        data_size /= 2;
        for (size_t i=0; i<data_size; i++) {
            size_t m = 2*i;
            data[i].first = 0.5f*(data[m+0].first + data[m+1].first);
            data[i].second = 0.5f*(data[m+0].second + data[m+0].second);
        }
    }
    // Delete excess mipmaps
    while (mipmaps.size() > cur_mip+1) {
        delete mipmaps.back();
        mipmaps.pop_back();
    }
}

void DataSeries::setColor(const QColor& color) {
    this->color = color;
    for (QLineSeries* series : mipmaps) {
        QPen pen = series->pen();
        pen.setBrush(QBrush(color));
        pen.setWidth(2);
        series->setPen(pen);
    }
}

void DataSeries::setName(const std::string& name) {
    this->name = name;
    for (QLineSeries* series : mipmaps)
        series->setName(name.c_str());
}

void DataSeries::CreateMipMap(int mip, std::vector<DataPoint>& data, size_t size) {
    while (mipmaps.size() <= mip) {
        LineSeries* series = new LineSeries();
        mipmaps.push_back(series);
        series->setVisible(false);
        series->setUseOpenGL(true);
    }

    QList<QPointF> qdata;
    const float alpha = 0.6f;
    const float norm = 1.0f/(1+alpha+alpha*alpha);
    if (mip >= 1) {
        data[size-2].second = (alpha*data[size-1].second+data[size-2].second)/(1+alpha);
        data[size-3].second = (alpha*data[size-2].second+data[size-3].second)/(1+alpha);
        float rolling = data[size-3].second*(1+alpha);
        for (int i=size-4; i>=0; i--) {
            rolling = data[i].second + alpha*rolling-alpha*alpha*alpha*data[i+3].second;
            data[i].second = rolling*norm;
        }

        data[1].second = (alpha*data[0].second+data[1].second)/(1+alpha);
        data[2].second = (alpha*data[1].second+data[2].second)/(1+alpha);
        rolling = data[2].second*(1+alpha);

        qdata.append({data[0].first-target_delta, 0});
        for (int i=3; i<size; i++) {
            rolling = data[i].second + alpha*rolling-alpha*alpha*alpha*data[i-3].second;
            data[i].second = rolling*norm;
            qdata.append({data[i].first, data[i].second});
        }
        qdata.append({data[size-1].first+target_delta, 0});
    } else {
        qdata.append({data[0].first, 0});
        for (size_t i=0; i<size-1; i++) {
            qdata.append({data[i].first, data[i].second});
            qdata.append({0.96f*data[i+1].first + 0.04f*data[i].first, data[i].second});
        }
        qdata.append({data.back().first, data.back().second});
    }
    mipmaps[mip]->replace(std::move(qdata));
}

int BinarySearch(float x, const DataPoint* points, int size) {
    if (size == 1)
        return 0;

    if (points[size/2].first >= x) return BinarySearch(x, points, size/2);
    else return size/2 + BinarySearch(x, points+size/2, size-size/2);
}

int DataSeries::indexFromPos(float x) {
    const DataPoint* points = data.data();
    int size = (int)data.size();
    if (size <= 2) return 0; // Not supported!

    // Bounds checking
    if (points[0].first >= x) return -1;
    if (points[size-1].first <= x) return -1;

    // Guess index on linear spacing
    float range = points[size-1].first - points[0].first;
    int index = 0.5f + (size-1)*(x-points[0].first)/range;
    index = std::min(std::max(index, 1), size-2);

    // If guess fail, binary search
    if (points[index+1].first < x || points[index-1].first > x)
        index = BinarySearch(x, points, size);
    index = std::min(std::max(index, 1), size-2);

    // Fix boundaries
    if (index < size-1 && points[index].first < x)
        index ++;
    if (index && points[index].first >= x)
        index --;

    return index;
}

float DataSeries::interpFromPos(float x) {
    const DataPoint* points = data.data();
    if (data.size() <= 2) return 0; // Not supported!

    int index = indexFromPos(x);
    if (index < 0) return 0;

    return points[index].second;
}


float DataSeries::integralFromPos(float a, float b) { // TODO: Optimize from last range
    const DataPoint* points = data.data();
    int size = (int)data.size();
    if (size <= 2) return 0; // Not supported!

    int index = indexFromPos(a);
    if (index < 0) index = 0;

    float acc = 0;
    while (index < size && points[index].first <= b) {
        acc += points[index].second;
        index ++;
    }
    return acc;
}

void DataSeries::setVisible(bool bVisible) {
    QWARNING(visible_mip < mipmaps.size(), "Invalid mipmap: " << visible_mip, return);
    if (visible_mip >= 0)
        mipmaps[visible_mip]->setVisible(bVisible);
    this->bIsVisible = bVisible;
}

void DataSeries::UpdateMip(
    float range,
    int width,
    Chart* chart,
    QValueAxis* xaxis,
    QValueAxis* yaxis,
    bool bAuto
) {
    if (data.size()<8)
        return;

    int mip = 0;
    if (bAuto) {
        float dot_per_cycle = data.size() / (data.back().first - data[0].first);
        float min_pixels_per_dot = 1.25f;
        float pixel_per_cycle = width / range;

        float pixels_per_dot = pixel_per_cycle / dot_per_cycle;
        while (mip+1 < mipmaps.size() && pixels_per_dot < min_pixels_per_dot) {
            mip ++;
            pixels_per_dot *= 2;
        }
    }

    if (visible_mip != mip) {
        QLineSeries* series = mipmaps[mip];
        series->setVisible(bIsVisible);

        series->setName(this->name.c_str());
        QPen pen = series->pen();
        pen.setBrush(QBrush(this->color));
        pen.setWidth(2);
        series->setPen(pen);
        chart->addSeries(series);
        series->attachAxis(xaxis);
        series->attachAxis(yaxis);

        if (visible_mip >= 0 && visible_mip < mipmaps.size()) {
            mipmaps[visible_mip]->setVisible(false);
            chart->removeSeries(mipmaps[visible_mip]);
        }
        visible_mip = mip;
        //std::cout << "Mip: " << visible_mip << std::endl;
    }
}
