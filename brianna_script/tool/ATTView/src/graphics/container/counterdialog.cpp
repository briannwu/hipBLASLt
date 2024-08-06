#include "counterdialog.h"
#include <QCheckBox>
#include <QFrame>
#include "include/custom_layouts.h"
#include <QLabel>
#include <QPushButton>

#define NUM_CUS 16

QAllBox::QAllBox(std::vector<class QCheckBox*>& _boxes): QCheckBox("All"), boxes(_boxes) {
    connect(this, &QCheckBox::stateChanged, this, &QAllBox::checkAll);
}

void QAllBox::checkAll(int state) {
    for (QCheckBox* box : boxes) {
        if (box)
            box->setChecked(state!=0);
    }
}

CounterDialog::CounterDialog(std::vector<bool>& cu_list, std::vector<bool>& se_list):
    cu_list(cu_list), se_list(se_list), QDialog() {
    this->setWindowTitle("Compute Unit and Shader Filter");
    this->layout = new QVBox(this);

    QFrame* cu_frame = new QFrame(this);
    cu_frame->setLayout(new QVBox());
    QFrame* cu_widget = new QFrame(cu_frame);
    QBox* cu_layout = new QBox();
    cu_widget->setLayout(cu_layout);

    cu_frame->layout()->addWidget(new QLabel("Select compute units:"));
    cu_frame->layout()->addWidget(cu_widget);

    QFrame* se_frame = new QFrame(this);
    se_frame->setLayout(new QVBox());
    QFrame* se_widget = new QFrame(se_frame);
    QBox* se_layout = new QBox();
    se_widget->setLayout(se_layout);

    se_frame->layout()->addWidget(new QLabel("Select shader engines:"));
    se_frame->layout()->addWidget(se_widget);

    for (int j=0; j<NUM_CUS/4; j++) {
        for (int i=0; i<4; i++) {
            if (4*j+i >= cu_list.size()) break;

            QCheckBox* box = new QCheckBox(std::to_string(j*4+i).c_str());
            box->setChecked(cu_list[4*j+i]);

            cu_layout->addWidget(box, j+1, i);
            compute_boxes.push_back(box);
        }
    }
    cu_layout->addWidget(new QAllBox(compute_boxes), 0, 0);

    for (int j=0; j<se_list.size(); j++) {
        for (int i=0; i < 6; i++) {
            if (6*j+i >= se_list.size()) break;

            QCheckBox* box = new QCheckBox(std::to_string(j*6+i).c_str());
            box->setChecked(se_list[6*j+i]);

            se_layout->addWidget(box, j+1, i);
            shader_boxes.push_back(box);
        }
    }
    se_layout->addWidget(new QAllBox(shader_boxes), 0, 0);

    this->layout->addWidget(cu_frame);
    this->layout->addWidget(se_frame);

    QPushButton* but_accept = new QPushButton("Save");
    QPushButton* but_reject = new QPushButton("Cancel");

    QObject::connect(but_accept, &QPushButton::clicked, this, &CounterDialog::SaveExit);
    QObject::connect(but_reject, &QPushButton::clicked, this, &CounterDialog::reject);

    QFrame* but_frame = new QFrame(this);
    QHBox* but_layout = new QHBox();
    but_frame->setLayout(but_layout);
    but_layout->addWidget(but_accept);
    but_layout->addWidget(but_reject);
    this->layout->addWidget(but_frame);
}

CounterDialog::~CounterDialog() {
    if (this->layout)
        delete this->layout;
};

void CounterDialog::SaveExit() {
    for (int i=0; i<compute_boxes.size() && i<cu_list.size(); i++)
        cu_list[i] = compute_boxes[i]->isChecked();

    for (int i=0; i<shader_boxes.size() && i<se_list.size(); i++)
        se_list[i] = shader_boxes[i]->isChecked();

    accept();
}
