

#include "dialogs.h"
#include <fstream>
#include <QCheckBox>
#include <QWidget>
#include "mainwindow.h"
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <filesystem>
#include "inputfile.h"
#include <QWidget>
#include <QIntValidator>
#include <QPushButton>

namespace fs = std::filesystem;

void ATTFileList::ResetKernel(const std::string& fname) {
    size_t slash = fname.rfind('/');
    size_t name_match_pos = fname.rfind("kernel.txt");
    QWARNING(name_match_pos < fname.size() && slash < name_match_pos, "Invalid name: " << fname, return);

    std::string dir = fname.substr(0, slash);
    std::string name_match = fname.substr(0, name_match_pos);

    std::cout << "Dir: " << dir << std::endl;
    for (QCheckBox* att : list)
        delete att;
    list = {};

    QVBox* layout = new QVBox();
    this->setLayout(layout);

    std::vector<QCheckBox*> filelist;
    for (const auto & entry : fs::directory_iterator(dir)) {
        std::string fpath = entry.path().string();
        if (fpath.find(name_match) == std::string::npos || fpath.rfind(".att") != fpath.size()-4)
            continue;
        QCheckBox* check = new QCheckBox(fpath.c_str());
        layout->addWidget(check);
        check->setChecked(true);
        filelist.push_back(check);
    }
    layout->insertStretch(999);
}

std::vector<std::string> ATTFileList::GetATTFileNames() {
    std::vector<std::string> flist;
    for (QCheckBox* box : list) {
        if (box->isChecked())
            flist.push_back(box->text().toStdString());
    }
    return flist;
}

AnalyseDialog::AnalyseDialog(const std::string& defkernel, const std::string& definput): QDialog() {
    this->setWindowTitle("Parse ATT data");
    layout = new QVBox(this);

    this->setLayout(layout);
    kernelinfo = new QLineEdit();
    kernelinfo->setText(defkernel.c_str());
    inputfile = new QLineEdit();
    inputfile->setText(definput.c_str());
    assemblyfile = new QLineEdit();
    assemblyfile->setText(definput.c_str());

    attfiles = new ATTFileList();

    attfiles_area = new QScrollArea(this);
    attfiles_area->setWidgetResizable(true);
    attfiles_area->setWidget(attfiles);

    layout->addWidget(new QLabel("Select input file:"));
    layout->addWidget(inputfile);
    layout->addWidget(new QLabel("Select kernel file:"));
    layout->addWidget(kernelinfo);
    layout->addWidget(new QLabel("Select assembly file:"));
    layout->addWidget(assemblyfile);
    attfiles_label = new QLabel("Select att files:");
    layout->addWidget(attfiles_label);
    layout->addWidget(attfiles_area);

    QObject::connect(kernelinfo, &QLineEdit::editingFinished, this, &AnalyseDialog::Update);
}

void AnalyseDialog::Update() {
    std::string str = kernelinfo->displayText().toStdString();

    std::ifstream file(str.c_str(), std::fstream::in);
    QWARNING(file.good(), "Failed to open " << str, return);

    std::string kname;
    std::getline(file, kname);

    size_t commapos = kname.find(": ");
    QWARNING(commapos != std::string::npos && commapos < kname.size()-3, "Invalid kernel: " << kname, return);
    kname = kname.substr(commapos+2);
    size_t kdpos = kname.find(".kd");
    if (kdpos != std::string::npos)
        kname = kname.substr(0, kdpos);

    std::cout << "Kname: " << kname << std::endl;
    attfiles->ResetKernel(str);
}

CollectDialog::CollectDialog(const std::string& defkernel, const std::string& definput):
    AnalyseDialog(defkernel, definput) {
    this->setWindowTitle("Collect ATT data");

    attfiles->setVisible(false);
    attfiles_area->setVisible(false);
    attfiles_label->setVisible(false);
    binary = new QLineEdit();
    output_folder = new QLineEdit();

    layout->addWidget(new QLabel("Binary:"));
    layout->addWidget(binary);
    layout->addWidget(new QLabel("Output folder:"));
    layout->addWidget(output_folder);
}

