#include <fstream>
#include "jsonselector.h"
#include "mainwindow.h"
#include "code/qcodelist.h"
#include "wave/waveview.h"
#include <QWidget>
#include <QTableWidget>
#include "data/wavedata.h"

static void sort_by_id(std::vector<std::string>& names) {
    try {
        std::sort(names.begin(), names.end(), [](
            const std::string& v1, const std::string& v2) -> bool {
                return stoi(v1) < stoi(v2);
        });
    } catch(std::exception& e) {
        QWARNING(false, "Could not sort filenames:" << e.what(), return);
    }
}

void Selector::addSortedNames() {
    names.clear();
    for(auto& [name, value] : data_json.items()) {
        if (value.size() == 0) continue;
        names.push_back(name);
    }

    sort_by_id(names);

    for(auto& name : names)
        this->addItem(QString(name.c_str()));
}

SESelector::SESelector(JsonRequest& request):
    Selector(
        request.data["wave_filenames"],
        MainWindow::window->shaderSel,
        nullptr
    ) {
    try {
        WaveManager::global_begin_time = request.data["global_begin_time"];
    } catch(std::exception& e) {
        WaveManager::global_begin_time = 0;
    }
    try {
        WaveManager::bIsNaviWave = std::string(request.data["gfxv"]) == "navi";
    } catch(std::exception& e) {
        WaveManager::bIsNaviWave = false;
    }

    // Possible new set of files with same name, so json cache needs to be flushed
    WaveReader::InvalidadeCache();
    if (names.size() > 0) this->textChanged(names[0].c_str());
    QObject::connect(this, &QComboBox::currentTextChanged, this, &SESelector::textChanged);
}

void SESelector::textChanged(const QString &text) {
    if (this->selection)
        delete this->selection;
    this->selection = new SimdSelector(data_json[text.toStdString()], this);
}

SimdSelector::SimdSelector(json& _data, SESelector* parent): 
    Selector(_data, MainWindow::window->simdSel, parent) {

    if (names.size() > 0) this->textChanged(names[0].c_str());
    QObject::connect(this, &QComboBox::currentTextChanged, this, &SimdSelector::textChanged);
}

void SimdSelector::textChanged(const QString &text) {
    if (this->selection)
        delete this->selection;
    this->selection = new WSlotSelector(data_json[text.toStdString()], this);
}

WSlotSelector::WSlotSelector(json& _data, SimdSelector* parent):
    Selector(_data, MainWindow::window->wslSel, parent) {

    if (names.size() > 0) this->textChanged(names[0].c_str());
    QObject::connect(this, &QComboBox::currentTextChanged, this, &WSlotSelector::textChanged);
}

void WSlotSelector::textChanged(const QString &text) {
    if (this->selection)
        delete this->selection;
    this->selection = new WaveIDSelector(data_json[text.toStdString()], this);
}

WaveIDSelector::WaveIDSelector(json& _data, WSlotSelector* parent):
    Selector(_data, MainWindow::window->widSel, parent) {

    MainWindow::window->widSelector = this;
    if (names.size() > 0) this->textChanged(names[0].c_str());
    QObject::connect(this, &QComboBox::currentTextChanged, this, &WaveIDSelector::textChanged);
}

WaveIDSelector::~WaveIDSelector() {
    if (MainWindow::window && MainWindow::window->widSelector == this)
        MainWindow::window->widSelector = nullptr;
}

std::string WaveIDSelector::GetJsonPath(const std::string& waveslot) const {
    Selector* wslotSel = selector_parent;
    Selector* simdSel = wslotSel->selector_parent;
    Selector* shaderSel = simdSel->selector_parent;

    return  MainWindow::window->GetJsonDir()
            +"se"+shaderSel->currentText().toStdString()
            +"_sm"+simdSel->currentText().toStdString()
            +"_sl"+wslotSel->currentText().toStdString()
            +"_wv"+waveslot+".json";
}

void WaveIDSelector::textChanged(const QString &text) {
    if (this->selection)
        delete this->selection;

    Selector* wslotSel = selector_parent;
    Selector* simdSel = wslotSel->selector_parent;
    Selector* shaderSel = simdSel->selector_parent;

    std::string filename = GetJsonPath(text.toStdString());

    auto* waveview = MainWindow::window->waveview_contents;
    auto* code_contents = MainWindow::window->code_contents;

    QWARNING(code_contents, "No code widget", return);
    code_contents->Populate(filename);

    QWARNING(waveview, "No waveview widget", return);
    waveview->SetAsMain();
    waveview->Reset();
    waveview->Populate(WaveManager(filename));

    if (!MainWindow::window->bHotspotGatherAllWaves) // TODO: Make global hotspot be for all SEs
        MainWindow::window->AddHotspot();
    QWARNING(MainWindow::window->cuwaves_content, "No CU waves widget", return);
    MainWindow::window->cuwaves_content->Reset();

    for (auto& [simd_name, current_simd] : shaderSel->data_json[shaderSel->currentText().toStdString()].items()) {
        try {
            std::vector<std::string> slotnames;
            for (auto& [slot_name, slot_content] : current_simd.items())
                slotnames.push_back(slot_name);
            sort_by_id(slotnames);

            for (auto& slot_name : slotnames) {
                std::vector<std::string> allFileNames;
                for (auto& [wid, fname] : current_simd[slot_name].items())
                    if (int64_t(fname[1])-WaveManager::global_begin_time < waveview->endtime &&
                        int64_t(fname[2])-WaveManager::global_begin_time > waveview->begintime) {
                        allFileNames.push_back(MainWindow::window->GetJsonDir()+std::string(fname[0]));
                }

                if (allFileNames.size() == 0) continue;

                WaveManager manager(allFileNames, waveview->begintime, waveview->endtime);
                QWaveView* view = new QWaveView(
                    MainWindow::window->cuwaves_content,
                    MainWindow::window->cuwaves_scrollarea
                );
                view->Populate(manager);
                MainWindow::window->cuwaves_content->AddSlot(view, simd_name+'-'+slot_name);
            }
        } catch (std::exception& e) {
            QWARNING(false, e.what(), return);
        }
    }
    MainWindow::window->cuwaves_content->layout->insertStretch(100);

    QTableWidget* wave_info = MainWindow::window->wave_info_table;
    QWARNING(wave_info, "No wave info", return);

    int i = 0;
    for (auto& [name, value, stalls] : WaveManager(filename).GetWaveInfo()) {
        wave_info->setCellWidget(i, 0, new QLabel(name.c_str()));
        wave_info->setCellWidget(i, 1, new QLabel(std::to_string(value).c_str()));
        wave_info->setCellWidget(i, 2, new QLabel(std::to_string(stalls).c_str()));
        i++;
    }
}
