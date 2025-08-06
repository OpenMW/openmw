#include <components/esm3/loaddial.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/store.hpp"

#include "keywordsearch.hpp"

#include "hypertextparser.hpp"

namespace MWDialogue
{
    namespace HyperTextParser
    {
        std::vector<Token> parseHyperText(const std::string& text)
        {
            std::vector<Token> result;
            size_t posEnd = std::string::npos;
            size_t iterationPos = 0;
            for (;;)
            {
                const size_t posBegin = text.find('@', iterationPos);
                if (posBegin != std::string::npos)
                    posEnd = text.find('#', posBegin);

                if (posBegin != std::string::npos && posEnd != std::string::npos)
                {
                    if (posBegin != iterationPos)
                        tokenizeKeywords(text.substr(iterationPos, posBegin - iterationPos), result);

                    std::string link = text.substr(posBegin + 1, posEnd - posBegin - 1);
                    result.emplace_back(link, Token::ExplicitLink);

                    iterationPos = posEnd + 1;
                }
                else
                {
                    if (iterationPos != text.size())
                        tokenizeKeywords(text.substr(iterationPos), result);
                    break;
                }
            }

            return result;
        }

        void tokenizeKeywords(const std::string& text, std::vector<Token>& tokens)
        {
            const auto& keywordSearch
                = MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>().getDialogIdKeywordSearch();

            std::vector<KeywordSearch<int /*unused*/>::Match> matches;
            keywordSearch.highlightKeywords(text.begin(), text.end(), matches);

            for (const auto& match : matches)
            {
                tokens.emplace_back(std::string(match.mBeg, match.mEnd), Token::ImplicitKeyword);
            }
        }

        size_t removePseudoAsterisks(std::string& phrase)
        {
            size_t pseudoAsterisksCount = 0;

            if (!phrase.empty())
            {
                std::string::reverse_iterator rit = phrase.rbegin();

                const char specialPseudoAsteriskCharacter = 127;
                while (rit != phrase.rend() && *rit == specialPseudoAsteriskCharacter)
                {
                    pseudoAsterisksCount++;
                    ++rit;
                }
            }

            phrase = phrase.substr(0, phrase.length() - pseudoAsterisksCount);

            return pseudoAsterisksCount;
        }
    }
}
