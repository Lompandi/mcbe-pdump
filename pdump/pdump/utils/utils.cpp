
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

    std::optional<PIMAGE_SECTION_HEADER>
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

    std::vector<std::pair<uint32_t, uint32_t>> 
    get_runtime_functions_range(std::byte* base, PIMAGE_NT_HEADERS pNtHdr) {
        uint32_t dFnRva;
        dFnRva = pNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
        if (!dFnRva)
            return {};

        size_t entries_count = pNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size
            / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
        if (!entries_count)
            return {};

        PIMAGE_RUNTIME_FUNCTION_ENTRY pRtFn = (PIMAGE_RUNTIME_FUNCTION_ENTRY)
            rva_to_foa(dFnRva, pNtHdr, (uintptr_t)base);

        if (!pRtFn)
            return {};

        std::vector<std::pair<uint32_t, uint32_t>> func_list;
        for (unsigned i = 0; i < entries_count; i++, pRtFn++) {
            func_list.push_back(std::make_pair(pRtFn->BeginAddress, pRtFn->EndAddress));
        }

        return func_list;
    }

    PIMAGE_SECTION_HEADER get_enclose_section_hdr(DWORD rva,
        PIMAGE_NT_HEADERS pNTHeader)
    {
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
        unsigned i;

        for (i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++)
        {
            if ((rva >= section->VirtualAddress) &&
                (rva < (section->VirtualAddress + section->Misc.VirtualSize)))
                return section;
        }

        return 0;
    }

    LPVOID rva_to_foa(DWORD rva, PIMAGE_NT_HEADERS pNTHeader, uintptr_t imageBase)
    {
        PIMAGE_SECTION_HEADER pSectionHdr;
        INT delta;

        pSectionHdr = get_enclose_section_hdr(rva, pNTHeader);
        if (!pSectionHdr)
            return 0;

        delta = (INT)(pSectionHdr->VirtualAddress - pSectionHdr->PointerToRawData);
        return (PVOID)(imageBase + rva - delta);
    }

    DWORD foa_to_rva(DWORD foa, const PIMAGE_NT_HEADERS ntHeaders, const uint8_t* peFileData) {
        IMAGE_FILE_HEADER fileHeader = ntHeaders->FileHeader;
        IMAGE_OPTIONAL_HEADER optionalHeader = ntHeaders->OptionalHeader;

        DWORD sectionOffset = sizeof(IMAGE_NT_HEADERS) + fileHeader.SizeOfOptionalHeader;

        for (int i = 0; i < fileHeader.NumberOfSections; ++i) {
            IMAGE_SECTION_HEADER* sectionHeader = (IMAGE_SECTION_HEADER*)(peFileData + sectionOffset + i * sizeof(IMAGE_SECTION_HEADER));

            if (foa >= sectionHeader->PointerToRawData && foa < (sectionHeader->PointerToRawData + sectionHeader->SizeOfRawData)) {
                return (foa - sectionHeader->PointerToRawData + sectionHeader->VirtualAddress);
            }
        }
        return 0;
    }
}