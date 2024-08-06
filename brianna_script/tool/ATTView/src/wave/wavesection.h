#pragma once

#include <QWidget>
#include "data/wavemanager.h"
#include "token.h"
#include <unordered_set>
#include <iostream>

#define NAVI_CLOCK_TO_PX 7 // One clock = 3pixels on screen

//! A Block of tokens with pre-defined size.
class QWaveSection : public QWidget {
    Q_OBJECT
    set_tracked();
    using Super = QWidget;
public:
    QWaveSection(bool bIsMain): QWaveSection(nullptr, bIsMain) {};
    virtual ~QWaveSection();
    explicit QWaveSection(QWidget *parent, bool bIsMain);
    void AddToken(const Token& token, const Token& tooltip, int slot) {
        TokenData td;
        td.token = token;
        td.tip = tooltip;
        td.slot = slot;
        token_data.push_back(td);
    };
    void AddState(int cycles, int state) { state_data.push_back({cycles, state}); };
    void Reset();

    virtual QSize sizeHint() const override { return QSize(GetTokenSize(Width), bIsMain ? 32 : 24); };
    virtual QSize minimumSizeHint() const override { return sizeHint(); };

    virtual void paintEvent(QPaintEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;

    static void ReloadRange() { SetViewRange(VIEW_MIN, VIEW_MAX, true); };
    static void SetViewRange(int vmin, int vmax, bool bForce=false);

    int TOKEN_POSY() { return 2+5*bIsMain; }
    int SLOT_OFFSET() { return 1+bIsMain; }
    int TOKEN_HEIGHT() { return 8*(2+bIsMain); }
    int WSTATE_POSY() { return TOKEN_POSY()+TOKEN_HEIGHT()+(1+bIsMain)*SLOT_OFFSET(); }
    int WSTATE_HEIGHT() { return 4+bIsMain; }
    //int64_t BeginTime() const { if (bIsVisible && token_data.size()) return token_data[0].token.clock; return -1; };
    int64_t BeginTime() const { if (token_data.size()) return token_data[0].token.clock; return -1; };

    const bool bIsMain;
    bool bIsVisible = true;
protected:
    void UpdateData();

    bool bInit = false;
    std::vector<TokenData> token_data;
    std::vector<WaveState> state_data;
    std::vector<TokenArray> binned_tokens;
    std::vector<WaveState> binned_states;

    static int VIEW_MIN;
    static int VIEW_MAX;
public:
    static void SetMip(int s, int pos = 0) {
        mipmap_level = s;
        for (auto* section : all_sections) {
            if (!section) continue;
            section->Reset();
            section->updateGeometry();
            section->repaint();
        }
    }
    static int GetMip() { return mipmap_level; }
    static int64_t PosToClock(int64_t value) {
        //return (int64_t(value*WaveManager::BaseClock()/4/2) << mipmap_level);
        return ((value/(WaveManager::bIsNaviWave?NAVI_CLOCK_TO_PX:2)) << mipmap_level);
    }
    static int64_t GetTokenSize(int64_t value) {
        //return (2*value*4/WaveManager::BaseClock() + ((1<<mipmap_level)>>1)) >> mipmap_level;
        return ((value*(WaveManager::bIsNaviWave?NAVI_CLOCK_TO_PX:2)) + ((1<<mipmap_level)>>1)) >> mipmap_level;
    }
    static Token TransformTokenSize(const Token& td) {
        Token token = td;
        token.cycles = GetTokenSize(td.cycles);
        token.clock = GetTokenSize(td.clock);
        return token;
    }
    static const int Width;

protected:
    static int mipmap_level;

private:
    static std::unordered_set<QWaveSection*> all_sections;
signals:
};
