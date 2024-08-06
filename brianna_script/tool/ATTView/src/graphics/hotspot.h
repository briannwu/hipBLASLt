#pragma once

#include "include/custom_layouts.h"
#include <QtCharts/QtCharts>
#include <array>
#include <vector>

class HotspotView : public QChartView {
    Q_OBJECT
    set_tracked();
    using Super = QChartView;
public:
    HotspotView(
        int bins,
        const std::vector<class CodeData>& code,
        const std::vector<class Token>& tokens,
        const std::vector<class WaitList>& waitcnt,
        bool bAttribWaitcnt,
        int begin,
        int end
    );
    //virtual ~HotspotChart() { /*for (auto* series : vseries) delete series; */ if(chart) delete chart; };
    void HighlightLines(int bin);
    void barClickTarget(int index, class QBarSet* barset);
    void barHoverTarget(bool status, int index, class QBarSet* barset);
protected:
    int getBin(int line) { return (line - code_begin)/step; }
    const int code_begin;
    const int code_end;
    const int n_bins;
    class Chart* chart = nullptr;
    int step;
    std::vector<std::array<int, 16>> cycles_per_bin;
};
