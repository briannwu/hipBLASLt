#include "datanode.h"

//std::unordered_map<int8_t, int64_t> CounterData::shader_order_offset = {};

DataNode* DataNode::GetChild(int coord) {
    if (!leaves[coord])
        leaves[coord] = new DataNode();
    return leaves[coord];
}

std::map<int, CounterData>& DataNode::AccumFromChild(uint64_t se, uint64_t cu) {
    if (leaves.size() == 0)
        return data;
    data = {};
    //data.reserve(8192); // Todo: Improve this
    for (auto& [idx, node] : leaves) {
        if (!node) continue;
        auto& child_data = node->AccumFromChild(se, cu);
        for (auto& [key, child_point] : child_data) {
            if (!((1 << child_point.se) & se) || !((1<<child_point.cu) & cu))
                continue;

            auto it = data.find(key);    // Equal only if same timestamp and same bank!
            if (it == data.end())        // Todo: Optimize from ordered map
                data.insert({child_point.GetOrderedOperator(), child_point});
            else
                (*it).second += child_point;
        }
    }
    return this->data;
}

DataNode::~DataNode() {
    for (auto& [key, c] : leaves)
        if (c)
            delete c;
}

DataNode::Histogram& DataNode::updateDelta() { // TODO: Cache this result
    histogram.clear();
    histogram[LONG_MAX/4] = 0; // Default to large number

    if (leaves.size() == 0) {
        int64_t last_time_bank[4] = {-INT_MAX, -INT_MAX, -INT_MAX, -INT_MAX}; // just a small number
        for (auto& [key, counter] : data) {
            int64_t delta = counter.time - last_time_bank[counter.bank];
            if (!delta) continue;
            histogram[delta] ++;
            last_time_bank[counter.bank] = counter.time;
        }
        return histogram;
    } else {
        for (auto& [idx, node] : leaves) {
            if (!node) continue;
            const Histogram& leafhisto = node->updateDelta();
            for (auto& [k, v] : leafhisto)
                histogram[k] += v;
        }
    }
    return histogram;
}

void RootNode::Insert(int SE, const std::vector<CounterData>& data) {
    DataNode* se_node = this->GetChild(SE);
    //se_node->data.reserve(data.size()/12); // Todo: Optimize

    for(auto& datapoint : data) {
        DataNode* cu_node = se_node->GetChild(datapoint.cu);
        cu_node->data[datapoint.GetOrderedOperator()] = datapoint;
    }
}

std::map<int, CounterData>& RootNode::AccumFromChild(uint64_t se, uint64_t cu) {
    this->updateDelta();

    auto compare = [](HistogramData a, HistogramData b) { return a.second < b.second; };
    this->delta_time = std::max_element(histogram.begin(), histogram.end(), compare)->first;

    std::map<int, CounterData>& merged = this->Super::AccumFromChild(se, cu);

    std::array<std::unordered_map<int, CounterData>, 4> last_se_value;
    std::array<CounterData, 4> current_sum;

    for (auto& [t, d] : merged) {
        struct CounterData& sum_in_bank = current_sum[d.bank];
        for (auto& [se, last_in_se] : last_se_value[d.bank]) {
            if (last_in_se.time && d.time - last_in_se.time > delta_time) {
                sum_in_bank -= last_in_se;
                last_in_se = CounterData();
            }
        }
        sum_in_bank += d;
        sum_in_bank -= last_se_value[d.bank][d.se];
        last_se_value[d.bank][d.se] = d;

        for (int c=0; c<4; c++)
            d.events[c] = sum_in_bank.events[c];
    }

    return merged;
}
