#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "code/qcodelist.h"
#include "wave/waveview.h"
#include "button/jsonselector.h"
#include "graphics/chart.h"
#include "graphics/hotspot.h"
#include "graphics/specialized_charts.h"
#include "collection/inputfile.h"
#include "collection/dialogs.h"
#include "collection/options.h"
#include <fstream>
#include <sys/stat.h>
#include <QFileDialog>
#include "data/wavedata.h"
#include <QSpinBox>
#include "graphics/container/counterdialog.h"

inline bool FileExists(const std::string& name) {
    if (name.find("http://") != std::string::npos) {
        try {
            // TODO: Only check if file exists on the server.
            JsonRequest try_request(name);
        } catch (std::exception& e) {
            return false;
        }
        return true;
    }
    struct stat buffer;   
    return stat(name.c_str(), &buffer) == 0;
}

MainWindow* MainWindow::window = nullptr;

MainWindow::MainWindow(std::pair<std::string, std::string> attdirs, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    MainWindow::window = this;
    ui->setupUi(this);
    this->shaderSel = ui->SESelLay;
    this->simdSel = ui->SMSelLay;
    this->wslSel = ui->WSLSelLay;
    this->widSel = ui->WIDSelLay;
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(215, 215, 215));
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    ui->splitter->setSizes(QList<int>({height()/3, 2*height()/3}));
    ui->splitter_2->setSizes(QList<int>({height()/6, height()/2, 2*height()/6}));

    for (int i=0; i<16; i++)
        cu_enable_list.push_back(true);

    this->graph_info_table = ui->graph_info_table;
    this->graph_info_table->setHorizontalHeaderLabels(QList<QString>({"Counter", "Value", "%/View"}));
    this->wave_info_table = ui->wave_info_table;
    this->wave_info_table->setHorizontalHeaderLabels(QList<QString>({"Type", "Hit", "Stalls"}));
    this->hotspot_tab = ui->hotspot_tab;

    QVBox* code_view_layout = new QVBox();
    ui->code_tab->setLayout(code_view_layout);

    this->source_tab = ui->sourcecode_contents;
    this->source_tab_parent = ui->tabWidget_2;
    this->source_scrollArea = ui->source_scrollArea;

    // --- Wave View ---
    this->waveview_scrollarea = new QScrollArea(this);
    this->waveview_scrollarea->setWidgetResizable(true);
    this->waveview_contents = new QWaveView(this->waveview_scrollarea, this->waveview_scrollarea);
    this->waveview_scrollarea->setWidget(waveview_contents);
    this->waveview_scrollarea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->waveview_scrollarea->setFixedHeight(64);

    code_view_layout->addWidget(this->waveview_scrollarea);

    // --- Code View ---
    this->code_scrollarea = new QScrollArea(this);
    this->code_scrollarea->setWidgetResizable(true);
    this->code_contents = new QCodelist();
    this->code_scrollarea->setWidget(code_contents);
    code_view_layout->addWidget(this->code_scrollarea);

    // --- CU Waves View ---
    this->cuwaves_scrollarea = new QScrollArea(ui->cuwaves_tab);
    this->cuwaves_scrollarea->setWidgetResizable(true);
    this->cuwaves_content = new QWaveSlots(this);
    this->cuwaves_scrollarea->setWidget(this->cuwaves_content);
    ui->cuwaves_tab->setLayout(new QBox());
    ui->cuwaves_tab->layout()->addWidget(this->cuwaves_scrollarea);

    // --- MENU ---
    if (attdirs.first != "") {
        jsons_dir = attdirs.first+'/';
        project_dir = attdirs.second + '/';
        ui->ConfigNameEdit->setText(jsons_dir.c_str());
        ui->ProjectFEdit->setText(project_dir.c_str());
        ResetSelector();
    }
    connect(ui->ConfigNameEdit, &QLineEdit::editingFinished, this, &MainWindow::ResetSelector);
    connect(ui->ProjectFEdit, &QLineEdit::editingFinished, this, &MainWindow::ResetSelector);
    connect(ui->lod_checkBox, &QCheckBox::stateChanged, this, &MainWindow::UpdateGraphAutoLod);

    connect(ui->actionATT_Parser, &QAction::triggered, this, &MainWindow::OpenParserDialog);
    connect(ui->actionATT_Collect, &QAction::triggered, this, &MainWindow::OpenCollectionDialog);

    connect(ui->actionATT_Input_file_4, &QAction::triggered, this, &MainWindow::InputEditDialog);
    connect(ui->actionATT_Input_file_3, &QAction::triggered, this, &MainWindow::ImportInputFile);
    connect(ui->actionATT_Input_file_2, &QAction::triggered, this, &MainWindow::ExportInputFile);

    connect(ui->actionProject_folder, &QAction::triggered, this, &MainWindow::SetProjectFolder);
    connect(ui->actionJsons_folder, &QAction::triggered, this, &MainWindow::SetJsonsFolder);
    connect(ui->actionHotOptions, &QAction::triggered, this, &MainWindow::OpenOptionsDialog);
    connect(ui->actionCounters_filter, &QAction::triggered, this, &MainWindow::CountersFilter);

    connect(ui->waveSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::SetWaveViewMipmap);

    ui->wview_range_min->setValidator( new QIntValidator(this) );
    ui->wview_range_max->setValidator( new QIntValidator(this) );
    connect(ui->wview_range_min, &QLineEdit::editingFinished, this, &MainWindow::SetWaveViewRange);
    connect(ui->wview_range_max, &QLineEdit::editingFinished, this, &MainWindow::SetWaveViewRange);
    //QObject::connect(this->code_scrollarea->verticalScrollBar(),
    //    &QScrollBar::valueChanged, this->code_contents, &QCodelist::MoveTo);
}

