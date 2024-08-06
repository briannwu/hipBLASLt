#pragma once

#include <string>
#include <vector>

class InputFile {
    friend class InputFileEdit;
public:
    InputFile() {};
    InputFile(const std::string& filename);

    static std::vector<std::string> GetAvailableCounters();
    std::string getContents();
    void Export(const std::string& filename);

private:
    int target_cu = 1;
    int simd_mask = 0xF;
    int counter_period = -1;
    std::vector<std::string> counters;
};
