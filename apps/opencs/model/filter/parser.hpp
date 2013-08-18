#ifndef CSM_FILTER_PARSER_H
#define CSM_FILTER_PARSER_H

#include <boost/shared_ptr.hpp>

#include "node.hpp"

namespace CSMFilter
{
    class Parser
    {
        public:

            enum State
            {
                State_Begin,
                State_End
            };

        private:

            State mState;
            boost::shared_ptr<Node> mFilter;

        public:

            Parser();

            void parse (const std::string& filter);
            ///< Discards any previous calls to parse

            State getState() const;

            boost::shared_ptr<Node> getFilter() const;
            ///< Throws an exception if getState()!=State_End
    };
}

#endif
