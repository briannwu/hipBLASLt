#pragma once
#include <QWidget>
#include "include/custom_layouts.h"

#define FONT_SIZE 5

// This class will paint arrows on top of a QCodelist
class ArrowCanvas : public QWidget {
    Q_OBJECT
    set_tracked();
public:
    ArrowCanvas() {};
    ArrowCanvas(class QWidget* parent): QWidget(parent) {};
    virtual void paintCanvas(class QCodelist* list);
    void buildConnections(const class std::vector<class WaitList>& waitcnt);

protected:
    void Connect(QCodelist* list, QPainter& painter, int l1, int l2, int xslot);

    int colorstate = 0;
    std::vector<std::tuple<int, int, int, bool>> arrows; // source, dest, slot, is_interior

private:
    int max_slot_alloc = 0;
    bool bInit = false;

public:
    static std::vector<QColor> arrow_colors;
};

class ArrowSlot : public QWidget {
    Q_OBJECT
    set_tracked();
    using Super = QWidget;
    static int min_width;
public:
    ArrowSlot(): QWidget() {};
    ArrowSlot(class QWidget* parent): QWidget(parent) {};
    virtual void mousePressEvent(class QMouseEvent *e) override;
    virtual QSize sizeHint() const override { return QSize(3*min_width/2, 1); };
    virtual QSize minimumSizeHint() const override { return QSize(min_width, 1); };
    static void SetWidthRequirements(int req) { min_width = req; }
};
