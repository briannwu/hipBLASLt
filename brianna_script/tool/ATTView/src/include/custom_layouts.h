#pragma once
#include <QtCore>
#include <QBoxLayout>
#include <QRadioButton>
#include "memtracker.h"

//! A QVBoxLayout that force deletes it's widgets on destruct
class QVBox : public QVBoxLayout {
    Q_OBJECT
    set_tracked();
public:
    QVBox(): QVBoxLayout() {}
    QVBox(QWidget* w): QVBoxLayout(w) {}
    virtual ~QVBox(); 
};

//! A QHBoxLayout that force deletes it's widgets on destruct
class QHBox : public QHBoxLayout {
    Q_OBJECT
    set_tracked();
public:
    QHBox(): QHBoxLayout() {}
    QHBox(QWidget* w): QHBoxLayout(w) {}
    virtual ~QHBox(); 
};

//! A QGridLayout that force deletes it's widgets on destruct
class QBox : public QGridLayout {
    Q_OBJECT
    set_tracked();
public:
    QBox(): QGridLayout() {}
    QBox(QWidget* w): QGridLayout(w) {}
    virtual ~QBox(); 
};

class QSelButton: public QRadioButton {
    Q_OBJECT
    set_tracked();
public:
    QSelButton(const QString& str, QWidget* parent): QRadioButton(str, parent) {};
};
