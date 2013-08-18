
#include "parser.hpp"

#include <stdexcept>

#include "booleannode.hpp"

CSMFilter::Parser::Parser() : mState (State_Begin) {}

void CSMFilter::Parser::parse (const std::string& filter)
{
    // reset
    mState = State_Begin;
    mFilter.reset();


    // for now we ignore the filter string
    mFilter.reset (new BooleanNode (false));
    mState = State_End;
}

CSMFilter::Parser::State CSMFilter::Parser::getState() const
{
    return mState;
}

boost::shared_ptr<CSMFilter::Node> CSMFilter::Parser::getFilter() const
{
    if (mState!=State_End)
        throw std::logic_error ("No filter available");

    return mFilter;
}