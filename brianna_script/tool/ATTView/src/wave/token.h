#pragma once

#include <QPushButton>
#include "include/custom_layouts.h"
#include "data/wavemanager.h"
#include "include/highlight.h"

//! A Token with data visualization options. TODO: Self highlight on click
class TokenData {
public:
    Token token;
    Token tip;
    int slot = 0;

    virtual std::string ToolTip() const;
    void DrawToken(QPainter& painter, int64_t begintime, bool bIsMain);

    const std::string& GetName() const { return GetName(token.type); }
    const std::string& GetStyleColor() const { return GetStyleColors(token.type); }
    const QColor& GetColor() const { return GetColor(token.type); }

    static const QColor& GetColor(int i) { return Q_COLORS[i%Q_COLORS.size()]; };
    static const std::string& GetName(int i) { return INST_TYPE[i%INST_TYPE.size()]; }
    static const std::string& GetStyleColors(int i) { return STYLE_COLORS[i%STYLE_COLORS.size()]; }

    static std::vector<std::string> INST_TYPE;
private:
    static std::vector<std::string> STYLE_COLORS;
    static std::vector<QColor> Q_COLORS;

    //static HighlightTimer timer;
};

//! A Token containing multiple collapsed tokens
class TokenArray : public TokenData {
public:
    TokenArray(const TokenData& td);
    std::array<int, 12> cycles = {};
    int max_cycles = 0;

    TokenArray& operator +=(const TokenData& td);    
    virtual std::string ToolTip() const override;
};

struct WaveState : public std::pair<int, int> {
    WaveState() = default;
    WaveState(const std::pair<int, int>& p): std::pair<int, int>(p) {};
    WaveState(int first, int second): std::pair<int, int>(first, second) {};
    void DrawState(class QPainter& painter, int64_t pos, bool bIsMain);

    const QColor& GetColor() const { return GetStateColor(this->second); }
    const std::string& GetName() const { return GetStateName(this->second); }

    static const QColor& GetStateColor(int i) { return STATE_COLORS[i%STATE_COLORS.size()]; }
    static const std::string& GetStateName(int i) { return STATE_NAMES[i%STATE_NAMES.size()]; }
private:
    static std::vector<QColor> STATE_COLORS;
    static std::vector<std::string> STATE_NAMES;
};

/*
class QToken : public QPushButton
{
    Q_OBJECT
    set_tracked();
    using Super = QPushButton;
public:
    explicit QToken(QWidget *parent, const Token& token, const std::string& tooltip, bool selfRegister = false);
    virtual ~QToken();
    virtual void mousePressEvent(class QMouseEvent *e) override;
    virtual void Highlight(bool bIntoView);
    virtual void IncrementHighlight();
    virtual void paintEvent(class QPaintEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    Token token;
    HighlightTimer timer;

    static std::vector<std::string> INST_TYPE;
    static std::vector<std::string> COLORS;
    static const std::string& GetType(int index) { return INST_TYPE[index%INST_TYPE.size()]; }
    static const std::string& GetColors(int index) { return COLORS[index%COLORS.size()]; }
private:
    bool bRegistered;
signals:
};*/
