#pragma once
#include <QColor>

//! A QColor with common math operations
class Color : public QColor {
public:
    Color() = default;
    Color(int r, int g, int b): QColor(clip(r),clip(g),clip(b)) {};
    Color(const QColor& c): QColor(c) {};
    Color& operator=(const QColor& o) { setRed(o.red()); setGreen(o.green()); setBlue(o.blue()); return *this; }
    Color& operator*=(float f) { setRed(clip(red()*f)); setGreen(clip(green()*f)); setBlue(clip(blue()*f)); return *this; }
    Color& operator/=(float f) { *this *= (1.0f/f); return *this; }
    Color& operator*=(const Color& o) { setRed(clip(red()*o.red())); setGreen(clip(green()*o.green())); setBlue(clip(blue()*o.blue())); return *this; }
    Color& operator+=(const Color& o) { setRed(clip(red()+o.red())); setGreen(clip(green()+o.green())); setBlue(clip(blue()+o.blue())); return *this; }
    
    Color operator*(float f) { Color color = *this; color*=f; return color; }
    Color operator/(float f) { Color color = *this; color/=f; return color; }
    Color operator*(const Color& o) { Color color = *this; color*=o; return color; }
    Color operator+(const Color& o) { Color color = *this; color+=o; return color; }
    
    static float clip(float f) { return std::max(0.0f, std::min(255.0f, f)); }
};

//! A QTimer used for highlighting (token, lines)
class HighlightTimer {
public:
    virtual void Highlight();
    virtual void IncrementHighlight(float amount = 0.02f);
    virtual ~HighlightTimer();
    class QTimer* timer = nullptr;
    float vis = 1.0f;
};
