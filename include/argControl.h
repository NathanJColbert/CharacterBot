#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>

class ARGControl {
public:
    ARGControl() = default;

    template<typename T>
    void addOption(const std::string& flag, T& variable, const std::string& description);

    bool parse(int argc, char** argv);
    void printHelp() const;

private:
    struct Option {
        std::function<void(const std::string&)> setFunc;
        std::string description;
    };

    std::unordered_map<std::string, Option> options;
};