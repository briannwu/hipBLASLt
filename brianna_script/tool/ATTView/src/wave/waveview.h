#pragma once

#include <QWidget>
#include <QBoxLayout>
#include "data/wavemanager.h"
#include "token.h"
#include "wavesection.h"

//! A timeline view of a waveslot[0,9] within a SIMD.
class QWaveView : public QWidget {
    Q_OBJECT
    set_tracked();
    using Super = QWidget;
public:
    explicit QWaveView(QWidget *parent, class QScrollArea* sarea);
    virtual ~QWaveView();
    void Populate(const WaveManager& manager);
    void Reset();
    void AddToken(Token token, int slot);
    void AddTimeline(int64_t clock, int cycles, int state);
    void SetAsMain() { bIsMain = true; mainview = this; }

    static void HighlightTokenFromLine(int code_line);
    static QWaveView& GetMainView() { QASSERT(mainview, "Empty mainview");  return *mainview; }

    int64_t begintime = 0;
    int64_t endtime = 1;
private:
    virtual void paintEvent(class QPaintEvent* event) override;
    int64_t VisibleBeginTime() const;

    class QHBox* layout = nullptr;
    std::vector<class QWaveSection*> sections;
    std::vector<int> section_begintime;

    // TODO: This needs to map to a ordered_set/ordered vector
    std::unordered_map<int, std::vector<int64_t>> line_to_clock;
    bool bIsMain = false;
    int64_t cycles_per_section = 1;
    static QWaveView* mainview;
signals:
};

//! A collection of QWaveView (s). Used by SIMD/CU View.
class QWaveSlots : public QWidget {
    Q_OBJECT
    set_tracked();
public:
    QWaveSlots(): QWaveSlots(nullptr) {};
    explicit QWaveSlots(QWidget *parent);
    void AddSlot(QWaveView* view, const std::string& name);
    void Reset();

    std::vector<QWaveView*> waves;
    class QVBox* layout = nullptr;
signals:
};

