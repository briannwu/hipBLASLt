#include "highlight.h"
#include <iostream>
#include <QTimer>
#include <QWidget>
#include "include/custom_layouts.h"

void HighlightTimer::Highlight() {

    if (vis < 1) vis -= 0.5f;
    else vis = 0.0f;

    if (timer != nullptr) return;

    timer = new QTimer();
    timer->setSingleShot(false);
    timer->setInterval(33);
    timer->start();
}

void HighlightTimer::IncrementHighlight(float amount) {
    this->vis += amount;
    if (this->vis < 1) return;

    QWARNING(timer, "Empty timer!", return);

    timer->stop();
    delete timer;
    timer = nullptr;
}

HighlightTimer::~HighlightTimer() {
    if (timer) delete timer;
    timer = nullptr;
}