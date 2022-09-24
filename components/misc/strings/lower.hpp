#ifndef COMPONENTS_MISC_STRINGS_LOWER_H
#define COMPONENTS_MISC_STRINGS_LOWER_H

#include <string>
#include <string_view>

namespace Misc::StringUtils
{
    /* Mapping table to go from uppercase to lowercase for plain ASCII.*/
    static constexpr unsigned char tolowermap[256] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
        47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103, 104,
        105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
        119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
        141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162,
        163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184,
        185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
        207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228,
        229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
        251, 252, 253, 254, 255 };

    /// Plain and simple locale-unaware toLower. Anything from A to Z is lower-cased, multibyte characters are
    /// unchanged. Don't use std::tolower(char, locale&) because that is abysmally slow. Don't use tolower(int) because
    /// that depends on global locale.
    inline constexpr char toLower(char c)
    {
        return tolowermap[static_cast<unsigned char>(c)];
    }
    inline constexpr char8_t toLower(char8_t c)
    {
        return tolowermap[static_cast<unsigned char>(c)];
    }

    /// Transforms input string to lower case w/o copy
    inline void lowerCaseInPlace(std::string& str)
    {
        for (auto& ch : str)
            ch = toLower(ch);
    }
    inline void lowerCaseInPlace(std::u8string& str)
    {
        for (auto& ch : str)
            ch = toLower(ch);
    }

    /// Returns lower case copy of input string
    inline std::string lowerCase(std::string_view in)
    {
        std::string out(in);
        lowerCaseInPlace(out);
        return out;
    }
    inline std::u8string lowerCase(std::u8string_view in)
    {
        std::u8string out(in);
        lowerCaseInPlace(out);
        return out;
    }
}

#endif
