#ifndef OPENMW_COMPONENTS_MISC_MESSAGEFORMATPARSER_H
#define OPENMW_COMPONENTS_MISC_MESSAGEFORMATPARSER_H

#include <string_view>

namespace Misc
{
    class MessageFormatParser
    {
    protected:
        enum Placeholder
        {
            StringPlaceholder,
            IntegerPlaceholder,
            FloatPlaceholder
        };

        enum class Notation
        {
            Fixed,
            Scientific,
            Shortest,
            HexUpper,
            HexLower
        };

        virtual void visitedPlaceholder(
            Placeholder placeholder, char padding, int width, int precision, Notation notation)
            = 0;
        virtual void visitedCharacter(char c) = 0;

    public:
        virtual ~MessageFormatParser();

        virtual void process(std::string_view message);
    };
}

#endif
