#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include "include/custom_layouts.h"
#include <sstream>

struct CounterData {
    int64_t time = 0;
    int32_t events[4] = {0,0,0,0};
    int8_t cu = 0;
    int8_t bank = 0;
    int8_t se = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "t:" << time << " c:" << int(cu) << " b:" << int(bank) << " se:" << int(se) << " ev: ";
        ss << events[0] << " " << events[1] << " " << events[2] << " " << events[3] << '\n';
        return ss.str();
    }
    CounterData& operator+=(const CounterData& other) {
        for (int i=0; i<4; i++)
            events[i] += other.events[i];
        return *this;
    }
    CounterData& operator-=(const CounterData& other) {
        for (int i=0; i<4; i++)
            events[i] -= other.events[i];
        return *this;
    }
    uint64_t GetOrderedOperator() const {
        //return ((time >> 2) & ~0x3ull) | bank;
        //return ((time - shader_order_offset[se]) & ~0x3ull) | bank;
        return (time & ~0x3ull) | bank;
    }
    bool operator==(const CounterData& other) const {
        return this->GetOrderedOperator() == other.GetOrderedOperator();
    }
    bool operator<(const CounterData& other) const {
        return GetOrderedOperator() < other.GetOrderedOperator();
    }
    bool operator<=(const CounterData& other) const {
        return GetOrderedOperator() <= other.GetOrderedOperator();
    }
    uint64_t GetFixedTime() const {
        return time;// - shader_order_offset[se];
    }

    //static std::unordered_map<int8_t, int64_t> shader_order_offset;
};

class DataNode {
    set_tracked();
public:
    typedef std::unordered_map<int64_t, int> Histogram;
    typedef std::pair<int64_t, int> HistogramData;

    std::map<int, CounterData> data;
    std::unordered_map<int, DataNode*> leaves;

    DataNode() {};
    virtual ~DataNode();
    DataNode* GetChild(int coord);
    virtual std::map<int, CounterData>& AccumFromChild(uint64_t se, uint64_t cu);
protected:
    Histogram& updateDelta();
    Histogram histogram;
};

// Root = GPU
class RootNode : public DataNode {
    using Super = DataNode;
public:
    void Insert(int SE, const std::vector<CounterData>& data);
    virtual std::map<int, CounterData>& AccumFromChild(uint64_t se, uint64_t cu) override;
    int64_t getDelta() const { return this->delta_time; };
    int64_t delta_time;
};
