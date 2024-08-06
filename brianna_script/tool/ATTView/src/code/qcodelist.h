#ifndef QCODELIST_H
#define QCODELIST_H

#include <QWidget>
#include "qcodeline.h"

//! A block of QCodeline. Codelines are grouped for performance reasons.
class QCodeSection : public QWidget {
    Q_OBJECT
    set_tracked();
    using Super = QWidget;
public:
    explicit QCodeSection(QWidget *parent);
    virtual ~QCodeSection();
    void AddLine(class QCodeline* line);
    class QVBox* layout;
    int num_lines = 0;

    virtual QSize sizeHint() const override { return QSize(this->Super::sizeHint().width(), LINE_HEIGHT*num_lines); };
    virtual QSize minimumSizeHint() const override { return QSize(this->Super::minimumSizeHint().width(), LINE_HEIGHT*num_lines); };
};

//! A list of QCodeline. Divided into several QCodeSection (s).
class QCodelist : public QWidget
{
    Q_OBJECT
    set_tracked();
public:
    explicit QCodelist(QWidget *parent = nullptr);
    virtual ~QCodelist();
    QCodeline* AddLine(
        int cycle,
        int index,
        const std::string& line,
        int asm_line_num,
        const std::vector<int>& cycles,
        const std::string& cppline,
        int64_t cycles_sum,
        int hitcount
    );
    void Populate(const std::string& path); ///< Receives a valid json file and populates the list.
    void Reset(); ///< Erases the list
    virtual void paintEvent(QPaintEvent* event) override;
    QPoint GetLinePos(int lineid);
    QSize GetSlotSize();

    void changeStrategy(const QString& text); ///< Sets what happens with hitcounts (sum, avg, max...)

    class ArrowCanvas* connector = nullptr;

    class QVBox* layout_main = nullptr;
    std::vector<class QCodeSection*> sections;

    std::vector<class WaitList> waitcnt;
    std::unordered_map<int, std::pair<class QCodeSection*, class QCodeline*>> lines;

    const int max_section_size = 256;
private:
    bool bInitComplete = false;
signals:
};

#endif // QCODELIST_H
