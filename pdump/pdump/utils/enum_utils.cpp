
#include "enum_util.hpp"

namespace pdm::enum_utils {
	char* g_alloc[0x200] = {};

    std::string extract_enum_value(const char* input) {
        std::string prot_type(input);

        auto templ_start = prot_type.find('<');
        auto templ_end = prot_type.find('>');

        auto templ_prot = prot_type.substr(templ_start + 1, templ_end - templ_start - 1);

        auto split = [&](auto token) {
            auto off = templ_prot.find(token);
            templ_prot = templ_prot.substr(off + strlen(token));
            };

        split(",");
        split("::");

        return templ_prot;
    }
}
