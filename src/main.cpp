#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>


#define LOG_ERROR   "\033[31mError: \033[0m"
#define LOG_WARNING "\033[33mWarning: \033[0m"
#define LOG_INFO    "\033[32mInfo: \033[0m"
#define LOG_DEBUG   "\033[34mDebug: \033[0m"


typedef std::map<std::string, std::string> FuckAIConfig;

const char* VERSION = "1.0.0";

const char* USAGE =
    "FuckAI - A simple command helper AI for those of you who still don't remember the commands yet\n"
    "----------------------------------------------------\n"
    "\n"
    "Usage: FuckAI <Commands to know>\n"
    "Options:\n"
    "  --help,    -h, help              Show this help message\n"
    "  --version, -v, version           Show version information\n"
    "  --config,  -c, config            Edit configuration options\n"
    "    --config(or -c, config) <key> <value>  To set a configuration option\n"
    "  --show-config, -s, show-config   Show current configuration options\n";

const std::set<std::string> CONFIG_KEYS = {
    "OPENAI_API_KEY",
    "OPENAI_API_BASE_URL",
    "OPENAI_API_MODEL"
};

const char* CONFIG_FILE = "config.txt";


FuckAIConfig load_config(const std::string& filename) {
    FuckAIConfig config;
    std::ifstream ifs(filename);
    std::string line;
    while (std::getline(ifs, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            if (CONFIG_KEYS.count(key)) {
                config[key] = value;
            } else {
                std::cerr << LOG_WARNING << "Unknown config ignored on loading: " << key << "=" << value << std::endl;
            }
        }
    }
    return config;
}

int save_config(const std::string& filename, const FuckAIConfig& config) {
    std::filesystem::path file_path(filename);
    auto dir = file_path.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(dir, ec)) {
            std::cerr << LOG_ERROR << "Could not create directory: " << dir << std::endl;
            return 1;
        }
    }

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << LOG_ERROR << "Could not open config file for writing." << std::endl;
        return 1;
    }
    for (const auto& pair : config) {
        if (CONFIG_KEYS.count(pair.first) == 0) {
            std::cerr << LOG_WARNING << "Unknown config ignored on saving: " << pair.first << "=" << pair.second << std::endl;
        }
        ofs << pair.first << "=" << pair.second << std::endl;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << LOG_ERROR << "No command provided." << std::endl;
        std::cerr << USAGE << std::endl;
        return 1;
    }

    if (
        std::strcmp(argv[1], "--help") == 0 ||
        std::strcmp(argv[1], "-h") == 0 ||
        std::strcmp(argv[1], "help") == 0
    ) {
        std::cout << USAGE << std::endl;
        return 0;
    }

    if (
        std::strcmp(argv[1], "--version") == 0 ||
        std::strcmp(argv[1], "-v") == 0 ||
        std::strcmp(argv[1], "version") == 0
    ) {
        std::cout << "Version " << VERSION << std::endl;
        return 0;
    }

    FuckAIConfig config = load_config(CONFIG_FILE);

    if (
        std::strcmp(argv[1], "--config") == 0 ||
        std::strcmp(argv[1], "-c") == 0 ||
        std::strcmp(argv[1], "config") == 0
    ) {
        if (argc < 4) {
            std::cerr << "Usage: FuckAI --config(or -c, config) <key> <value>" << std::endl;
            return 1;
        }
        std::string key = argv[2];
        std::string value = argv[3];

        config[key] = value;
        save_config(CONFIG_FILE, config);
        return 0;
    }

    if (
        std::strcmp(argv[1], "--show-config") == 0 ||
        std::strcmp(argv[1], "-s") == 0 ||
        std::strcmp(argv[1], "show-config") == 0
    ) {
        for (const auto& pair : config) {
            std::cout << pair.first << "=" << pair.second << std::endl;
        }
        return 0;
    }
}
