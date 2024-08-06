#pragma once

#include <vector>
#include <QDialog>
#include <QFrame>
#include <string>
#include <unordered_map>

class ATTFileList: public QFrame {
    Q_OBJECT
public:
    ATTFileList() {};
    void ResetKernel(const std::string& dir);
    virtual QSize sizeHint() const override { return QSize(512, 512); };
    std::vector<std::string> GetATTFileNames();

private:
    std::vector<class QCheckBox*> list;
};

class AnalyseDialog: public QDialog {
    Q_OBJECT
public:
    AnalyseDialog(const std::string& defkernel, const std::string& definput);
    //~AnalyseDialog();
    void Update();
protected:
    class QLineEdit* kernelinfo;
    class QLineEdit* inputfile;
    class QLineEdit* assemblyfile;
    class ATTFileList* attfiles;
    class QVBoxLayout* layout;
    class QScrollArea* attfiles_area;
    class QLabel* attfiles_label;
};

class CollectDialog: public AnalyseDialog {
    Q_OBJECT
public:
    CollectDialog(const std::string& defkernel, const std::string& definput);

protected:
    class QLineEdit* output_folder;
    class QLineEdit* binary;
};

class InputFileEdit: public QDialog {
    Q_OBJECT
public:
    InputFileEdit(class InputFile& inputfile);
    //virtual ~InputFileEdit();
    void SaveExit();
private:
    std::vector<QCheckBox*> simd_mask;
    class QLineEdit* target_cu;
    class QLineEdit* perfcounters_period;
    std::unordered_map<std::string, class QCheckBox*> selected_perf;

    class InputFile& inputfile;
};
