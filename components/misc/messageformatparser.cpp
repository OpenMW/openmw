#include "messageformatparser.hpp"

namespace Misc
{
    MessageFormatParser::~MessageFormatParser() {}

    void MessageFormatParser::process(const std::string& m)
    {
        for (unsigned int i = 0; i < m.size(); ++i)
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

                        int width = 0;
                        bool widthSet = false;
                        while (i < m.size() && m[i] >= '0' && m[i] <= '9')
                        {
                            width = width * 10 + (m[i] - '0');
                            widthSet = true;
                            ++i;
                        }

                        if (i < m.size())
                        {
                            int precision = -1;
                            if (m[i] == '.')
                            {
                                precision = 0;
                                while (++i < m.size() && m[i] >= '0' && m[i] <= '9')
                                {
                                    precision = precision * 10 + (m[i] - '0');
                                }
                            }

                            if (i < m.size())
                            {
                                width = (widthSet) ? width : -1;

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
