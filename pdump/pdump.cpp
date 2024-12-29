#define NOMINMAX

#include <print>

#include "pdump/pdump.hpp"

int main(int argc, char** argv) {

    if (argc < 2) {
        std::print("You must specify a executable path to bedrock server!\n");
        return 0;
    }
    pdm::g_CmdOpts.add_option("--dump", "true");
    pdm::g_CmdOpts.add_option("--dump-max", "1024");
    pdm::g_CmdOpts.add_option("--dump-decode", "false");

    pdm::g_CmdOpts.add_option("--verbose", "false");
    pdm::g_CmdOpts.add_option("--output", "");

    //"D:\\bedrock-server-1.21.51.02\\bedrock_server.exe"
    pdm::g_ExecutablePath = argv[1];

    pdm::g_CmdOpts.parse(argc - 1, &argv[2]);
    pdm::start();

    return 0;
}
