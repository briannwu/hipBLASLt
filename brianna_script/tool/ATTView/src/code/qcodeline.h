#ifndef QCODELINE_H
#define QCODELINE_H

#include <unordered_map>
#include <vector>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include "include/custom_layouts.h"
#include "include/highlight.h"
#include "sourceref.h"

#define COUNT_SIZEHINT 144
#define CPP_SIZEHINT 320
#define ASM_SIZEHINT 384

#define LINE_HEIGHT 20
#define CODE_FONT 11

/** 
 *  Widget that displays information about an assembly instruction.
 *  To be used as a element of QCodelist.
 */
class QCodeline : public QWidget {
    Q_OBJECT
    set_tracked();
    using Super = QWidget;
public:
    explicit QCodeline(
        Super* parent,
        int wave_id,
        int index,
        const std::string& line,
        int line_num,
        const std::vector<int>& cycles,
        const std::string& cppline,
        int64_t cycles_sum,
        int hitcount
    );
    virtual ~QCodeline();
    virtual void Highlight(bool bIntoView);

    int type;
    int wave_id;
    class ArrowSlot* aslot;     //!< Widget so canvas can find the position of this line, for drawing
    class QASMLine* line;
    class QCppLabel* cppline;
    //class QHitcountLabel* hitcount;
    class CyclesLabel* cycles;
    const int line_map_index;               //!< Entry in the line_map
    class QHBox* hlayout = nullptr;

    static std::unordered_map<int, QCodeline*> line_map;    //!< Dictrionary of all codelines, so tokens can
                                                            //!< reference this widget using the line number
    static QCodeline* GetLine(int line_number, int wave_id) {
        QCodeline* line = line_map[line_number];
        if (!line || line->wave_id != wave_id)
            return nullptr;
        return line;
    };
    static QCodeline* GetLine(int line_number) {
        return line_map[line_number];
    };
    static bool HighlightLine(int line_number, bool bIntoView) {
        QCodeline* line = GetLine(line_number);
        if (line)
            line->Highlight(bIntoView);
        return line != nullptr;
    };
    void CreateLayout();

    static bool InsertTokenAtLine(int line_number, int wave_id, class QToken* token) { return false; };
    static bool RemoveTokenAtLine(int line_number, int wave_id, class QToken* token) { return false; };

    virtual void paintEvent(class QPaintEvent* event) override;
    //virtual void mouseMoveEvent(class QMouseEvent* event) override;
    virtual void EnableQTLayout();

    virtual QSize sizeHint() const override { return QSize(this->Super::sizeHint().width(), LINE_HEIGHT); };
    virtual QSize minimumSizeHint() const override { return QSize(this->Super::sizeHint().width(), LINE_HEIGHT); };
    virtual void leaveEvent(QEvent* event) override;
    virtual void enterEvent(QEvent* event) override;

    static QCodeline* firstline;
    bool bUnderMouse = false;
signals:
};

class CustomTextWidget: public QWidget {
    Q_OBJECT
public:
    CustomTextWidget() = default;
    CustomTextWidget(const char* txt): str(txt ? txt : "") {};
    virtual ~CustomTextWidget() { if (functional_element) delete functional_element; }

    virtual void setText(const char* txt) { str = txt ? txt : ""; };
    virtual void paint(class QPainter& painter, int posy) = 0;
    virtual void CreateFuncElement();
    virtual void DeleteFuncElement() { if (functional_element) delete functional_element; functional_element = nullptr; }

    std::string str = "";
    class QWidget* functional_element = nullptr;
};

//! A text widget containing an assembly instruction
class QASMLine: public CustomTextWidget {
    Q_OBJECT
    set_tracked();
    using Super = CustomTextWidget;
public:
    QASMLine(const std::string& line, int asm_line, int code_line, QCodeline* parent);
    virtual void Highlight(bool bFast);

    virtual void paintEvent(class QPaintEvent* event) override {};
    virtual QSize sizeHint() const override { return QSize(ASM_SIZEHINT, LINE_HEIGHT); };
    virtual QSize minimumSizeHint() const override { return QSize(ASM_SIZEHINT, LINE_HEIGHT); };
    virtual void paint(class QPainter& painter, int posy) override;
private:
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void IncrementHighlight();
    HighlightTimer timer;
    QCodeline* codeline_parent;
    Color highlightColor;
    float highlightSpeed;
    const int code_line;
};

//! A text widget containing a reference to the original source file
class QCppLabel : public CustomTextWidget {
    Q_OBJECT
    set_tracked();
    using Super = CustomTextWidget;
public:
    QCppLabel(const std::string& name, int cycles);
    virtual QSize sizeHint() const override { return QSize(CPP_SIZEHINT, LINE_HEIGHT); };
    virtual QSize minimumSizeHint() const override { return QSize(CPP_SIZEHINT, LINE_HEIGHT); };
    void customMousePress(bool);
    static void ResetCppFile();     /**< Needs to be called when loading a new cpp file */

    virtual void paint(class QPainter& painter, int posy) override;
    virtual void CreateFuncElement() override;
    SourceRef& GetSourceRef() { return sourceref; }
protected:
    class SourceRef sourceref;      /**< Reference to a cpp source file */
private:
    std::string tooltip;
};

//! A text widget containing the number of hits a instruction received
class QHitcountLabel : public CustomTextWidget {
    Q_OBJECT
    set_tracked();
    using Super = CustomTextWidget;
public:
    QHitcountLabel(const char* name): Super(name) {}

    virtual QSize sizeHint() const override { return QSize(COUNT_SIZEHINT, LINE_HEIGHT); };
    virtual QSize minimumSizeHint() const override { return QSize(COUNT_SIZEHINT, LINE_HEIGHT); };
    virtual void paint(class QPainter& painter, int posy) override;
};

//! A text widget containing the cycles used for a particular instruction.
class CyclesLabel: public QHitcountLabel {
    Q_OBJECT
    using Super = QHitcountLabel;
public:
    enum class Strategy {
        SUM_ALL,    ///< Sum over all waves
        MEAN_ALL,   ///< Mean over all waves
        SUM,        ///< Sum over this wave
        MEAN,       ///< Mean over this wave
        MAX,        ///< Max over this wave
        FIRST,      ///< First iteration of this wave
        LAST        ///< Last iteration of this wave
    };

    CyclesLabel(const std::vector<int>& cycles, int64_t cycles_sum, int hitcount): Super(""),
        cycles(cycles), all_hitcount(hitcount), all_cycles_sum(cycles_sum) { setStrategy("SUM_ALL"); };
    void setStrategy(const QString& strategy);
    void setStrategy(Strategy strategy);    ///< Sets which operation to be performed
                                            ///< on the cycles list, for displaying.
    virtual void paint(class QPainter& painter, int posy) override;

private:
    int all_hitcount;
    int64_t all_cycles_sum;
    std::vector<int> cycles;
};

#endif // QCODELINE_H
