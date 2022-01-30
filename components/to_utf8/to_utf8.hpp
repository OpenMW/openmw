#ifndef COMPONENTS_TOUTF8_H
#define COMPONENTS_TOUTF8_H

#include <string>
#include <cstring>
#include <vector>

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

            // Convert to UTF8 from the previously given code page.
            std::string getUtf8(const char *input, size_t size);
            inline std::string getUtf8(const std::string &str)
            {
                return getUtf8(str.c_str(), str.size());
            }

            // Convert input to UTF8 to the given output string
            void toUtf8(std::string& input, std::string& output, size_t size);

            std::string getLegacyEnc(const char *input, size_t size);
            inline std::string getLegacyEnc(const std::string &str)
            {
                return getLegacyEnc(str.c_str(), str.size());
            }

        private:
            void resize(size_t size);
            size_t getLength(const char* input, bool &ascii) const;
            void copyFromArray(unsigned char chp, char* &out) const;
            size_t getLength2(const char* input, bool &ascii) const;
            void copyFromArray2(const char*& chp, char* &out) const;

            std::vector<char> mOutput;
            const signed char* translationArray;
    };
}

#endif
