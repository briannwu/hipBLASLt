#pragma once

#include <QDialog>

class OptionsDialogH: public QDialog {
    Q_OBJECT
public:
    OptionsDialogH();
protected:
    class QLineEdit* hotspot_bins;
    class QLineEdit* hotspot_begin;
    class QLineEdit* hotspot_end;
    class QCheckBox* hotspot_waitcnt;
    class QCheckBox* hotspot_sebox;
    void SaveExit();
};
