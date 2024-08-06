#include "specialized_charts.h"
#include <fstream>
#include "mainwindow.h"
#include "data/wavedata.h"
#include "container/datanode.h"


std::vector<std::string> WaveChartView::state_names = {"Empty", "Idle", "Exec", "Wait", "Stall"};

std::vector<QColor> WaveChartView::colors = {
    {255, 255, 255},
    {150, 150, 150},
    {32, 255, 32},
    {255, 255, 0},
    {255, 32, 32}
};

// LineSeries colors
std::vector<QColor> CounterChartView::colors = {
    {32, 32, 255},
    {255, 32, 32},
    {32, 255, 32},

    {255, 255, 255},
    {0, 0, 0},

    {255, 255, 32},
	{255, 32, 255},
	{32, 255, 255},

    {128, 255, 0},
    {0, 128, 255},
    {255, 0, 128},

    {255, 128, 0},
    {0, 255, 128},
    {128, 0, 255},

    {160, 255, 255},
    {255, 160, 255},
    {255, 255, 160}
};

std::vector<QColor> OccupancyChartView::colors = {
    {255, 0, 0},
    {0, 255, 0},
    {0, 0, 255},
    {255, 160, 0},
    {255, 0, 160},
    {0, 160, 255},
    {160, 0, 255},
    {0, 255, 160},
    {255, 0, 160}
};

void WaveChartView::LoadWaveStateData(const std::string& filename, int SE) {

    for (int state = 1; state < 5; state++) {
        JsonRequest file(filename + "wstates" + std::to_string(state) + ".json");
        QWARNING(!file.fail() && !file.bad(), "Error opening file " << filename, return);

        std::vector<DataPoint> datapoints;
        int64_t cycle = 0;
        int64_t last_nonzero = 0;
        for (auto& v : file.data["data"]) {
            int point = int(v);
            datapoints.push_back({16*(float)cycle, (float)point});
            cycle ++;
            if (point) last_nonzero = datapoints.size();
        }
        if (last_nonzero+2 < datapoints.size())
            datapoints.erase(datapoints.begin()+last_nonzero+1,datapoints.end());

        while (datapoints.size() < 3)
            datapoints.push_back({datapoints.size(), 0});
        AddData(state_names[state], colors[state%colors.size()], std::move(datapoints), 16);
    }
}

void WaveChartView::UpdateGraphTable(float mousepos) {
    float total = 0;
    float values[4];
    memset(values, 0, sizeof(values));

    for (int i=0; i<4; i++) {
        if (vseries[state_names[i+1]] == nullptr)
            continue;
        values[i] = vseries[state_names[i+1]]->interpFromPos(mousepos);
        total += values[i];
    }
    total = std::max(total, 1.0f) / 100.0f; // Display as percentage
    for (int i=0; i<4; i++)
        MainWindow::window->UpdateGraphInfo(state_names[i+1], values[i], values[i]/total);
}

void CounterChartView::LoadCounterData(JsonRequest& file, int shader_engine) {
    std::vector<CounterData> data;
    data.reserve(file.data["data"].size());

    for (auto& event : file.data["data"])
        data.push_back(CounterData({
            int64_t(event[0]),
            {int(event[1]),
            int(event[2]),
            int(event[3]),
            int(event[4])},
            int8_t(event[5]),
            int8_t(event[6]),
            int8_t(shader_engine)
        }));

    rootnode->Insert(shader_engine, data);
}

/* Fix missing 0-valued perfcounters */
static void FixTime(std::map<int, CounterData>& counters_loaded, int64_t delta_time) {
    std::cout << "delta_time: " << delta_time << std::endl;
    std::vector<CounterData> add_insert_data;
    for (auto& [key, counter] : counters_loaded) {
        if (counter.events[0]+counter.events[1]+counter.events[2]+counter.events[3] == 0)
            continue;

        if (key > delta_time && counters_loaded.find(key-delta_time) == counters_loaded.end())
            add_insert_data.push_back({counter.time-delta_time, 0, 0, 0, 0, counter.cu, counter.bank});
        if (counters_loaded.find(key+delta_time) == counters_loaded.end())
            add_insert_data.push_back({counter.time+delta_time, 0, 0, 0, 0, counter.cu, counter.bank});
    }

    for (auto& d : add_insert_data)
        counters_loaded[d.GetOrderedOperator()] = d;
}

