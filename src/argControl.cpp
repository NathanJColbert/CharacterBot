#include "argControl.h"

template<typename T>
void ARGControl::addOption(const std::string& flag, T& variable, const std::string& description) {
    options[flag] = {
        [&](const std::string& val) {
            if constexpr (std::is_same<T, int>::value) variable = std::stoi(val);
            else if constexpr (std::is_same<T, float>::value) variable = std::stof(val);
            else if constexpr (std::is_same<T, bool>::value) variable = (val == "true" || val == "1");
            else variable = val;
        },
        description
    };
}

bool ARGControl::parse(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (options.count(arg)) {
            if (i + 1 < argc) {
                options[arg].setFunc(argv[++i]);
            } else {
                std::cerr << "Missing value for option: " << arg << "\n";
                return false;
            }
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
    }
    return true;
}

void ARGControl::printHelp() const {
    std::cout << "Options:\n";
    for (const auto& [flag, opt] : options) {
        std::cout << "  " << flag << "\t" << opt.description << "\n";
    }
}

template void ARGControl::addOption<int>(const std::string&, int&, const std::string&);
template void ARGControl::addOption<std::string>(const std::string&, std::string&, const std::string&);
template void ARGControl::addOption<bool>(const std::string&, bool&, const std::string&);
template void ARGControl::addOption<float>(const std::string&, float&, const std::string&);