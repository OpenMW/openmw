#include "messageformatparser.hpp"

#include <charconv>

namespace
{
    int parseNumber(std::size_t& i, std::string_view m, int fallback)
    {
        if (i < m.size() && m[i] >= '0' && m[i] <= '9')
        {
            const char* start = m.data() + i;
            int parsed;
            auto [ptr, ec] = std::from_chars(start, m.data() + m.size(), parsed);
            i += ptr - start;
            if (ec == std::errc())
                return parsed;
        }
        return fallback;
    }
}

namespace Misc
{
    MessageFormatParser::~MessageFormatParser() = default;

    void MessageFormatParser::process(std::string_view m)
    {
        for (std::size_t i = 0; i < m.size(); ++i)
        {
            if (m[i] != '%')
            {
                visitedCharacter(m[i]);
                continue;
            }
            if (++i == m.size())
                break;
            if (m[i] == '%')
            {
                visitedCharacter('%');
                continue;
            }

            int flags = None;
            while (i < m.size())
            {
                if (m[i] == '-')
                    flags |= AlignLeft;
                else if (m[i] == '+')
                    flags |= PositiveSign;
                else if (m[i] == ' ')
                    flags |= PositiveSpace;
                else if (m[i] == '0')
                    flags |= PrependZero;
                else if (m[i] == '#')
                    flags |= AlternateForm;
                else
                    break;
                ++i;
            }

            int width = parseNumber(i, m, -1);

            if (i < m.size())
            {
                int precision = -1;
                if (m[i] == '.')
                {
                    ++i;
                    precision = parseNumber(i, m, 0);
                }

                if (i < m.size())
                {
                    if (m[i] == 'S' || m[i] == 's')
                        visitedPlaceholder(StringPlaceholder, flags, width, precision, Notation::Fixed);
                    else if (m[i] == 'd' || m[i] == 'i')
                        visitedPlaceholder(IntegerPlaceholder, flags, width, precision, Notation::Fixed);
                    else if (m[i] == 'f' || m[i] == 'F')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::Fixed);
                    else if (m[i] == 'e')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::ScientificLower);
                    else if (m[i] == 'E')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::ScientificUpper);
                    else if (m[i] == 'g')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::ShortestLower);
                    else if (m[i] == 'G')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::ShortestUpper);
                    else if (m[i] == 'a')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::HexLower);
                    else if (m[i] == 'A')
                        visitedPlaceholder(FloatPlaceholder, flags, width, precision, Notation::HexUpper);
                    else
                        visitedCharacter(m[i]);
                }
            }
        }
    }
}
