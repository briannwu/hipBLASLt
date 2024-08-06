#include "custom_layouts.h"

QVBox::~QVBox() {
    while (QLayoutItem* child = this->takeAt(0)) {
        delete child->widget();
        delete child;
    }
}
QHBox::~QHBox() {
    while (QLayoutItem* child = this->takeAt(0)) {
        delete child->widget();
        delete child;
    }
}

QBox::~QBox() {
    while (QLayoutItem* child = this->takeAt(0)) {
        delete child->widget();
        delete child;
    }
}

int MemTracker::count = 0;
std::unordered_map<std::string, int> MemTracker::classes;
