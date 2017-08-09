#ifndef CSM_FILTER_PARSER_H
#define CSM_FILTER_PARSER_H

#include "node.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSMFilter
{
    struct Token;

    class Parser
    {
            std::shared_ptr<Node> mFilter;
            std::string mInput;
            int mIndex;
            bool mError;
            const CSMWorld::Data& mData;

            Token getStringToken();

            Token getNumberToken();

            Token getNextToken();

            Token checkKeywords (const Token& token);
            ///< Turn string token into keyword token, if possible.

            std::shared_ptr<Node> parseImp (bool allowEmpty = false, bool ignoreOneShot = false);
            ///< Will return a null-pointer, if there is nothing more to parse.

            std::shared_ptr<Node> parseNAry (const Token& keyword);

            std::shared_ptr<Node> parseText();

            std::shared_ptr<Node> parseValue();

            void error();

        public:

            Parser (const CSMWorld::Data& data);

            bool parse (const std::string& filter, bool allowPredefined = true);
            ///< Discards any previous calls to parse
            ///
            /// \return Success?

            std::shared_ptr<Node> getFilter() const;
            ///< Throws an exception if the last call to parse did not return true.
    };
}

#endif
