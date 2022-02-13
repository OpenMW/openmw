#ifndef COMPONENTS_TOUTF8_H
#define COMPONENTS_TOUTF8_H

#include <string>
#include <cstring>
#include <vector>
#include <string_view>

namespace ToUTF8
{
    // These are all the currently supported code pages
    enum FromType
    {
        WINDOWS_1250,      // Central ane Eastern European languages
        WINDOWS_1251,      // Cyrillic languages
        WINDOWS_1252,       // Used by English version of Morrowind (and
            // probably others)
        CP437           // Used for fonts (*.fnt) if data files encoding is 1252. Otherwise, uses the same encoding as the data files.
    };

    FromType calculateEncoding(const std::string& encodingName);
    std::string encodingUsingMessage(const std::string& encodingName);

    // class

    class Utf8Encoder
    {
        public:
            Utf8Encoder(FromType sourceEncoding);

            /// Convert to UTF8 from the previously given code page.
            /// Returns a view to internal buffer invalidate by next getUtf8 or getLegacyEnc call if input is not
            /// ASCII-only string. Otherwise returns a view to the input.
            std::string_view getUtf8(std::string_view input);

            /// Returns a view to internal buffer invalidate by next getUtf8 or getLegacyEnc call if input is not
            /// ASCII-only string. Otherwise returns a view to the input.
            std::string_view getLegacyEnc(std::string_view input);

        private:
            inline void resize(std::size_t size);
            inline std::pair<std::size_t, bool> getLength(std::string_view input) const;
            inline void copyFromArray(unsigned char chp, char* &out) const;
            inline std::pair<std::size_t, bool> getLengthLegacyEnc(std::string_view input) const;
            inline void copyFromArrayLegacyEnc(std::string_view::iterator& chp, std::string_view::iterator end, char* &out) const;

            std::vector<char> mOutput;
            const signed char* translationArray;
    };
}

#endif
