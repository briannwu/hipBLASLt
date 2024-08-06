#pragma once
#include <vector>
#include <QDialog>
#include <QFrame>
#include <QCheckBox>
#include "include/memtracker.h"

class QAllBox: public QCheckBox {
    Q_OBJECT
    set_tracked();
public:
    QAllBox(std::vector<class QCheckBox*>& boxes);
private:
    void checkAll(int state);
    std::vector<class QCheckBox*>& boxes;
};

class CounterDialog: public QDialog {
    Q_OBJECT
    set_tracked();
public:
    CounterDialog(std::vector<bool>& cu_list, std::vector<bool>& se_list);
    virtual ~CounterDialog();

    void SelectAllSE(bool enable);
    void SelectAllCU(bool enable);
    void SaveExit();

    std::vector<bool>& cu_list;
    std::vector<bool>& se_list;

private:
    class QVBox* layout = nullptr;
    std::vector<class QCheckBox*> compute_boxes = {};
    std::vector<class QCheckBox*> shader_boxes = {};
};
