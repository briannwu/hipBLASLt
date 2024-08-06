#include "wavedata.h"
#include <vector>
#include "include/json.hpp"
#include <cxxabi.h>
#include <fstream>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

std::unordered_map<std::string, WaveReader*> WaveReader::reader_cache;
std::string WaveReader::code_path_cache;
nlohmann::json WaveReader::code_cache;

const WaveReader* WaveReader::Get(const std::string& path) {
    WaveReader* reader = reader_cache[path];
    if (reader == nullptr)
        reader = reader_cache[path] = new WaveReader(path);
    return reader;
}

void WaveReader::InvalidadeCache() {
    for (auto& [name, pointer] : reader_cache)
        if (pointer != nullptr) delete pointer;
    reader_cache.clear();
    code_path_cache = "";
}

WaveReader::WaveReader(const std::string& path) {
    JsonRequest json(path);
    nlohmann::json& data = json.data;

    //std::string code_path = path.substr(0, path.rfind("_sm")) + "_code.json";
    std::string code_path = path.substr(0, path.rfind("se")) + "code.json";
    if (code_path != code_path_cache) {
        JsonRequest coderequest(code_path);
        try {
            if (coderequest.fail() || coderequest.bad()) throw std::exception{};
            code_cache = coderequest.data["code"];
            code_path_cache = code_path;
        } catch (std::exception& e) {
            QWARNING(false, "Could not parse " << code_path << " attempting old json format.", (void)0);
            code_cache = data["wave"]["code"];
        }
    }

    auto& code = code_cache;
    auto& instructions = data["wave"]["instructions"];
    int wave_id = data["wave"]["id"];

    int i = 0;
    for(auto& c : code) {
        std::string cppline;
        try {
            cppline = c[3].is_null() ? "" : std::string(c[3]);
        } catch(...) {}
        code_data.push_back({i, c[1], wave_id, int(c[5]), int64_t(c[6]), int(c[7]), 
                            {}, {}, std::move(std::string(c[0])), std::move(cppline)});
        i += 1;
    }

    if (code_data.size() > 0) { // Demangle first line, which is a function name
        int status;
        char* funcname = abi::__cxa_demangle(code_data[0].inst.c_str(), 0, 0, &status);
        if (funcname != nullptr) {
            code_data[0].inst = std::string(funcname);
            free(funcname);
        }
    }

    for(auto& inst : instructions) {
        int64_t clock = int64_t(inst[0]) - WaveManager::global_begin_time;
        tokens.push_back({clock, inst[1], std::max(int(inst[2]), int(inst[3])), inst[4], wave_id});
    }
    sort(
        tokens.begin(),
        tokens.end(),
        [](const Token& first, const Token& second) {
            return (first.clock == second.clock) ?
            (first.cycles > second.cycles) :
            (first.clock < second.clock);
        }
    );

    int thrownLine = 0;
    int max_line = 0;

    for(auto& token : tokens) {
        try {
            CodeData& code = code_data.at(token.code_line);
            code.clock.push_back(token.clock);
            code.cycles.push_back(token.cycles);
            max_line = std::max(max_line, token.code_line);
        } catch (std::out_of_range& e) {
            thrownLine = token.code_line;
        }
    }
    QWARNING(thrownLine==0, "Token referenced invalid code line: " << thrownLine, (void)0);

    //if (max_line+32 < code_data.size())
    //    code_data.erase(code_data.begin()+max_line+32, code_data.end());

    for(auto& time : data["wave"]["timeline"])
        timeline.push_back({int(time[0]), int(time[1])});


    for (auto& array : data["wave"]["waitcnt"]) {
        WaitList list = {array[0], {}};
        for (auto& pair : array[1])
            list.sources.push_back({int(pair[0]), int(pair[1])});
        waitcnt.push_back(std::move(list));
    }

    std::vector<std::string> info_params = {"Issue", "Valu",
                "Salu", "Vmem", "Smem", "Flat", "Lds", "Br"};

    auto& json_wave_info = data["wave"]["info"];
    for (auto& param : info_params) {
        try {
            wave_info.push_back({param, int(json_wave_info[param]), int(json_wave_info[param+"_stall"])});
        } catch (std::exception& e) {
            std::cout << "Warning: Invalid param " << param << std::endl;
        }
    }

    this->wbegin = int64_t(data["wave"]["begin"]) - WaveManager::global_begin_time;
    this->wend = int64_t(data["wave"]["end"]) - WaveManager::global_begin_time;

    if (tokens.size() && tokens[0].clock > this->wbegin && this->wend > tokens.back().end_time()) {
        int64_t first_clock = tokens[0].clock - this->wbegin;
        int64_t last_clock = this->wend - tokens.back().end_time();
        for (int i=0; i<timeline.size() && first_clock > 0; i++) {
            int64_t sub = std::min(first_clock, int64_t(timeline[i].second));
            first_clock -= sub;
            timeline[i].second -= sub;
        }
        for (int i=timeline.size()-1; i>=0 && last_clock > 0; i--) {
            int64_t sub = std::min(last_clock, int64_t(timeline[i].second));
            last_clock -= sub;
            timeline[i].second -= sub;
        }
        this->wbegin = tokens[0].clock;
        this->wend = tokens.back().end_time();
    }
}

StreamRequest::StreamRequest(const std::string& path) {
    if (path.find("http://") != std::string::npos)
        ReadFromNetwork(path);
    else
        ReadFromFile(path);
}

void StreamRequest::ReadFromFile(const std::string& path) {
    std::ifstream f(path, std::fstream::in);
    if (f.good())
        *this << f.rdbuf();
    else
        setstate(std::ios_base::eofbit | std::ios_base::failbit);
}

void StreamRequest::ReadFromNetwork(const std::string& path) {
    std::cout << "Request: " << path << std::endl;
    QNetworkAccessManager manager(this);

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(5000);

    QEventLoop loop;
    connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    QNetworkReply* reply = manager.get(QNetworkRequest(QUrl(path.c_str())));
    timer.start();
    loop.exec();

    replyFinished(reply);
}

void StreamRequest::replyFinished(QNetworkReply* reply) {
    QByteArray arr = reply->readAll();
    *this << arr.data();

    if (str().size() == 0 || reply->error() != 0) {
        setstate(std::ios_base::eofbit | std::ios_base::failbit);
        std::cout << "Response: Error - " << reply->error() << std::endl;
    }
}

JsonRequest::JsonRequest(const std::string& path): StreamRequest(path) {
    QWARNING(!fail() && !bad(), "Could not parse: " << path, return);
    this->data = nlohmann::json::parse(*this);
}
