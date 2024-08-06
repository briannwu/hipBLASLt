#include "options.h"
#include <QWidget>
#include "mainwindow.h"
#include <QLineEdit>
#include <QLabel>
#include <QIntValidator>
#include <QPushButton>
#include <QFrame>
#include <string>
#include <QCheckBox>

OptionsDialogH::OptionsDialogH() {
    this->setWindowTitle("Options");
    QVBox* layout = new QVBox();
    this->setLayout(layout);

    layout->addWidget(new QLabel("Number of bins on hotspot view:"));
    hotspot_bins = new QLineEdit(this);
    hotspot_bins->setValidator(new QIntValidator(4, 100, this));
    hotspot_bins->setText(std::to_string(MainWindow::window->GetHotspotBins()).c_str());
    layout->addWidget(hotspot_bins);

    layout->addWidget(new QLabel("Code line range for hotspot:"));
    QWidget* rangewidget = new QWidget(this);
    QHBox* rangebox = new QHBox();
    rangewidget->setLayout(rangebox);
    layout->addWidget(rangewidget);

    hotspot_begin = new QLineEdit(this);
    hotspot_begin->setValidator(new QIntValidator(0, INT_MAX, this));
    hotspot_begin->setText(std::to_string(MainWindow::window->hotspot_begin).c_str());

    hotspot_end = new QLineEdit(this);
    hotspot_end->setValidator(new QIntValidator(1, INT_MAX, this));
    hotspot_end->setText(std::to_string(MainWindow::window->hotspot_end).c_str());

    rangebox->addWidget(hotspot_begin);
    rangebox->addWidget(hotspot_end);

    hotspot_waitcnt = new QCheckBox("Attribute waitcnt to memory ops");
    hotspot_waitcnt->setChecked(MainWindow::window->bHospotTrackWaitcnt);
    layout->addWidget(hotspot_waitcnt);

    hotspot_sebox = new QCheckBox("Gather all waves from shader engine");
    hotspot_sebox->setChecked(MainWindow::window->bHotspotGatherAllWaves);
    layout->addWidget(hotspot_sebox);

    QPushButton* but_accept = new QPushButton("Save");
    QPushButton* but_reject = new QPushButton("Cancel");

    QObject::connect(but_accept, &QPushButton::clicked, this, &OptionsDialogH::SaveExit);
    QObject::connect(but_reject, &QPushButton::clicked, this, &OptionsDialogH::reject);

    QFrame* but_frame = new QFrame(this);
    QHBox* but_layout = new QHBox();
    but_frame->setLayout(but_layout);
    but_layout->addWidget(but_accept);
    but_layout->addWidget(but_reject);
    layout->addWidget(but_frame);
}

void OptionsDialogH::SaveExit() {
    MainWindow::window->bHospotTrackWaitcnt = hotspot_waitcnt->isChecked();
    MainWindow::window->bHotspotGatherAllWaves = hotspot_sebox->isChecked();
    MainWindow::window->hotspot_n_bins = atoi(hotspot_bins->displayText().toStdString().c_str());
    MainWindow::window->hotspot_begin = atoi(hotspot_begin->displayText().toStdString().c_str());
    MainWindow::window->hotspot_end = atoi(hotspot_end->displayText().toStdString().c_str());

    MainWindow::window->AddHotspot();
    accept();
}