#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include "include/custom_layouts.h"
#include <vector>

std::pair<std::string, std::string> ParseArgs(int argc, char** argv) {
    if (argc < 2 || argv[1] == nullptr)
        return {"", ""};

    std::string ui_dir(argv[1]);
    std::string project_dir((argc > 2) ? argv[2] : argv[1]);
    return {ui_dir, project_dir};
}

int main(int argc, char *argv[])
{
    int res;
    {
        QApplication a(argc, argv);
        MainWindow w(ParseArgs(argc, argv));
        w.show();
        res = a.exec();
    }
    MemTracker::Dump();
    return res;
}
