#ifndef CSM_FILTER_PARSER_H
#define CSM_FILTER_PARSER_H

#include <boost/shared_ptr.hpp>

#include "node.hpp"

namespace CSMFilter
{
    struct Token;

    class Parser
    {
            boost::shared_ptr<Node> mFilter;
            std::string mInput;
            int mIndex;
            bool mError;

            Token getStringToken();

            Token getNumberToken();

            Token getNextToken();

            Token checkKeywords (const Token& token);
            ///< Turn string token into keyword token, if possible.

            boost::shared_ptr<Node> parseImp (bool allowEmpty = false);
            ///< Will return a null-pointer, if there is nothing more to parse.

            boost::shared_ptr<Node> parseNAry (const Token& keyword);

            void error();

        public:

            Parser();

            bool parse (const std::string& filter);
            ///< Discards any previous calls to parse
            ///
            /// \return Success?

            boost::shared_ptr<Node> getFilter() const;
            ///< Throws an exception if the last call to parse did not return true.
    };
}

#endif
