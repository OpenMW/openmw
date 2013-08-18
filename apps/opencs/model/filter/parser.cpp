
#include "parser.hpp"

#include <stdexcept>

#include "booleannode.hpp"

namespace CSMFilter
{
    struct Token
    {
        enum Type
        {
            Type_EOS,
            Type_None
        };

        Type mType;

        Token (Type type);
    };

    Token::Token (Type type) : mType (type) {}
}

CSMFilter::Token CSMFilter::Parser::getNextToken (const std::string& filter, int& index) const
{
    if (index>=static_cast<int> (filter.size()))
        return Token (Token::Type_EOS);

    return Token (Token::Type_None);
}

bool CSMFilter::Parser::isEndState() const
{
    return mState==State_End || mState==State_UnexpectedCharacter;
}

CSMFilter::Parser::Parser() : mState (State_Begin) {}

void CSMFilter::Parser::parse (const std::string& filter)
{
    // reset
    mState = State_Begin;
    mFilter.reset();
    int index = 0;

    while (!isEndState())
    {
        Token token = getNextToken (filter, index);

        switch (token.mType)
        {
            case Token::Type_None: mState = State_UnexpectedCharacter; break;
            case Token::Type_EOS: mState = State_End; break;
        }
    }

    if (mState==State_End && !mFilter)
    {
        // Empty filter string equals to filter "true".
        mFilter.reset (new BooleanNode (true));
    }
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