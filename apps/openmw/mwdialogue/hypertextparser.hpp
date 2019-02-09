#ifndef GAME_MWDIALOGUE_HYPERTEXTPARSER_H
#define GAME_MWDIALOGUE_HYPERTEXTPARSER_H

#include <string>
#include <vector>

namespace MWDialogue
{
    namespace HyperTextParser
    {
        struct Token
        {
            enum Type
            {
                ExplicitLink, // enclosed in @#
                ImplicitKeyword
            };

            Token(const std::string & text, Type type) : mText(text), mType(type) {}

            bool isExplicitLink() { return mType == ExplicitLink; }

            std::string mText;
            Type mType;
        };

        // In translations (at least Russian) the links are marked with @#, so
        // it should be a function to parse it
        std::vector<Token> parseHyperText(const std::string & text);
        void tokenizeKeywords(const std::string & text, std::vector<Token> & tokens);
        size_t removePseudoAsterisks(std::string & phrase);
    }
}

#endif