std::string MainWindow::GetJsonDir() { return ui->ConfigNameEdit->displayText().toStdString()+'/'; }

void MainWindow::SetWaveViewRange() {
    try {
        std::string str_vmin = ui->wview_range_min->displayText().toStdString();
        std::string str_vmax = ui->wview_range_max->displayText().toStdString();

        int vmin = stoi(str_vmin);
        int vmax = stoi(str_vmax);

        QWaveSection::SetViewRange(vmin, vmax);
    } catch (std::exception& e) {
        QWARNING(false, "Could not set range values.", return);
    }
}

void MainWindow::OpenOptionsDialog() {
    OptionsDialogH dialog;
    dialog.exec();
}

void MainWindow::OpenParserDialog() {
    AnalyseDialog dialog(GetJsonDir(), "");
    dialog.exec();
}

void MainWindow::OpenCollectionDialog() {
    CollectDialog dialog(GetJsonDir(), "");
    dialog.exec();
}

void MainWindow::InputEditDialog() {
    if (!this->input_file)
        this->input_file = new InputFile();

    InputFileEdit dialog(*input_file);
    dialog.exec();
}

void MainWindow::CountersFilter() {
    CounterDialog dialog(cu_enable_list, se_enable_list);
    dialog.exec();

    UpdateCountersChartSelection();
}

void MainWindow::SetProjectFolder() {
    project_dir = QFileDialog::getExistingDirectory(
        this, "Select Dir", project_dir.c_str()).toStdString();
    ui->ProjectFEdit->setText(project_dir.c_str());
    ResetSelector();
}

void MainWindow::SetJsonsFolder() {
    jsons_dir = QFileDialog::getExistingDirectory(
        this, "Select Dir", jsons_dir.c_str()).toStdString();
    ui->ConfigNameEdit->setText(jsons_dir.c_str());
    ResetSelector();
}

void MainWindow::ImportInputFile() {
    std::string filename = QFileDialog::getOpenFileName(
        this, "Select file", GetJsonDir().c_str(), "Text (*.txt)"
    ).toStdString();

    QWARNING(FileExists(filename), "Invalid file:", return);

    if (input_file)
        delete input_file;

    input_file = new InputFile(filename);
}

