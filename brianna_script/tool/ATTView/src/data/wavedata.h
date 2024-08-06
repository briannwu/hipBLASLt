#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "wavemanager.h"
#include "include/json.hpp"
#include <QObject>

//! Class responsible for parsing the wavedata files
class WaveReader {
    set_tracked();
public:
    WaveReader(const std::string& path);
    static const WaveReader* Get(const std::string& path);
    static void InvalidadeCache();

    std::vector<CodeData> code_data;
    std::vector<Token> tokens;
    std::vector<std::pair<int, int>> timeline;
    std::vector<WaitList> waitcnt;
    std::vector<WaveInfo> wave_info;
    int64_t wbegin, wend;

private:
    static std::unordered_map<std::string, WaveReader*> reader_cache;
    static std::string code_path_cache;
    static nlohmann::json code_cache;
};

//! Class containing a string to be parsed by WaveReader. Can be requested by disk or network.
class StreamRequest: public QObject, public std::stringstream {
    Q_OBJECT
    set_tracked();
public:
    StreamRequest(const std::string& path);

protected:
    class QNetworkAccessManager* manager = nullptr;
    void ReadFromFile(const std::string& path);
    void ReadFromNetwork(const std::string& path);
    void replyFinished(class QNetworkReply* reply);
};

//! Requests a json file by disk or network, depending on path
class JsonRequest: public StreamRequest {
    Q_OBJECT
    set_tracked();
public:
    JsonRequest(const std::string& path);
    nlohmann::json data;
};
