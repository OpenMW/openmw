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

            virtual void visitedPlaceholder(Placeholder placeholder, char padding, int width, int precision) = 0;
            virtual void visitedCharacter(char c) = 0;

        public:
            virtual void process(const std::string& message);
    };
}

#endif
