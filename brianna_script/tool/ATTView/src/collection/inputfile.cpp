#include "inputfile.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include "../include/memtracker.h"

enum ATTParam {
    UNKNOWN = 0,
    CU,
    SIMD_MASK,
    PERFCOUNTER,
    PERFCOUNTERS_COL_PERIOD,
    REDUCED_MEMORY
};

std::unordered_map<std::string, ATTParam> att_params = {
    {"TARGET_CU", ATTParam::CU},
    {"SIMD_MASK", ATTParam::SIMD_MASK},
    {"PERFCOUNTER", ATTParam::PERFCOUNTER},
    {"PERFCOUNTERS_COL_PERIOD", ATTParam::PERFCOUNTERS_COL_PERIOD},
    {"REDUCED_MEMORY", ATTParam::REDUCED_MEMORY}
};

InputFile::InputFile(const std::string& filename) {
    std::ifstream file(filename, std::fstream::in);

    while (file.good()) {
        std::string s;
        file >> s;
        size_t pos = s.find('=');

        if (pos == std::string::npos || pos+1 >= s.size())
            continue;

        try {
            ATTParam param = att_params.at(s.substr(0, pos));
            s = s.substr(pos+1);

            if (param == ATTParam::PERFCOUNTER) {
                counters.push_back(s);
                continue;
            }

            int value;
            try {
                if (s.find("0x") != std::string::npos)
                    value = std::stoi(s.substr(s.find("0x")+2).c_str(), 0, 16);
                else
                    value = std::stoi(s.c_str(), 0, 10);
            } catch (std::exception& e) {
                value = 0xF;
            }

            switch (param) {
                case ATTParam::CU: {
                    this->target_cu = value;
                    break;
                }
                case ATTParam::SIMD_MASK: {
                    this->simd_mask = value;
                    break;
                }
                case ATTParam::PERFCOUNTERS_COL_PERIOD: {
                    this->counter_period = value;
                    break;
                }
            }
        } catch (std::out_of_range& e) {
            std::cout << "Invalid param: " << s << std::endl;
        }
    }
}

std::string InputFile::getContents() {
    std::stringstream output;

    output << "att: TARGET_CU=" << this->target_cu << '\n';
    output << "SIMD_MASK=0x" << std::hex << this->simd_mask << std::dec << '\n';

    if (this->counter_period >= 0 && counters.size()>0) {
        output << "\nPERFCOUNTERS_COL_PERIOD=" << this->counter_period << '\n';
        for (auto& c : counters)
            output << "PERFCOUNTER=" << c << '\n';
    }

    return output.str();
}

static std::vector<std::string> get_counters() {
    char buffer[128];
    std::string result;
    std::vector<std::string> lines;

    std::unique_ptr<FILE, decltype(&pclose)>
        pipe(popen("rocprofv2 --list-counters | grep SQ", "r"), pclose);

    QWARNING(bool(pipe), "Could not open rocprofv2", return lines);

    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr)
        result += std::string(buffer);

    size_t cur_pos = 0;
    while (cur_pos != std::string::npos && cur_pos+1 < result.size()) {
        size_t new_pos = result.find('\n', cur_pos+1);
        if (new_pos != std::string::npos)
            lines.push_back(result.substr(cur_pos+1, new_pos-cur_pos-1));
        cur_pos = new_pos;
    }
    return lines;
}

std::vector<std::string> InputFile::GetAvailableCounters() {
    std::vector<std::string> counters;
    std::vector<std::string> lines = get_counters();

    for (int i=0; i<int(lines.size())-1; i++) {
        size_t cpos = lines[i].find("SQ_");
        if (cpos == std::string::npos)
            continue;
        if (lines[i+1].find("block SQ can only handle") == std::string::npos)
            continue;

        size_t space_pos = lines[i].find(" : ", cpos);
        if (space_pos != std::string::npos)
            counters.push_back(lines[i].substr(cpos, space_pos-cpos));
    }
    return counters;
}

void InputFile::Export(const std::string& filename) {
    std::string contents = getContents();
    std::ofstream f(filename, std::fstream::out);
    f << contents;
}
