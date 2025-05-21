#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

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
    "Usage:\n"
    "  FuckAI <Commands to know> <AI Options>\n"
    "  FuckAI <Options>\n"
    "Options:\n"
    "  --help,    -h:         Show this help message\n"
    "  --version, -v:         Show version information\n"
    "  --config,  -c:         Edit configuration options\n"
    "    --config(or -c) <key> <value>:\n"
    "      keys: OPENAI_API_KEY, OPENAI_API_BASE_URL, OPENAI_API_MODEL\n"
    "      values: <string>\n"
    "                         To set a configuration option\n"
    "  --show-config, -s:     Show current configuration options\n"
    "\n"
    "AI Options:\n"
    "  --all:                 Use full command list for maximum coverage (token usage increases)\n"
    "  --mini:                Use minimal token usage (may miss some commands)\n"
;

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

std::string get_architecture() {
    FILE* fp = popen("uname -m", "r");
    if (!fp) return "unknown";

    char buf[128];
    std::string result;
    if (fgets(buf, sizeof(buf), fp)) result = buf;
    pclose(fp);

    result.erase(result.find_last_not_of(" \n\r\t")+1);
    return result;
}

std::string get_os_version() {
    std::ifstream ifs("/etc/os-release");
    std::string line, pretty_name;

    while (std::getline(ifs, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            pretty_name = line.substr(13, line.length()-14);
            break;
        }
    }
    if (!pretty_name.empty()) return pretty_name;
    FILE* fp = popen("uname -sr", "r");

    if (!fp) return "unknown";
    char buf[128];
    std::string result;

    if (fgets(buf, sizeof(buf), fp)) result = buf;
    pclose(fp);
    result.erase(result.find_last_not_of(" \n\r\t")+1);
    return result;
}

std::set<std::string> get_installed_commands() {
    std::set<std::string> commands;
    const char* path_env = std::getenv("PATH");
    if (!path_env) return commands;
    std::string path_var = path_env;
    std::stringstream ss(path_var);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (std::filesystem::exists(dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && (entry.status().permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none) {
                    commands.insert(entry.path().filename().string());
                }
            }
        }
    }
    return commands;
}

std::string post_openai_all(
    const std::string& prompt,
    const FuckAIConfig& config,
    const std::string& architecture,
    const std::string& os_version,
    const std::set<std::string>& installed_commands
    ) {
    if (config.find("OPENAI_API_KEY") == config.end()) {
        std::cerr << LOG_ERROR << "API key not found in configuration.\n" << "Please run `FuckAI --config OPENAI_API_KEY <value>` to set it." << std::endl;
        return "";
    }
    if (config.find("OPENAI_API_MODEL") == config.end()) {
        std::cerr << LOG_ERROR << "API model not found in configuration.\n" << "Please run `FuckAI --config OPENAI_API_MODEL <value>` to set it." << std::endl;
        return "";
    }

    std::string api_key = config.at("OPENAI_API_KEY");
    std::string api_url = config.count("OPENAI_API_BASE_URL") ? config.at("OPENAI_API_BASE_URL") : "https://api.openai.com/v1/chat/completions";
    std::string model = config.count("OPENAI_API_MODEL") ? config.at("OPENAI_API_MODEL") : "gpt-4.1-mini-2025-04-14";

    std::string system_prompt =
        "You are an expert in Linux command-line tools.\n"
        "\n"
        "Below is detailed information about this system:\n"
        "Architecture: " + architecture + "\n"
        "OS: " + os_version + "\n"
        "Below is a list of commands installed on this system: \n"
        "```" +
        [&]{
            std::string cmds;
            for (const auto& cmd : installed_commands) cmds += cmd + ", ";
            if (!cmds.empty()) cmds.pop_back(), cmds.pop_back();
            return cmds;
        }() +
        "```\n"
        "The user's question:\n" + prompt + "\n"
        "\n"
        "Based on the available commands and system information, suggest the most appropriate command and provide a brief explanation.\n"
        "Please output your answer in JSON format:\n"
        "[\n"
        "  {\n"
        "    \"command\": \"<the most appropriate command for the user's request, using only the available commands>\",\n"
        "    \"description\": \"<a brief explanation of what the command does and why it is suitable>\"\n"
        "  },\n"
        "  {\n"
        "    \"command\": \"<another possible command>\",\n"
        "    \"description\": \"<explanation>\"\n"
        "  }\n"
        "  // ...more items if applicable\n"
        "]\n"
        "- Only suggest commands that are actually installed on this system.\n"
        "- If there are multiple possible commands, list them in order of recommendation (most standard or widely used first).\n"
        "- If no suitable command is available, respond with an empty array [].\n"
        "- If there is no suitable command installed, suggest an appropriate installation command (such as using apt, yum, or another package manager) in the description field.\n"
        "- Please provide all explanations and answers in the same language as the user's question."
    ;

    nlohmann::json body = {
        {"model", model},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt}},
            {{"role", "user"}, {"content", prompt}}
        }},
        {"temperature", 0.7},
        {"max_tokens", 8128},
        {"stream", false}
    };

    auto response = cpr::Post(
        cpr::Url{api_url},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{body.dump()}
    );

    if (response.status_code != 200) {
        std::cerr << LOG_ERROR << "API request failed with status code: " << response.status_code << std::endl;
        std::cerr << LOG_ERROR << "Response:\n" << response.text << std::endl;
        return "";
    }

    auto json = nlohmann::json::parse(response.text);
    if (json.contains("choices") && !json["choices"].empty()) {
        return json["choices"][0]["message"]["content"];
    }

    return "No response from API";
}

