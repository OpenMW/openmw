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
            if (m[i] == '%')
            {
                if (++i < m.size())
                {
                    if (m[i] == '%')
                        visitedCharacter('%');
                    else
                    {
                        char pad = ' ';
                        if (m[i] == '0' || m[i] == ' ')
                        {
                            pad = m[i];
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
                                    visitedPlaceholder(StringPlaceholder, pad, width, precision, FixedNotation);
                                else if (m[i] == 'd' || m[i] == 'i')
                                    visitedPlaceholder(IntegerPlaceholder, pad, width, precision, FixedNotation);
                                else if (m[i] == 'f' || m[i] == 'F')
                                    visitedPlaceholder(FloatPlaceholder, pad, width, precision, FixedNotation);
                                else if (m[i] == 'e' || m[i] == 'E')
                                    visitedPlaceholder(FloatPlaceholder, pad, width, precision, ScientificNotation);
                                else if (m[i] == 'g' || m[i] == 'G')
                                    visitedPlaceholder(FloatPlaceholder, pad, width, precision, ShortestNotation);
                            }
                        }
                    }
                }
            }
            else
            {
                visitedCharacter(m[i]);
            }
        }
    }
}
