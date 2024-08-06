#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "include/custom_layouts.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
friend class OptionsDialogH;
public:
    MainWindow(std::pair<std::string, std::string> attdirs, QWidget *parent = nullptr);
    virtual ~MainWindow();
    std::string GetJsonDir();
    void ResetSelector();

    void ResetCountersChart();
    void CreateCountersChart();
    void UpdateCountersChartSelection();
    void CreateOccupancyChart(bool bDispatch);
    void CreateWavesChart();
    void setChartBarPos(float x);
    void UpdateGraphInfo(const std::string& name, int value, float integral);
    void UpdateGraphAutoLod(int bAutoLod);
    void OpenCollectionDialog();
    void OpenParserDialog();
    void ImportInputFile();
    void ExportInputFile();
    void InputEditDialog();
    void SetProjectFolder();
    void SetJsonsFolder();
    void CountersFilter();
    void OpenOptionsDialog();
    void AddHotspot();
    void SetWaveViewMipmap(int value);
    int GetHotspotBins() const { return hotspot_n_bins; }
    void SetWaveViewRange();
    std::vector<std::string> GetFilenamesForHotspot();

    uint64_t GetSEMask() { return ToMask(se_enable_list); }
    uint64_t GetCUMask() { return ToMask(cu_enable_list); }
    static uint64_t ToMask(const std::vector<bool>& list);

    const std::string& GetProjectDir() const { return project_dir; }

    class QLayout* widSel = nullptr;
    class QLayout* wslSel = nullptr;
    class QLayout* simdSel = nullptr;
    class QLayout* shaderSel = nullptr;
    class QCodelist* code_contents = nullptr;
    class QWaveView* waveview_contents = nullptr;
    class QWaveSlots* cuwaves_content = nullptr;
    class QScrollArea* code_scrollarea = nullptr;
    class QScrollArea* waveview_scrollarea = nullptr;
    class QWidget* wv_states = nullptr;
    class CounterChartView* counters_chart = nullptr;
    class WaveChartView* waves_chart = nullptr;
    class OccupancyChartView* occupancy_chart = nullptr;
    class OccupancyChartView* dispatch_chart = nullptr;
    class HotspotView* hotspot_chart = nullptr;
    class QScrollArea* cuwaves_scrollarea = nullptr;

    class QGridLayout* waves_chart_layout = nullptr;
    class QGridLayout* counters_chart_layout = nullptr;
    class QGridLayout* occupancy_chart_layout = nullptr;
    class QGridLayout* dispatch_chart_layout = nullptr;
    class QTableWidget* wave_info_table = nullptr;
    class QTableWidget* graph_info_table = nullptr;
    class QWidget* source_tab = nullptr;
    class QWidget* hotspot_tab = nullptr;
    class QTabWidget* source_tab_parent = nullptr;
    class QScrollArea* source_scrollArea = nullptr;
    class InputFile* input_file = nullptr;

    class WaveIDSelector* widSelector = nullptr;
    static MainWindow* window;
    bool bHotspotGatherAllWaves = false;
private:
    Ui::MainWindow *ui = nullptr;
    class SESelector* seSelector = nullptr;
    std::string lastPath = "";

    std::unordered_map<std::string, class QLabel*> counter_values_tableitem;
    std::unordered_map<std::string, class QLabel*> counter_integral_tableitem;

    std::string project_dir;
    std::string jsons_dir;
    int hotspot_n_bins = 18;
    int hotspot_begin = 0;
    int hotspot_end = 1000000;
    bool bHospotTrackWaitcnt = false;

    std::vector<bool> cu_enable_list = {};
    std::vector<bool> se_enable_list = {};
};
#endif // MAINWINDOW_H