void CounterChartView::UpdateDataSelection(
    const std::vector<std::string>& counter_names,
    uint64_t se_mask,
    uint64_t cu_mask
) {
    this->data_xmin = 0;
    this->data_ymin = 0;
    this->data_xmax = 0;
    this->data_ymax = 0;

    std::map<int, CounterData>& counters_loaded = rootnode->AccumFromChild(se_mask, cu_mask);
    if (counters_loaded.size() == 0)
        return;

    FixTime(counters_loaded, rootnode->getDelta());

    std::array<std::vector<DataPoint>, 16> datapoints;
    for (auto& dp : datapoints)
        dp.reserve(counters_loaded.size()/std::max(1ul,counter_names.size()/4));

    for (auto& [key, counter] : counters_loaded) {
        for (int i=0; i<4; i++)
            datapoints[i+4*counter.bank].push_back({counter.GetFixedTime(), counter.events[i]});
    }

    for (int i=0; i<16 && i<counter_names.size(); i++)
        AddData(counter_names[i], colors[i%colors.size()], std::move(datapoints[i]), rootnode->getDelta());
}

CounterChartView::CounterChartView(QWidget* parent): ChartView(parent) {
    rootnode = new RootNode();
}

CounterChartView::~CounterChartView() {
    if (rootnode) delete rootnode;
}

void CounterChartView::UpdateGraphTable(float mousepos) {
    for (auto& [name, series] : vseries) {
        if (!series || !series->isVisible()) continue;
        float value = series->interpFromPos(mousepos);
        float integral = series->integralFromPos(xaxis->min(), xaxis->max());
        MainWindow::window->UpdateGraphInfo(name, value, integral);
    }
}

union occupancy_data {
    struct {
        uint64_t kernel_id : 12;
        uint64_t value : 7;
        uint64_t cu : 4;
        uint64_t time : 41; // /16
    };
    uint64_t raw;
};

void OccupancyChartView::LoadOccupancyData(const std::string& filename)
{
    JsonRequest file(filename);
    QWARNING(!file.fail() && !file.bad(), "Error opening file " << filename, return);

    const int num_dispatch_ids = file.data["dispatches"].size();

    int c = 0;
    for (auto& [SE, array] : file.data.items())
    {
        if (SE == "names" || SE == "dispatches" || array.size() == 0) continue;

        int accum = 0;
        std::vector<int> current_occupancy(16*num_dispatch_ids, 0);
        std::vector<DataPoint> datapoints;

        occupancy_data data;
        data.raw = array[0];
        int64_t last_time = int64_t(data.time)-1;

        for (auto& v : array) {
            occupancy_data data;
            data.raw = v;
            while (data.time > last_time+1) {
                datapoints.push_back({16*(float)last_time, (float)accum});
                last_time += 1;
            }
            accum += data.value - current_occupancy.at(data.cu | (data.kernel_id<<4));
            current_occupancy.at(data.cu | (data.kernel_id<<4)) = data.value;
        }
        AddData("SE"+SE, colors[(c++)%colors.size()], std::move(datapoints), 16);
    }
}

void DispatchChartView::LoadOccupancyData(const std::string& filename)
{
    JsonRequest file(filename);
    QWARNING(!file.fail() && !file.bad(), "Error opening file " << filename, return);

    int num_dispatch_ids = file.data["dispatches"].size();
    std::unordered_map<int, std::string> kernel_names;
    for (auto& [id, name] : file.data["dispatches"].items())
        kernel_names[stoi(id)] = name;

    int64_t maxtime = 0;
    std::vector<std::vector<occupancy_data>> occupancy;

    for (auto& [SE, array] : file.data.items())
    {
        if (SE == "names" || SE == "dispatches" || array.size() == 0) continue;

        occupancy.push_back({});
        for (auto& v : array)
            occupancy.back().push_back(occupancy_data{.raw = v});

        if (occupancy.back().size())
            maxtime = std::max<int64_t>(maxtime, occupancy.back().back().time);
    }
    maxtime += 2;

    const int num_SEs = occupancy.size();
    std::vector<std::vector<DataPoint>> datapoints(num_dispatch_ids, std::vector<DataPoint>{});
    std::vector<std::array<int32_t, 32>> current_occupancy(num_SEs*num_dispatch_ids, std::array<int32_t, 32>{});
    std::vector<int64_t> current_iterator(num_SEs, 0);

    int64_t time = 0;
    std::vector<int> kernel_occupancy(num_dispatch_ids, 0);

    while (time < maxtime)
    {
        for (size_t occ = 0; occ < num_SEs; occ++)
        {
            while(current_iterator[occ] < occupancy[occ].size()
                && occupancy[occ][current_iterator[occ]].time < time)
            {
                auto& data = occupancy.at(occ).at(current_iterator[occ]);
                auto& current = current_occupancy.at(occ*num_dispatch_ids + data.kernel_id).at(data.cu);
                kernel_occupancy.at(data.kernel_id) += data.value - current;
                current = data.value;
                current_iterator.at(occ) += 1;
            }
        }
        for (size_t id=0; id<kernel_occupancy.size(); id++)
            datapoints[id].push_back({16*time, kernel_occupancy[id]});
        time += 1;
    }

    for (int id=0; id<num_dispatch_ids; id++)
        AddData(kernel_names[id], colors[id%colors.size()], std::move(datapoints[id]), 16);
}
