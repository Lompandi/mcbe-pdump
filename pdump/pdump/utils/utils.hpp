#pragma once

#include <windows.h>
#include <optional>
#include <print>
#include <vector>

#include "hasher.hpp"
#include <iostream>
#include <sstream>
#include <span>


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

    enum SearchDirection {
        Up = 0,
        Down,
    };

    inline std::optional<PIMAGE_SECTION_HEADER>
    get_section_hdr(const std::string& section_name, LPVOID lpBase, PIMAGE_NT_HEADERS& ntHeaders) {
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)lpBase;
        ntHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpBase + dosHeader->e_lfanew);
        PIMAGE_FILE_HEADER fileHeader = &ntHeaders->FileHeader;
        PIMAGE_OPTIONAL_HEADER optionalHeader = &ntHeaders->OptionalHeader;

        PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < fileHeader->NumberOfSections; i++) {
            std::string csection_name((char*)sectionHeader[i].Name);
            if (csection_name == section_name) {
                return &sectionHeader[i];
            }
        }

        return std::nullopt;
    }

    inline void read_pdata_functions(LPVOID lpBase, PIMAGE_SECTION_HEADER pdataSection, std::vector<RUNTIME_FUNCTION*>& functions) {
        LPVOID pdataBase = (LPVOID)((DWORD_PTR)lpBase + pdataSection->VirtualAddress);
        size_t pdataSize = pdataSection->SizeOfRawData;

        size_t count = pdataSize / sizeof(RUNTIME_FUNCTION);
        functions.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            functions.push_back(reinterpret_cast<RUNTIME_FUNCTION*>(static_cast<uint8_t*>(pdataBase) + i * sizeof(RUNTIME_FUNCTION)));
        }
    }


    // WIP
    std::optional<uint8_t> hex_to_byte(const std::string& hex) {
        if (hex.size() != 2) return std::nullopt;

        uint8_t byte;
        std::istringstream(hex) >> std::hex >> byte;
        return byte;
    }

    inline std::pair<std::span<uint8_t>, std::span<uint8_t>> make_signature(const std::string& signature) {
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> masks;

        std::istringstream signatureStream(signature);
        std::string token;

        while (signatureStream >> token) {
            if (token == "??") {
                bytes.push_back(0x00);
                masks.push_back(0xFF);
            }
            else {
                auto byteOpt = hex_to_byte(token);
                if (byteOpt) {
                    bytes.push_back(byteOpt.value());
                    masks.push_back(0x00); 
                }
                else {
                    std::cerr << "Invalid byte value in signature: " << token << std::endl;
                }
            }
        }

        return { std::span<uint8_t>(bytes), std::span<uint8_t>(masks) };
    }

    std::optional<PIMAGE_NT_HEADERS> get_nt_header_from_file(const std::string& filePath, std::vector<uint8_t>& fileData);
}