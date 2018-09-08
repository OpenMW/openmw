#ifndef OPENMW_COMPONENTS_MISC_MESSAGEFORMATPARSER_H
#define OPENMW_COMPONENTS_MISC_MESSAGEFORMATPARSER_H

#include <string>

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

            enum Notation
            {
                FixedNotation,
                ScientificNotation,
                ShortestNotation
            };

            virtual void visitedPlaceholder(Placeholder placeholder, char padding, int width, int precision, Notation notation) = 0;
            virtual void visitedCharacter(char c) = 0;

        public:
            virtual ~MessageFormatParser();

            virtual void process(const std::string& message);
    };
}

#endif