void MainWindow::ExportInputFile() {
    QWARNING(input_file, "InputFile not loaded.", return);

    std::string filename = QFileDialog::getSaveFileName(
        this, "Select file", GetJsonDir().c_str(), "Text (*.txt)"
    ).toStdString();

    input_file->Export(filename);
}

void MainWindow::ResetSelector() {
    project_dir = ui->ProjectFEdit->displayText().toStdString();
    while (project_dir.size() && project_dir.back() == '/')
        project_dir = project_dir.substr(0, project_dir.size()-1);
    project_dir = project_dir+'/';

    std::string newpath = GetJsonDir();
    while (newpath.size() && newpath.back() == '/')
        newpath = newpath.substr(0, newpath.size()-1);
    newpath += "/filenames.json";
    if (project_dir+newpath == lastPath) return;

    std::shared_ptr<JsonRequest> request;
    try {
        request = std::make_shared<JsonRequest>(GetJsonDir()+"filenames.json");
        if (!request) throw std::exception();
    } catch (std::exception& e) {
        QWARNING(false, "Invalid filename " << newpath, return);
    }
    lastPath = project_dir+newpath;

    if (this->seSelector) delete this->seSelector;
    this->seSelector = nullptr;
    this->seSelector = new SESelector(*request);
    if (this->bHotspotGatherAllWaves) // TODO: Make global hotspot be for all SEs
        this->AddHotspot();

    CreateCountersChart();
    CreateWavesChart();
    CreateOccupancyChart(false);
    CreateOccupancyChart(true);
    UpdateCountersChartSelection();
    if (this->counters_chart)
        this->counters_chart->setAutoLod(ui->lod_checkBox->isChecked());

    QWaveSection::ReloadRange();
    //this->counters_chart->UpdateMip();
}

MainWindow::~MainWindow()
{
    if (waveview_contents) delete waveview_contents;
    if (waveview_scrollarea) delete waveview_scrollarea;

    if (cuwaves_content) delete cuwaves_content;
    if (cuwaves_scrollarea) delete cuwaves_scrollarea;

    //if (code_contents) delete code_contents;
    //if (code_scrollarea) delete code_scrollarea;

    if (waves_chart) delete waves_chart;
    if (counters_chart) delete counters_chart;

    if (counters_chart_layout) delete counters_chart_layout;
    if (waves_chart_layout) delete waves_chart_layout;

    if (dispatch_chart) delete dispatch_chart;
    if (occupancy_chart) delete occupancy_chart;
    if (dispatch_chart_layout) delete dispatch_chart_layout;
    if (occupancy_chart_layout) delete occupancy_chart_layout;

    if (seSelector) delete seSelector;
    if (widSelector) delete widSelector;
    if (ui) delete ui;

    WaveReader::InvalidadeCache();
    MainWindow::window = nullptr;
}

void MainWindow::ResetCountersChart() {
    if (this->counters_chart_layout) delete this->counters_chart_layout;
    
    this->counters_chart = new CounterChartView(this);
    this->counters_chart->setGeometry(0,0,300,this->counters_chart->size().width());

    counters_chart_layout = new QBox();
    counters_chart_layout->setContentsMargins(0,0,0,0);
    counters_chart_layout->setMargin(0);
    ui->wv_counters_tab->setLayout(counters_chart_layout);
    counters_chart_layout->addWidget(this->counters_chart);
}

void MainWindow::CreateCountersChart() {
    QWARNING(this->seSelector, "Selector missing", return);
    QWARNING(this->seSelector->names.size(), "Selector has no SE data", return;);

    ResetCountersChart();

    QWARNING(this->counters_chart, "Chart missing", return);
    se_enable_list = {};

    int max_se = 0;
    for(const auto& SE_name : this->seSelector->names) {
        try {
            max_se = std::max(max_se, stoi(SE_name));
        } catch (...) {};
    }

    for(int se_num=0; se_num<max_se; se_num++) {
        std::string filename = GetJsonDir() + "se" + std::to_string(se_num) + "_perfcounter.json";
        //QWARNING(FileExists(filename), "File " << filename << " not found", continue);
        try {
            JsonRequest file(filename);
            QWARNING(!file.fail(), "Error opening file " << filename, continue);
            this->counters_chart->LoadCounterData(file, se_num);
        } catch (...) {}
        while (se_enable_list.size() <= se_num) se_enable_list.push_back(true);
    }
}

