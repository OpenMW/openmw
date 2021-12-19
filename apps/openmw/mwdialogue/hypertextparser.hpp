#ifndef GAME_MWDIALOGUE_HYPERTEXTPARSER_H
#define GAME_MWDIALOGUE_HYPERTEXTPARSER_H

#include <string>
#include <vector>

#include "keywordsearch.hpp"

namespace MWDialogue
{
    class HyperTextParser
    {
        uint64_t mKeywordModPoint;
        KeywordSearch<std::string, int /*unused*/> mKeywordSearch;

    public:

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

        HyperTextParser() : mKeywordModPoint(0) {}

        // In translations (at least Russian) the links are marked with @#, so
        // it should be a function to parse it
        std::vector<Token> parseHyperText(const std::string & text);
        void tokenizeKeywords(const std::string & text, std::vector<Token> & tokens);
        static size_t removePseudoAsterisks(std::string & phrase);
    };
}

#endif
