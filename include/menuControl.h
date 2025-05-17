#pragma once
#include <string>
#include <vector>

inline std::string s_toLower(std::string input) {
    std::string result = "";
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); i++) {
        if (!isalnum(input[i])) continue;
        result += tolower(input[i]);
    }
    return result;
}

inline bool menuCompareInput(std::string input, std::vector<std::string> values) {
    input = s_toLower(input);
    for (size_t i = 0; i < values.size(); i++)
        if (input == values[i]) return true;
    return false;
}