void MainWindow::UpdateCountersChartSelection() {
    QWARNING(this->counters_chart, "Chart missing", return);
    std::string counters_path = GetJsonDir()+"graph_options.json";

    std::vector<std::string> counters_names;
    try {
        JsonRequest file(counters_path);
        QWARNING(!file.fail() && !file.bad(), "File " << counters_path << " not found", return);
        counters_names = file.data["counters.png"];
    } catch (...) {
        return; // No counters are present
    }

    this->counters_chart->UpdateDataSelection(counters_names, GetSEMask(), GetCUMask());

    int i = 4;
    for (auto& name : counters_names) {
        class QLabel* v_label = new QLabel("");
        class QLabel* i_label = new QLabel("");
        counter_values_tableitem[name] = v_label;
        counter_integral_tableitem[name] = i_label;

        graph_info_table->setCellWidget(i, 0, new QLabel(name.c_str()));
        graph_info_table->setCellWidget(i, 1, v_label);
        graph_info_table->setCellWidget(i, 2, i_label);
        i++;
    }
}

void MainWindow::CreateWavesChart() {
    //if (this->waves_chart) delete this->waves_chart;
    if (this->waves_chart_layout) delete this->waves_chart_layout;

    this->waves_chart = new WaveChartView(this);
    this->waves_chart->setGeometry(0,0,300,this->waves_chart->size().width());
    this->waves_chart_layout = new QBox();
    waves_chart_layout->setContentsMargins(0,0,0,0);
    waves_chart_layout->setMargin(0);
    ui->wv_states_tab->setLayout(waves_chart_layout);
    waves_chart_layout->addWidget(this->waves_chart);
    this->waves_chart->LoadWaveStateData(GetJsonDir(), 0);
    this->waves_chart->setAutoLod(ui->lod_checkBox->isChecked());
    //this->counters_chart->UpdateMip();

    for (int i=0; i<WaveChartView::state_names.size()-1; i++) {
        auto& name = WaveChartView::state_names[i+1];
        class QLabel* v_label = new QLabel("");
        class QLabel* i_label = new QLabel("");

        counter_values_tableitem[name] = v_label;
        counter_integral_tableitem[name] = i_label;

        graph_info_table->setCellWidget(i, 0, new QLabel(("Waves " + name).c_str()));
        graph_info_table->setCellWidget(i, 1, v_label);
        graph_info_table->setCellWidget(i, 2, i_label);
    }
    graph_info_table->setToolTip("Displays current values under the mouse pointer and total shown on the current view. For waves, displays current values and percentage of each wave state.");
}

void MainWindow::CreateOccupancyChart(bool bDispatch)
{
    auto* layout = bDispatch ? this->dispatch_chart_layout : this->occupancy_chart_layout;
    auto* chart = bDispatch ? this->dispatch_chart : this->occupancy_chart;

    if (layout) delete layout;
    if (chart) delete chart;

    chart = bDispatch ? new DispatchChartView(this) : new OccupancyChartView(this);
    chart->setGeometry(0,0,300,chart->size().width());
    layout = new QBox();
    layout->setContentsMargins(0,0,0,0);
    layout->setMargin(0);
    if (bDispatch)
    {
        if (ui->dispatch_tab->layout())
            delete ui->dispatch_tab->layout();
        ui->dispatch_tab->setLayout(layout);
    }
    else
    {
        if (ui->occupancy_tab->layout())
            delete ui->occupancy_tab->layout();
        ui->occupancy_tab->setLayout(layout);
    }
    layout->addWidget(chart);
    chart->LoadOccupancyData(GetJsonDir()+"occupancy.json");
    chart->setAutoLod(ui->lod_checkBox->isChecked());
}

