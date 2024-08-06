#include "hotspot.h"
#include "include/memtracker.h"
#include "dataseries.h"
#include "data/wavemanager.h"
#include "code/qcodeline.h"
#include <wave/token.h>
#include <vector>
#include <sstream>
#include <QStackedBarSeries>
#include <QBarSet>


HotspotView::HotspotView(
    int _n_bins,
    const std::vector<CodeData>& code,
    const std::vector<Token>& tokens,
    const std::vector<WaitList>& waitcnt,
    bool bAttribWaitcnt,
    int begin,
    int end
): n_bins(_n_bins), code_begin(begin), code_end(std::max(begin,std::min(end,(int)code.size()))) {
    this->step = std::max(1, (code_end-code_begin+n_bins-1)/n_bins);

    cycles_per_bin = std::vector<std::array<int, 16>>(n_bins);
    for (auto& a : cycles_per_bin) for (auto& v : a) v=0;

    std::unordered_map<int, int> rev_wait;
    for (int i=0; i<(int)waitcnt.size(); i++)
        rev_wait[waitcnt[i].code_line] = i;

    try {
        for (const Token& tok : tokens) {
            int waitnct_split = 1;

            if (bAttribWaitcnt && tok.type == 9 && rev_wait.find(tok.code_line) != rev_wait.end()) {
                auto& wait = waitcnt[rev_wait.at(tok.code_line)].sources;
                for (auto& [line, _] : wait) if (line < code.size()) {
                    int code_bin = getBin(line);
                    if (code_bin >= 0 && code_bin < n_bins)
                        waitnct_split += 1;
                }
                for (auto& [line, _] : wait) if (line < code.size()) {
                    int code_bin = getBin(line);
                    if (code_bin >= 0 && code_bin < n_bins)
                        cycles_per_bin.at(code_bin).at(code[line].type) += (tok.cycles+waitnct_split/2)/waitnct_split;
                }
            }
            int code_bin = getBin(tok.code_line);
            if (code_bin >= 0 && code_bin < n_bins)
                cycles_per_bin.at(code_bin).at(tok.type) += (tok.cycles+waitnct_split/2)/waitnct_split;
        }
    } catch(std::exception& e) {
        QWARNING(false, e.what(), return);
    }

    QStackedBarSeries* series = new QStackedBarSeries();
    for (int type=1; type<TokenData::INST_TYPE.size(); type++) {
        QBarSet* inst_type = new QBarSet(TokenData::GetName(type).c_str());
        for (int bin=0; bin < n_bins; bin++)
            *inst_type << cycles_per_bin[bin][type];
        inst_type->setColor(TokenData::GetColor(type));
        series->append(inst_type);
    }
    QObject::connect(series, &QStackedBarSeries::clicked, this, &HotspotView::barClickTarget);
    QObject::connect(series, &QStackedBarSeries::hovered, this, &HotspotView::barHoverTarget);

    chart = new Chart();
    chart->setAnimationDuration(200);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    this->setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    chart->addSeries(series);

    QStringList categories;
    for (int i=0; i<n_bins; i++)
        categories << QString(std::to_string(code_begin+i*step+step/2).c_str());

    QBarCategoryAxis *xaxis = new QBarCategoryAxis();
    xaxis->append(categories);
    chart->addAxis(xaxis, Qt::AlignBottom);
    xaxis->setGridLineVisible(false);
    xaxis->setTitleText("Code Line");

    QValueAxis *yaxis = new QValueAxis();
    chart->addAxis(yaxis, Qt::AlignLeft);
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setMargins(QMargins(0, 0, 0, 0));
    yaxis->setTitleText("Cycles");

    series->attachAxis(xaxis);
    series->attachAxis(yaxis);
}

// Highlights first and last line
void HotspotView::barClickTarget(int index, class QBarSet* barset) {
    int begin = code_begin + index * step;
    int end = code_begin + (index+1)*step-1;

    while (begin <= end) {
        bool bSuccess = QCodeline::HighlightLine(begin, true);
        if (bSuccess) break;
        begin ++;
    }
    while (end >= begin) {
        bool bSuccess = QCodeline::HighlightLine(end, false);
        if (bSuccess) break;
        end --;
    }
}

void HotspotView::barHoverTarget(bool status, int index, class QBarSet* barset) {
    if (!status)
        return;

    int begin = code_begin + index * step;
    int end = code_begin + (index+1)*step - 1;
    std::stringstream tooltip;
    tooltip << "Lines: " << begin << '-' << end << '\n';

    for (int type=1; type<TokenData::INST_TYPE.size(); type++)
        tooltip << TokenData::INST_TYPE[type] << ": \t" << cycles_per_bin[index][type] << '\n';
    this->setToolTip(tooltip.str().c_str());
}
