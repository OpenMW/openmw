#ifndef OPENMW_COMPONENTS_TOUTF8_TOUTF8_HPP
#define OPENMW_COMPONENTS_TOUTF8_TOUTF8_HPP

#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace ToUTF8
{
    // These are all the currently supported code pages
    enum FromType
    {
        WINDOWS_1250, // Central ane Eastern European languages
        WINDOWS_1251, // Cyrillic languages
        WINDOWS_1252, // Used by English version of Morrowind (and
                      // probably others)
        CP437 // Used for fonts (*.fnt) if data files encoding is 1252. Otherwise, uses the same encoding as the data
              // files.
    };

    enum class BufferAllocationPolicy
    {
        FitToRequiredSize,
        UseGrowFactor,
    };

    FromType calculateEncoding(std::string_view encodingName);
    std::string encodingUsingMessage(std::string_view encodingName);

    class StatelessUtf8Encoder
    {
    public:
        explicit StatelessUtf8Encoder(FromType sourceEncoding);

        /// Convert to UTF8 from the previously given code page.
        /// Returns a view to passed buffer that will be resized to fit output if it's too small.
        std::string_view getUtf8(
            std::string_view input, BufferAllocationPolicy bufferAllocationPolicy, std::string& buffer) const;

        /// Convert from UTF-8 to sourceEncoding.
        /// Returns a view to passed buffer that will be resized to fit output if it's too small.
        std::string_view getLegacyEnc(
            std::string_view input, BufferAllocationPolicy bufferAllocationPolicy, std::string& buffer) const;

    private:
        inline std::pair<std::size_t, bool> getLength(std::string_view input) const;
        inline void copyFromArray(unsigned char chp, char*& out) const;
        inline std::pair<std::size_t, bool> getLengthLegacyEnc(std::string_view input) const;
        inline void copyFromArrayLegacyEnc(
            std::string_view::iterator& chp, std::string_view::iterator end, char*& out) const;

        const std::basic_string_view<signed char> mTranslationArray;
    };

    class Utf8Encoder
    {
    public:
        explicit Utf8Encoder(FromType sourceEncoding);

        /// Convert to UTF8 from the previously given code page.
        /// Returns a view to internal buffer invalidate by next getUtf8 or getLegacyEnc call if input is not
        /// ASCII-only string. Otherwise returns a view to the input.
        std::string_view getUtf8(std::string_view input);

        /// Convert from UTF-8 to sourceEncoding.
        /// Returns a view to internal buffer invalidate by next getUtf8 or getLegacyEnc call if input is not
        /// ASCII-only string. Otherwise returns a view to the input.
        std::string_view getLegacyEnc(std::string_view input);

        const StatelessUtf8Encoder& getStatelessEncoder() const { return mImpl; }

    private:
        std::string mBuffer;
        StatelessUtf8Encoder mImpl;
    };
}

#endif
