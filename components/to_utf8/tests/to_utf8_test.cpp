#include <iostream>
#include <fstream>
#include <cassert>
#include <stdexcept>

#include "../to_utf8.hpp"

std::string getFirstLine(const std::string &filename);
void testEncoder(ToUTF8::FromType encoding, const std::string &legacyEncFile,
                 const std::string &utf8File);

/// Test character encoding conversion to and from UTF-8
void testEncoder(ToUTF8::FromType encoding, const std::string &legacyEncFile,
                 const std::string &utf8File)
{
    // get some test data
    std::string legacyEncLine = getFirstLine(legacyEncFile);
    std::string utf8Line = getFirstLine(utf8File);

    // create an encoder for specified character encoding
    ToUTF8::Utf8Encoder encoder (encoding);

    // convert text to UTF-8
    std::string convertedUtf8Line = encoder.getUtf8(legacyEncLine);

    std::cout << "original:  " << utf8Line          << std::endl;
    std::cout << "converted: " << convertedUtf8Line << std::endl;

    // check correctness
    assert(convertedUtf8Line == utf8Line);

    // convert UTF-8 text to legacy encoding
    std::string convertedLegacyEncLine = encoder.getLegacyEnc(utf8Line);
    // check correctness
    assert(convertedLegacyEncLine == legacyEncLine);
}

std::string getFirstLine(const std::string &filename)
{
    std::string line;
    std::ifstream text (filename.c_str());

    if (!text.is_open())
    {
        throw std::runtime_error("Unable to open file " + filename);
    }

    std::getline(text, line);
    text.close();

    return line;
}

int main()
{
    testEncoder(ToUTF8::WINDOWS_1251, "test_data/russian-win1251.txt", "test_data/russian-utf8.txt");
    testEncoder(ToUTF8::WINDOWS_1252, "test_data/french-win1252.txt", "test_data/french-utf8.txt");
    return 0;
}
