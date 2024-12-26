
#define NOMINMAX

#include <fstream>
#include <chrono>
#include <algorithm>
#include <unordered_map>

#include "utils.hpp"

namespace pdm {
    bool g_Verbose;

    void hexdump(const char* data, size_t size, size_t bytes_per_line,
        std::optional<size_t> max_byte_dump, bool create_ascii_decode) {

        auto bytedec = [](unsigned char byte) {
            return std::_Is_printable(byte) ? (char)byte : '.';
        };

        for (size_t i = 0; i < std::min(max_byte_dump.value_or(size), size); i += bytes_per_line) {
            std::print("{:08x}     ", i);
            for (size_t j = 0; j < bytes_per_line && (i + j) < size; ++j) {
                std::print("{:02x} ", (unsigned int)(unsigned char)data[i + j]);
            }

            std::print(" | ");

            if (create_ascii_decode) {
                for (size_t j = 0; j < bytes_per_line && (i + j) < size; ++j) {
                    std::print("{}", bytedec((unsigned char)data[i + j]));
                }
            }

            std::print("\n");
        }

        if (max_byte_dump && max_byte_dump.value() < size) {
            std::print("... {} bytes remaining\n", size - max_byte_dump.value());
        }
        std::print("\n");
    }

    void write_file(const std::string& filename, const char* data, size_t size) {
        std::ofstream target_file(filename, std::ios::binary);

        if (!target_file) {
            verbose_print("[pkdump] error opening file!");
            return;
        }

        target_file.write(data, size);

        if (target_file.fail()) {
            verbose_print("[pkdump] error writing to file!\n");
            return;
        }
        verbose_print(std::format("[pkdump] file written: {}\n", filename));
        target_file.close();
    }

    bool str_to_bool(const std::string& str) {
        static const std::unordered_map<std::string, bool> stringToBoolMap = {
            {"true", true},
            {"false", false},
            {"1", true},
            {"0", false}
        };

        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

        auto it = stringToBoolMap.find(lowerStr);
        if (it != stringToBoolMap.end()) {
            return it->second;
        }
        else {
            throw std::invalid_argument("Invalid input string: must be 'true', 'false', '1', or '0'");
        }
    }

    std::optional<PIMAGE_NT_HEADERS> get_nt_header_from_file(const std::string& filePath, std::vector<uint8_t>& fileData) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Error opening file: " << filePath << std::endl;
            return std::nullopt;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        fileData.resize(fileSize);
        if (!file.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
            std::cerr << "Error reading file: " << filePath << std::endl;
            return std::nullopt;
        }

        PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(fileData.data());
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            std::cerr << "Invalid DOS header signature." << std::endl;
            return std::nullopt;
        }

        PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(fileData.data() + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            std::cerr << "Invalid NT headers signature." << std::endl;
            return std::nullopt;
        }

        return ntHeaders;
    }
}