InputFileEdit::InputFileEdit(InputFile& inputfile): inputfile(inputfile) {
    this->setWindowTitle("Input File Editor");
    QHBox* h_layout = new QHBox();
    this->setLayout(h_layout);

    QWidget* left_widget = new QWidget(this);
    QVBox* left_layout = new QVBox();
    left_widget->setLayout(left_layout);
    h_layout->addWidget(left_widget);

    QScrollArea* right_widget = new QScrollArea(this);
    right_widget->setWidgetResizable(true);
    h_layout->addWidget(right_widget);

    QFrame* frame = new QFrame();
    QVBox* perfcounters_layout = new QVBox();
    perfcounters_layout->setSpacing(0);
    frame->setLayout(perfcounters_layout);
    right_widget->setWidget(frame);

    target_cu = new QLineEdit(this);
    target_cu->setValidator(new QIntValidator(0, 15, this));
    target_cu->setText(std::to_string(inputfile.target_cu).c_str());
    left_layout->addWidget(new QLabel("Target CU [0,15]:"));
    left_layout->addWidget(target_cu);

    QWidget* simd_widget = new QWidget(this);
    QHBox* simd_layout = new QHBox();
    simd_layout->setSpacing(0);
    simd_widget->setLayout(simd_layout);
    left_layout->addWidget(new QLabel("SIMD Mask:"));
    left_layout->addWidget(simd_widget);

    for (int i=0; i<4; i++) {
        simd_mask.push_back(new QCheckBox(std::to_string(i).c_str()));
        simd_layout->addWidget(simd_mask.back());
        if (inputfile.simd_mask & (1<<i))
            simd_mask.back()->setChecked(true);
    }

    perfcounters_period = new QLineEdit(this);
    perfcounters_period->setValidator(new QIntValidator(-1, 31, this));
    perfcounters_period->setText(std::to_string(inputfile.counter_period).c_str());
    left_layout->addWidget(new QLabel("Perfcounters Period [-1,31]:"));
    left_layout->addWidget(perfcounters_period);

    perfcounters_layout->addWidget(new QLabel("Select counters:"));
    auto perflist = InputFile::GetAvailableCounters();

    for (const std::string& perf : perflist) {
        selected_perf[perf] = new QCheckBox(perf.c_str());
        perfcounters_layout->addWidget(selected_perf[perf]);
    }

    for (auto& selected : inputfile.counters) {
        if (selected_perf[selected])
            selected_perf[selected]->setChecked(true);
    }

    QPushButton* but_accept = new QPushButton("Save");
    QPushButton* but_reject = new QPushButton("Cancel");

    QObject::connect(but_accept, &QPushButton::clicked, this, &InputFileEdit::SaveExit);
    QObject::connect(but_reject, &QPushButton::clicked, this, &InputFileEdit::reject);

    QFrame* but_frame = new QFrame(this);
    QHBox* but_layout = new QHBox();
    but_frame->setLayout(but_layout);
    but_layout->addWidget(but_accept);
    but_layout->addWidget(but_reject);
    left_layout->addWidget(but_frame);
}

void InputFileEdit::SaveExit() {
    inputfile.target_cu = atoi(target_cu->displayText().toStdString().c_str());
    inputfile.counter_period = atoi(perfcounters_period->displayText().toStdString().c_str());

    inputfile.simd_mask = 0;
    for (int i=0; i<4; i++)
        inputfile.simd_mask |= int(simd_mask[i]->isChecked()!=0) << i;

    inputfile.counters.clear();
    for (auto& [k, v] : selected_perf) {
        if (v && v->isChecked())
            inputfile.counters.push_back(k);
    }
    accept();
}

//InputFileEdit::~InputFileEdit() {}