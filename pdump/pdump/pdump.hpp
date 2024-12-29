#pragma once

#include <string>
#include <unordered_map>

namespace pdm {
    class CmdOptions_t {
    public:
        void add_option(const std::string& flag, const std::string& defaultValue) {
            m_CmdArgs[flag] = { defaultValue, false };
        }

        void parse(int argc, char* argv[]) {
            for (int i = 1; i < argc; ++i) {
                std::string arg = argv[i];
                if (m_CmdArgs.find(arg) != m_CmdArgs.end()) {
                    if (i + 1 < argc) {
                        m_CmdArgs[arg].first = argv[i + 1];
                        m_CmdArgs[arg].second = true;
                        ++i;
                    }
                }
            }
        }

        std::string get_flag_val(const std::string& flag) const {
            if (m_CmdArgs.find(flag) != m_CmdArgs.end()) {
                return m_CmdArgs.at(flag).first;
            }
            return {};
        }

        bool is_set(const std::string& flag) const {
            return m_CmdArgs.find(flag) != m_CmdArgs.end() && m_CmdArgs.at(flag).second;
        }

    private:
        std::unordered_map<std::string, std::pair<std::string, bool>> m_CmdArgs;
    };

    void start();
    extern CmdOptions_t g_CmdOpts;
    extern std::string g_ExecutablePath;
    extern std::string g_OutputDumpPath;
}