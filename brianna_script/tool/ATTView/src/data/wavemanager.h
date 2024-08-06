#ifndef WAVEMANAGER_H
#define WAVEMANAGER_H

#include <string>
#include <vector>
#include "include/custom_layouts.h"

struct Token {
    int64_t clock;
    int type;
    int cycles;
    int code_line;
    int wave_id;
    int64_t end_time() { return clock+cycles; }
};

struct CodeData {
    int index;
    int type;
    int wave_id;
    int asm_line_num;
    int64_t cycles_sum;
    int hitcount;
    std::vector<int64_t> clock;
    std::vector<int> cycles;
    std::string inst;
    std::string cppline;
};

struct WaitList {
    int code_line;
    std::vector<std::pair<int, int>> sources;
};

struct WaveInfo {
    std::string name;
    int64_t value;
    int64_t stalls;
};

//! Interface for obtaining parsed wavedata json files. Uses WaveReader as a cache.
class WaveManager {
    set_tracked();
public:
    WaveManager(const std::string& path);
    WaveManager(const std::vector<std::string>& paths, int64_t begintime, int64_t endtime);
    virtual ~WaveManager();
    virtual const std::vector<CodeData>& GetCode() const { return *code_data; }
    virtual const std::vector<Token>& GetTokens() const { return *tokens; }
    virtual const std::vector<std::pair<int, int>>& GetTimeline() const { return *timeline; }
    virtual const std::vector<WaitList>& GetWaitcnt() const { return *waitcnt; }
    virtual const std::vector<WaveInfo>& GetWaveInfo() const { return *wave_info; }
    virtual int64_t WaveBegin() const { return wbegin; }
    virtual int64_t WaveEnd() const { return wend; }

    static int64_t global_begin_time;
    static bool bIsNaviWave;
    static int64_t BaseClock() { return bIsNaviWave ? 1 : 4; };
protected:
    const std::vector<CodeData>* code_data;
    const std::vector<Token>* tokens;
    const std::vector<std::pair<int, int>>* timeline;
    const std::vector<WaitList>* waitcnt;
    const std::vector<WaveInfo>* wave_info;
    int64_t wbegin, wend;
    bool bIsExtTimeline = false;
};

#endif // WAVEMANAGER_H