void MainWindow::setChartBarPos(float x) {
    if (waves_chart)
        waves_chart->SetBarPos(x);
    if (counters_chart)
        counters_chart->SetBarPos(x);
    if (occupancy_chart)
        occupancy_chart->SetBarPos(x);
}

void MainWindow::UpdateGraphInfo(const std::string& name, int value, float integral) {
    QWARNING(graph_info_table, "No graph info", return);

    QLabel* v_label = counter_values_tableitem[name];
    if (v_label)
        v_label->setText(std::to_string(value).c_str());

    std::stringstream ss;
    ss << std::setprecision(4) << integral;
    QLabel* i_label = counter_integral_tableitem[name];
    if (i_label)
        i_label->setText(ss.str().c_str());
}

void MainWindow::UpdateGraphAutoLod(int bAutoLod) {
    if (waves_chart)
        waves_chart->setAutoLod((bool)bAutoLod);
    if (counters_chart)
        counters_chart->setAutoLod((bool)bAutoLod);
    if (occupancy_chart)
        occupancy_chart->setAutoLod((bool)bAutoLod);
}

void MainWindow::AddHotspot() {
    QWARNING(widSelector, "No widSelector", return);
    QWARNING(hotspot_tab && hotspot_tab->layout(), "No hotspot tab", return);

    auto filenames = GetFilenamesForHotspot();
    WaveManager manager(filenames, 0, (1l<<48l)-1);

    if (hotspot_chart)
        delete hotspot_chart;
    hotspot_chart = new HotspotView(
        hotspot_n_bins,
        manager.GetCode(),
        manager.GetTokens(),
        manager.GetWaitcnt(),
        bHospotTrackWaitcnt,
        hotspot_begin,
        hotspot_end
    );
    hotspot_tab->layout()->addWidget(hotspot_chart);
}

void MainWindow::SetWaveViewMipmap(int value) {
    QWARNING(waveview_scrollarea && waveview_contents, "No scroll area", return);
    value = std::max(0, 10 - value);
    int width = waveview_scrollarea->width();

    int old_value = QWaveSection::GetMip();
    int scrollbar_value = waveview_scrollarea->horizontalScrollBar()->value() + width/2;
    int cuwaves_scrollbar_value = cuwaves_scrollarea->horizontalScrollBar()->value() + width/2;
    QWaveSection::SetMip(value);

    if (old_value > value) {
        scrollbar_value = scrollbar_value << (old_value - value);
        cuwaves_scrollbar_value = cuwaves_scrollbar_value << (old_value - value);
    } else {
        scrollbar_value = scrollbar_value >> (value - old_value);
        cuwaves_scrollbar_value = cuwaves_scrollbar_value >> (value - old_value);
    }
    waveview_scrollarea->horizontalScrollBar()->setValue(scrollbar_value - width/2);
    cuwaves_scrollarea->horizontalScrollBar()->setValue(cuwaves_scrollbar_value - width/2);
}

uint64_t MainWindow::ToMask(const std::vector<bool>& list) {
    uint64_t mask = 0;
    for (int i=0; i<list.size(); i++)
        mask |= (list[i]!=false) << i;
    return mask;
}

std::vector<std::string> MainWindow::GetFilenamesForHotspot() {
    QWARNING(widSelector, "No widSelector", return {});

    if (!bHotspotGatherAllWaves || !seSelector)
        return {widSelector->GetJsonPath(widSelector->currentText().toStdString())};

    auto& current_se = seSelector->data_json[seSelector->currentText().toStdString()];

    std::vector<std::string> allFileNames;
    for (auto& simd : current_se) {
        for (auto& slot : simd) {
            for (auto& [wid, fname] : slot.items())
                allFileNames.push_back(GetJsonDir()+std::string(fname[0]));
        }
    }

    return allFileNames;
}
