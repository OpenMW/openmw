#include <components/esm/loaddial.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/store.hpp"
#include "../mwworld/esmstore.hpp"

#include "keywordsearch.hpp"

#include "hypertextparser.hpp"

namespace MWDialogue
{
    namespace HyperTextParser
    {
        std::vector<Token> parseHyperText(const std::string & text)
        {
            std::vector<Token> result;
            size_t pos_end, iteration_pos = 0;
            for(;;)
            {
                size_t pos_begin = text.find('@', iteration_pos);
                if (pos_begin != std::string::npos)
                    pos_end = text.find('#', pos_begin);

                if (pos_begin != std::string::npos && pos_end != std::string::npos)
                {
                    if (pos_begin != iteration_pos)
                        tokenizeKeywords(text.substr(iteration_pos, pos_begin - iteration_pos), result);

                    std::string link = text.substr(pos_begin + 1, pos_end - pos_begin - 1);
                    result.emplace_back(link, Token::ExplicitLink);

                    iteration_pos = pos_end + 1;
                }
                else
                {
                    if (iteration_pos != text.size())
                        tokenizeKeywords(text.substr(iteration_pos), result);
                    break;
                }
            }

            return result;
        }

        void tokenizeKeywords(const std::string & text, std::vector<Token> & tokens)
        {
            const MWWorld::Store<ESM::Dialogue> & dialogs =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

            std::list<std::string> keywordList;
            for (MWWorld::Store<ESM::Dialogue>::iterator it = dialogs.begin(); it != dialogs.end(); ++it)
                keywordList.push_back(Misc::StringUtils::lowerCase(it->mId));
            keywordList.sort(Misc::StringUtils::ciLess);

            KeywordSearch<std::string, int /*unused*/> keywordSearch;

            for (std::list<std::string>::const_iterator it = keywordList.begin(); it != keywordList.end(); ++it)
                keywordSearch.seed(*it, 0 /*unused*/);

            std::vector<KeywordSearch<std::string, int /*unused*/>::Match> matches;
            keywordSearch.highlightKeywords(text.begin(), text.end(), matches);

            for (std::vector<KeywordSearch<std::string, int /*unused*/>::Match>::const_iterator it = matches.begin(); it != matches.end(); ++it)
            {
                tokens.emplace_back(std::string(it->mBeg, it->mEnd), Token::ImplicitKeyword);
            }
        }

        size_t removePseudoAsterisks(std::string & phrase)
        {
            size_t pseudoAsterisksCount = 0;

            if( !phrase.empty() )
            {
                std::string::reverse_iterator rit = phrase.rbegin();

                const char specialPseudoAsteriskCharacter = 127;
                while( rit != phrase.rend() && *rit == specialPseudoAsteriskCharacter )
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
