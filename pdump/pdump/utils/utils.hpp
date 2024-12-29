#pragma once

#include <windows.h>
#include <psapi.h>

#include <optional>
#include <chrono>
#include <vector>
#include <iostream>
#include <sstream>
#include <print>
#include <span>

#include "hasher.hpp"
#include "enum_util.hpp"
#include "scanner/scanner.hpp"
#include <source_location>

namespace pdm {
    extern bool g_Verbose;

    void hexdump(const char* data, size_t size, size_t bytes_per_line, std::optional<size_t> max_byte_dump, bool create_ascii_decode);
    void write_file(const std::string& filename, const char* data, size_t size);

    inline void verbose_print(const std::string& formated) {
        if (g_Verbose) {
            std::cout << formated;
        }
    }

    bool str_to_bool(const std::string& str);

    std::optional<PIMAGE_SECTION_HEADER> get_section_hdr(const std::string& section_name, LPVOID lpBase, PIMAGE_NT_HEADERS& ntHeaders);
    std::vector<std::pair<uint32_t, uint32_t>> get_runtime_functions_range(std::byte* base, PIMAGE_NT_HEADERS pNtHdr);

    enum class log_level
    {
        Info = 0x0,
        Warning,
        Error,
        Debug,
    };

    constexpr std::string_view log_level_to_str(log_level log_lvl) {
        if (log_lvl == log_level::Info)
            return "INFO";
        else if (log_lvl == log_level::Warning)
            return "WARNING";
        else if (log_lvl == log_level::Error)
            return "ERROR";
        else if (log_lvl == log_level::Debug)
            return "DEBUG";
        return {};
    }

    template<typename... Args>
    std::string process_fmt(std::string_view fmt_str, Args&&... args) {
        return std::vformat(fmt_str, std::make_format_args(std::forward<Args>(args)...));
    }

    template <typename ...Args>
    void log(log_level const level, std::string_view const message, Args&&... args) {
        auto as_local = [](std::chrono::system_clock::time_point const tp) {
            return std::chrono::zoned_time{ std::chrono::current_zone(), tp };
        };

        auto to_string = [](auto tp) {
            return std::format("{:%F %T}", tp);
        };

        std::string fmt_str = process_fmt(message, std::forward<Args>(args)...);

        std::cout <<
            std::format("[{} {}] {}",
                to_string(as_local(std::chrono::system_clock::now())).substr(0, 23),
                log_level_to_str(level),
                fmt_str)
            << '\n';
    }
    
    std::optional<PIMAGE_NT_HEADERS> get_nt_header_from_file(const std::string& filePath, std::vector<uint8_t>& fileData);
    PIMAGE_SECTION_HEADER get_enclose_section_hdr(DWORD rva, PIMAGE_NT_HEADERS pNTHeader);

    LPVOID rva_to_foa(DWORD rva, PIMAGE_NT_HEADERS pNTHeader, uintptr_t imageBase);
    DWORD foa_to_rva(DWORD foa, const PIMAGE_NT_HEADERS ntHeaders, const uint8_t* peFileData);
}