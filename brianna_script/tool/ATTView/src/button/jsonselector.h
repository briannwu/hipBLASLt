#pragma once

#include <memory>
#include <QComboBox>
#include "include/custom_layouts.h"
#include "include/json.hpp"
using json = nlohmann::json;

//! A combobox selector class for selecting json files.
class Selector: public QComboBox {
    Q_OBJECT
    set_tracked();
public:
    Selector(json& data, class QLayout* w, Selector* sel_parent):
        QComboBox(nullptr), selector_parent(sel_parent), ui_elem(w), data_json(data) {
        w->addWidget(this);
        addSortedNames();
    }
    virtual ~Selector(){ if (selection) delete selection; }

    void addSortedNames();
    std::vector<std::string> names; ///< Possible (partial) names for json files
    json data_json;
    class QWidget* selection = nullptr;
    class QLayout* ui_elem = nullptr; ///< Layout for combobox in mainwindow.ui
    class Selector* selector_parent; ///< Selector up in the selection chain
};

//! A combobox for selecting the target shader engine
class SESelector: public Selector {
    Q_OBJECT
public:
    SESelector(class JsonRequest& request);
    virtual ~SESelector() {}
    void textChanged(const QString &text);
signals:
};

//! A combobox for selecting the target SIMD unit
class SimdSelector: public Selector {
    Q_OBJECT
public:
    SimdSelector(json& data, SESelector* parent);
    virtual ~SimdSelector() {}
    void textChanged(const QString &text);
};

//! A combobox for selecting the target wave slot
class WSlotSelector: public Selector {
    Q_OBJECT
public:
    WSlotSelector(json& data, SimdSelector* parent);
    virtual ~WSlotSelector() {};
    void textChanged(const QString &text);
};

//! A combobox for selecting the target wave id
class WaveIDSelector: public Selector {
    Q_OBJECT
public:
    WaveIDSelector(json& data, WSlotSelector* parent);
    virtual ~WaveIDSelector();
    void textChanged(const QString &text);
    std::string GetJsonPath(const std::string& waveslot) const;
};
