#pragma once
#include "chart.h"

//! Class for visualizing wave states
class WaveChartView : public ChartView {
public:
    WaveChartView(): WaveChartView(nullptr) {};
    WaveChartView(class QWidget* parent): ChartView(parent) {};
    void LoadWaveStateData(const std::string& filename, int SE);
    virtual void UpdateGraphTable(float mousepos) override;

    static std::vector<std::string> state_names;
private:
    static std::vector<QColor> colors;
};

//! Class for visualizing performance counters
class CounterChartView : public ChartView {
public:
    CounterChartView(): CounterChartView(nullptr) {};
    CounterChartView(class QWidget* parent);
    virtual ~CounterChartView();
    void LoadCounterData(class JsonRequest& file, int SE);
    void UpdateDataSelection(const std::vector<std::string>& counters_names, uint64_t se_mask, uint64_t cu_mask);
    virtual void UpdateGraphTable(float mousepos) override;

private:
    struct RootNode* rootnode = nullptr;
    static std::vector<QColor> colors;
};

//! Class for visualizing GPU occupancy
class OccupancyChartView : public ChartView {
public:
    OccupancyChartView(): OccupancyChartView(nullptr) {};
    OccupancyChartView(class QWidget* parent): ChartView(parent) {};
    //virtual ~OccupancyChartView();
    virtual void LoadOccupancyData(const std::string& filename);
    virtual void UpdateGraphTable(float mousepos) {};
protected:
    static std::vector<QColor> colors;
};

//! Class for visualizing GPU occupancy
class DispatchChartView : public OccupancyChartView {
public:
    DispatchChartView(class QWidget* parent): OccupancyChartView(parent) {};
    virtual void LoadOccupancyData(const std::string& filename);
};
