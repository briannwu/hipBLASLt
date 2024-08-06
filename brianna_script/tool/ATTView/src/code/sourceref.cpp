#include <fstream>
#include <unordered_map>
#include "mainwindow.h"
#include "sourceref.h"
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QScrollBar>

/* A cache for source files */
std::unordered_map<std::string, std::weak_ptr<SourceLoader>> SourceLoader::loaders;
std::string SourceRef::empty_str = "";

std::weak_ptr<SourceLoader> SourceRef::last_sucessful_loader{};
int SourceRef::last_sucessful_ref = 0xFFFFFFF;

std::string SourceLoader::current_loaded_source = "";
std::vector<QLabel*> SourceLoader::loaded_lines = {};
QLabel* SourceLoader::active_line = nullptr;

SourceLoader::SourceLoader(const std::string& filestr) {
    std::string project_dir = MainWindow::window->GetProjectDir();
    std::string dir = project_dir;
    if (filestr.size() && filestr[0] == '/')
        dir = "";

    file_name = filestr;
    file_path = dir+filestr;

    /*std::cout << "dir 0:" << project_dir << std::endl;
    std::cout << "file_name 0:" << file_name << std::endl;
    std::cout << "file_path 0:" << file_path << std::endl; */

    size_t split_pos = project_dir.find(':');
    if (split_pos != std::string::npos && split_pos+1 < project_dir.size())
    {
        std::string replace = project_dir.substr(0, split_pos);
        dir = project_dir.substr(split_pos+1);

        if (filestr.find(replace) != std::string::npos)
            file_path = dir + file_path.substr(filestr.find(replace)+replace.size()+1);
    }

    /* std::cout << "dir 1:" << dir << std::endl;
    std::cout << "file_name 1:" << file_name << std::endl;
    std::cout << "file_path 1:" << file_path << std::endl; */

    std::ifstream file(file_path, std::fstream::in);

    this->addspaces = {0};
    this->sourcelines = {""};
    this->cyclecount = {0};

    while (file.good()) {
        std::string ss;
        std::getline(file, ss);

        int spaces = 0;
        int tabs = 0;
        while (tabs < ss.size()-1 && ss[tabs] == '\t') tabs ++;
        while (spaces < ss.size() && ss[spaces] == ' ') spaces ++;
        spaces += 8*tabs;

        this->sourcelines.push_back(std::string(spaces, ' ')+ss.substr(tabs));
        this->cyclecount.push_back(0);
        this->addspaces.push_back(spaces-tabs);
    }
    this->bValid = sourcelines.size() > 1;
    std::cout << "New SourceLoader: " << file_path << " size: " << sourcelines.size() << std::endl;
}

SourceLoader::~SourceLoader() {
    std::cout << "SourceLoader destructed: " << file_path << std::endl;
    loaders.erase(file_name);
}

void SourceLoader::SetAsCurrentSource() {

    class QWidget* tab = MainWindow::window->source_tab;
    QWARNING(tab, "code tab not available", return);

    current_loaded_source = file_path;
    loaded_lines = {};
    active_line = nullptr;
    
    if (tab->layout())
        delete tab->layout();

    QBox* tablayout = new QBox();
    tab->setLayout(tablayout);
    tablayout->setSpacing(0);
    tablayout->setHorizontalSpacing(0);


    loaded_lines.push_back(new QLabel("<b>Cycles Count</b>"));
    tablayout->addWidget(new QLabel(""), 0, 0, Qt::AlignLeft);
    tablayout->addWidget(loaded_lines.back(), 0, 1, Qt::AlignLeft);
    tablayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, 2);

    int zfill = std::to_string(sourcelines.size()).size();
    for (int i=1; i<sourcelines.size(); i++) {
        std::string cc = std::to_string(i);
        int zf = zfill - cc.size();
        while (zf > 0) {
            zf --;
            cc = " &nbsp; " + cc;
        }
        cc += " | clk: <b>" + std::to_string(cyclecount[i]);
        cc += "</b> &nbsp; ";

        loaded_lines.push_back(new QLabel(sourcelines[i].c_str()));
        loaded_lines.back()->setTextFormat(Qt::PlainText);
        loaded_lines.back()->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        tablayout->addWidget(loaded_lines.back(), i, 1, Qt::AlignLeft);

        tablayout->addWidget(new QLabel(cc.c_str()), i, 0, Qt::AlignLeft);
    }
    tab->setStyleSheet("QLabel {background-color: #F0F0F0}");
}

void SourceLoader::HighlightLine(int lineref) {
    QWARNING(lineref < loaded_lines.size(), "Invalid source line", return);

    if (active_line)
        active_line->setStyleSheet("QLabel {background-color: #E0E0F0}");

    active_line = loaded_lines[lineref];
    active_line->setStyleSheet("QLabel {background-color: #80F080}");
}

SourceRef::SourceRef(const std::string& line, int cycles) {
    size_t pos1 = line.find(':');
    if (pos1 == std::string::npos || pos1 >= line.size()-2 || pos1 < 3) {
        AddToLastLoader(cycles);
        return;
    }

    size_t pos2 = line.find(':', pos1+2);

    std::string filestr = line.substr(0, pos1);
    auto element = SourceLoader::loaders.find(filestr);
    if (element == SourceLoader::loaders.end()) {
        loader_ref = std::make_shared<SourceLoader>(filestr);
        SourceLoader::loaders[filestr] = loader_ref;
    } else {
        loader_ref = std::shared_ptr<SourceLoader>(element->second);
    }
    if (!loader_ref->bValid)
        return;

    QWARNING(loader_ref->file_name == filestr, "Something went wrong with " << filestr, return);

    lineref = atoi(line.substr(pos1+1,pos2-pos1-1).c_str());
    if (lineref <= 0 || lineref >= loader_ref->sourcelines.size()) return;

    loader_ref->cyclecount[lineref] += cycles;
    last_sucessful_loader = loader_ref;
    last_sucessful_ref = lineref;
    bValid = true;
    // TODO: Check for system header
    //bHeader = (line.substr(pos1-2,2) == ".h") || (pos1 > 4 && line.substr(pos1-4,4) == ".hpp");
}

void SourceRef::AddToLastLoader(int cycles) {
    if (std::shared_ptr<SourceLoader> loader = last_sucessful_loader.lock()) {
        if (last_sucessful_ref < loader->cyclecount.size())
        loader->cyclecount[last_sucessful_ref] += cycles;
    }
}

void SourceRef::LoadRefToSourceTab() {
    if (!IsValid()) return;

    if (SourceLoader::current_loaded_source != loader_ref->file_path)
        loader_ref->SetAsCurrentSource();

    MainWindow::window->source_tab_parent->setCurrentIndex(3);

    SourceLoader::HighlightLine(this->lineref);
    //scroll->ensureWidgetVisible(active_line);
    if (!timer) {;
        timer = new QTimer();
        timer->setSingleShot(true);
        timer->setInterval(1);
        QObject::connect(timer, &QTimer::timeout, this, &SourceRef::HighlightRef);
    }
    timer->start();
};

void SourceRef::HighlightRef() {
    timer->stop();
    QWARNING(MainWindow::window->source_scrollArea, "No scroll area", return);

    if (!SourceLoader::active_line) return;

    MainWindow::window->source_scrollArea->ensureWidgetVisible(SourceLoader::active_line);
    MainWindow::window->source_scrollArea->horizontalScrollBar()->setValue(0);
}

SourceRef::~SourceRef() {
    if (timer) delete timer;
}
