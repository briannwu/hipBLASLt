#include "wavemanager.h"
#include "wavedata.h"
#include <set>

int64_t WaveManager::global_begin_time = 0;
bool WaveManager::bIsNaviWave = false;

WaveManager::WaveManager(const std::string& path) {
    const WaveReader* reader = WaveReader::Get(path);
    this->code_data = &(reader->code_data);
    this->tokens = &(reader->tokens);
    this->timeline = &(reader->timeline);
    this->waitcnt = &(reader->waitcnt);
    this->wave_info = &(reader->wave_info);
    this->wbegin = reader->wbegin;
    this->wend = reader->wend;
}

struct sort_string {
    bool operator()(const std::string& a, const std::string& b) const
    {
        size_t i=0, j=0;
        while (i < a.size()-1 && j < b.size()-1)
        {
            while(i < a.size()-1 && (a[i] < '0' || a[i] > '9')) i++;
            while(j < b.size()-1 && (b[j] < '0' || b[j] > '9')) j++;

            int u = atoi(a.c_str()+i);
            int v = atoi(b.c_str()+j);

            if (u != v)
                return u < v;

            while(i < a.size()-1 && a[i] >= '0' && a[i] <= '9') i++;
            while(j < b.size()-1 && b[j] >= '0' && b[j] <= '9') j++;
        }
        return false;
    }
};

WaveManager::WaveManager(
    const std::vector<std::string>& unordered_paths,
    int64_t begintime,
    int64_t endtime
) : WaveManager(unordered_paths.size() ? unordered_paths[0] : "")
{
    this->wbegin = begintime;
    this->wend = endtime;

    assert(unordered_paths.size());
    bIsExtTimeline = true;

    std::vector<Token>* ext_tokens = new std::vector<Token>();
    std::vector<std::pair<int, int>>* ext_timeline = new std::vector<std::pair<int, int>>();

    int64_t current_state = begintime;
    std::set<std::string, sort_string> paths(unordered_paths.begin(), unordered_paths.end());

    for (auto& path_name : paths) {
        WaveManager manager(path_name);

        if (manager.WaveEnd() < begintime) continue;
        if (manager.WaveBegin() >= endtime) break;

        for (Token token : manager.GetTokens()) {
            if (token.clock > endtime) break;
            if (token.clock + token.cycles <= begintime) continue;

            if (token.clock < begintime) {
                token.cycles -= begintime - token.clock;
                token.clock = begintime;
            }
            if (token.clock + token.cycles > endtime)
                token.cycles = endtime - token.clock;

            ext_tokens->push_back(token);
        }

        if (manager.WaveBegin() > current_state)
            ext_timeline->push_back({0, manager.WaveBegin()-current_state});
        current_state = manager.WaveBegin();

        for (std::pair<int, int> t : manager.GetTimeline()) {
            if (current_state >= endtime)
                break;
            if (t.second + current_state > endtime)
                t.second = endtime - current_state;
            if (current_state+t.second < begintime) {
                current_state += t.second;
                continue;
            }
            if (current_state < begintime) {
                t.second -= begintime - current_state;
                current_state = begintime;
            }

            if (t.second <= 0) continue;
            ext_timeline->push_back(t);
            current_state += t.second;
        }
    }

    this->tokens = ext_tokens;
    this->timeline = ext_timeline;
}

WaveManager::~WaveManager() {
    if (!bIsExtTimeline)
        return;

    if (timeline)
        delete timeline;
    if (tokens)
        delete tokens;
}