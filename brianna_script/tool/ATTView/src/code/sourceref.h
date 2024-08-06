#pragma once

#include "include/custom_layouts.h"
#include <string>
#include <memory>
#include <QScrollArea>

/* Class responsible for keeping source file records */
class SourceLoader {
    set_tracked();
    friend class SourceRef;
public:
    SourceLoader(const std::string& filename);
    ~SourceLoader();
    void SetAsCurrentSource();

    static std::string current_loaded_source;
    static void HighlightLine(int lineref);
private:
    /* A cache for source files */
    bool bValid = false;
    std::vector<std::string> sourcelines;
    std::vector<int> cyclecount;
    std::vector<int> addspaces;
    std::string file_name;
    std::string file_path;

    static std::unordered_map<std::string, std::weak_ptr<SourceLoader>> loaders;
    static std::vector<QLabel*> loaded_lines;
    static class QLabel* active_line;
};

//! Keeps a reference to a source loader. Each sourceref references a c++ line in the source code.
class SourceRef: public QObject {
    Q_OBJECT
    set_tracked();
public:
    SourceRef(const std::string& line, int cycles);
    SourceRef(const char* line, int cycles): SourceRef(std::string(line), cycles) {};
    ~SourceRef();
    bool IsValid() const { return bValid; };
    bool IsHeader() const { return bHeader; };
    const std::string& Get() const { if (IsValid()) return loader_ref->sourcelines[lineref]; else return empty_str; }
    int GetAddSpacesFromLine() const { if (IsValid()) return loader_ref->addspaces[lineref]; else return 0; }
    void LoadRefToSourceTab();
    void HighlightRef();
    void AddCyclesToRef(int cycles) { if(loader_ref) loader_ref->cyclecount[lineref] += cycles; }
private:
    int lineref = 0;
    bool bValid = false;
    bool bHeader = false;
    std::shared_ptr<SourceLoader> loader_ref;
    class QTimer* timer = nullptr;
    static std::string empty_str;

    static std::weak_ptr<SourceLoader> last_sucessful_loader;
    static int last_sucessful_ref;
    static void AddToLastLoader(int cycles);
};