std::string post_openai_mini(
    const std::string& prompt,
    const FuckAIConfig& config,
    const std::string& architecture,
    const std::string& os_version,
    const std::set<std::string>& installed_commands
    ) {
    if (config.find("OPENAI_API_KEY") == config.end()) {
        std::cerr << LOG_ERROR << "API key not found in configuration.\n" << "Please run `FuckAI --config OPENAI_API_KEY <value>` to set it." << std::endl;
        return "";
    }
    if (config.find("OPENAI_API_MODEL") == config.end()) {
        std::cerr << LOG_ERROR << "API model not found in configuration.\n" << "Please run `FuckAI --config OPENAI_API_MODEL <value>` to set it." << std::endl;
        return "";
    }

    std::string api_key = config.at("OPENAI_API_KEY");
    std::string api_url = config.count("OPENAI_API_BASE_URL") ? config.at("OPENAI_API_BASE_URL") : "https://api.openai.com/v1/chat/completions";
    std::string model = config.count("OPENAI_API_MODEL") ? config.at("OPENAI_API_MODEL") : "gpt-4.1-mini-2025-04-14";

    std::string system_prompt_1 =
        "You are an expert in Linux command-line tools.\n"
        "\n"
        "Below is detailed information about this system:\n"
        "Architecture: " + architecture + "\n"
        "OS: " + os_version + "\n"
        "\n"
        "The user's question:\n" + prompt + "\n"
        "\n"
        "Please list as many relevant commands as possible that could fulfill the user's request, covering all possible options.\n"
        "- Only output the command names (do not include any options or arguments).\n"
        "- Output your answer in JSON array format, like this:\n"
        "  [\"command1\", \"command2\", ...]\n"
        "- If no suitable command is available, respond with an empty array [].\n"
        "- Please provide all explanations and answers in the same language as the user's input."
    ;

    nlohmann::json body_1 = {
        {"model", model},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt_1}},
            {{"role", "user"}, {"content", prompt}}
        }},
        {"temperature", 0.7},
        {"max_tokens", 8128},
        {"stream", false}
    };

    auto response_1 = cpr::Post(
        cpr::Url{api_url},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{body_1.dump()}
    );

    if (response_1.status_code != 200) {
        std::cerr << LOG_ERROR << "API request failed with status code: " << response_1.status_code << std::endl;
        std::cerr << LOG_ERROR << "Response:\n" << response_1.text << std::endl;
        return "";
    }

    auto json_1 = nlohmann::json::parse(response_1.text);

    if (json_1.contains("choices") && !json_1["choices"].empty()) {
        std::string content = json_1["choices"][0]["message"]["content"];

        nlohmann::json cmd_json;
        try {
            cmd_json = nlohmann::json::parse(content);
        } catch (...) {
            return content;
        }

        std::vector<std::string> suggested, installed;
        if (cmd_json.is_array()) {
            for (const auto& cmd : cmd_json) {
                if (!cmd.is_string()) continue;
                std::string name = cmd.get<std::string>();
                suggested.push_back(name);
                if (installed_commands.count(name)) {
                    installed.push_back(name);
                }
            }
        }

        std::string system_prompt_2 =
            "You are an expert in Linux command-line tools.\n"
            "\n"
            "Below is detailed information about this system:\n"
            "Architecture: " + architecture + "\n"
            "OS: " + os_version + "\n"
            "Below is a list of commands suggested by the AI (suggested):\n"
            "```\n" +
            [&]{
                std::string cmds;
                for (const auto& cmd : suggested) cmds += cmd + ", ";
                if (!cmds.empty()) cmds.pop_back(), cmds.pop_back();
                return cmds;
            }() +
            "```\n"
            "Below is a list of commands that are actually installed on this system (installed):\n"
            "```\n" +
            [&]{
                std::string cmds;
                for (const auto& cmd : installed) cmds += cmd + ", ";
                if (!cmds.empty()) cmds.pop_back(), cmds.pop_back();
                return cmds;
            }() +
            "```\n"
            "\n"
            "Based on the available commands and system information, suggest the most appropriate command and provide a brief explanation.\n"
            "Please output your answer in JSON format:\n"
            "[\n"
            "  {\n"
            "    \"command\": \"<the most appropriate command for the user's request, using only the available commands>\",\n"
            "    \"description\": \"<a brief explanation of what the command does and why it is suitable>\"\n"
            "  },\n"
            "  {\n"
            "    \"command\": \"<another possible command>\",\n"
            "    \"description\": \"<explanation>\"\n"
            "  }\n"
            "  // ...more items if applicable\n"
            "]\n"
            "- Only suggest commands that are actually installed on this system (installed list).\n"
            "- If there are multiple possible commands, list them in order of recommendation (most standard or widely used first).\n"
            "- If no suitable command is available, respond with an empty array [].\n"
            "- If there is no suitable command installed, suggest an appropriate installation command (such as using apt, yum, or another package manager) in the description field.\n"
            "- Please provide all explanations and answers in the same language as the user's question."
        ;

        nlohmann::json body_2 = {
            {"model", model},
            {"messages", {
                {{"role", "system"}, {"content", system_prompt_2}},
                {{"role", "user"}, {"content", prompt}}
            }},
            {"temperature", 0.7},
            {"max_tokens", 8128},
            {"stream", false}
        };

        auto response_2 = cpr::Post(
            cpr::Url{api_url},
            cpr::Header{
                {"Authorization", "Bearer " + api_key},
                {"Content-Type", "application/json"}
            },
            cpr::Body{body_2.dump()}
        );

        if (response_2.status_code != 200) {
            std::cerr << LOG_ERROR << "API request failed with status code: " << response_2.status_code << std::endl;
            std::cerr << LOG_ERROR << "Response:\n" << response_2.text << std::endl;
            return "";
        }

        auto json_2 = nlohmann::json::parse(response_2.text);
        if (json_2.contains("choices") && !json_2["choices"].empty()) {
            return json_2["choices"][0]["message"]["content"];
        }
    }
    return "No response from API";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << LOG_ERROR << "No command provided." << std::endl;
        std::cerr << USAGE << std::endl;
        return 1;
    }

    if (
        std::strcmp(argv[1], "--help") == 0 ||
        std::strcmp(argv[1], "-h") == 0
    ) {
        std::cout << USAGE << std::endl;
        return 0;
    }

    if (
        std::strcmp(argv[1], "--version") == 0 ||
        std::strcmp(argv[1], "-v") == 0
    ) {
        std::cout << "Version " << VERSION << std::endl;
        return 0;
    }

    FuckAIConfig config = load_config(CONFIG_FILE);

    if (
        std::strcmp(argv[1], "--config") == 0 ||
        std::strcmp(argv[1], "-c") == 0
    ) {
        if (argc < 4) {
            std::cerr << "Usage: FuckAI --config <key> <value>" << std::endl;
            return 1;
        }
        std::string key = argv[2];
        std::string value = argv[3];

        if (CONFIG_KEYS.find(key) == CONFIG_KEYS.end()) {
            std::cerr << LOG_ERROR << "Invalid configuration key: " << key << std::endl;
            std::cerr << USAGE << std::endl;
            return 1;
        }

        config[key] = value;
        save_config(CONFIG_FILE, config);
        std::cout << LOG_INFO << "Configuration updated: " << key << std::endl;
        return 0;
    }

    if (
        std::strcmp(argv[1], "--show-config") == 0 ||
        std::strcmp(argv[1], "-s") == 0
    ) {
        for (const auto& pair : config) {
            std::cout << pair.first << "=" << pair.second << std::endl;
        }
        return 0;
    }

    if (argc > 3) {
        std::cerr << LOG_ERROR << "Please make sure to enclose strings in quotes." << std::endl;
        std::cerr << USAGE << std::endl;
        return 1;
    }

    std::string architecture = get_architecture();
    std::string os_version = get_os_version();
    std::set<std::string> installed_commands = get_installed_commands();

    std::cout << LOG_INFO << "System Architecture: " << architecture << std::endl;
    std::cout << LOG_INFO << "OS Version: " << os_version << std::endl;
    std::cout << LOG_INFO << "Installed Commands: "<< installed_commands.size() << std::endl;

    std::string prompt = argv[1];

    if (prompt.empty()) {
        std::cerr << LOG_ERROR << "No command provided." << std::endl;
        std::cerr << USAGE << std::endl;
        return 1;
    }

    bool use_all = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--all") == 0) use_all = true;
        else if (std::strcmp(argv[i], "--mini") == 0) use_all = false;
    }

    std::string response;
    if (use_all) {
        response = post_openai_all(prompt, config, architecture, os_version, installed_commands);
    } else {
        response = post_openai_mini(prompt, config, architecture, os_version, installed_commands);
    }

    if (!response.empty()) {
        try {
            auto result_json = nlohmann::json::parse(response);

            if (result_json.is_array()) {
                std::cout << "=== Commands ===" << std::endl;
                for (const auto& item : result_json) {
                    if (item.contains("command")) {
                        std::cout << "- " << item["command"].get<std::string>() << std::endl;
                    }
                    if (item.contains("description")) {
                        std::cout << "  " << item["description"].get<std::string>() << std::endl;
                    }
                    std::cout << std::endl;
                }
            } else {
                std::cout << LOG_ERROR << "No JSON format was returned: " + response << std::endl;
            }
        } catch (...) {
            std::cout << LOG_ERROR << "No JSON format was returned: " + response << std::endl;
        }
    } else {
        std::cerr << LOG_ERROR << "Failed to get a response from the API." << std::endl;
        return 1;
    }
}
