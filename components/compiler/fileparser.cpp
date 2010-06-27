#include "fileparser.hpp"

#include <iostream>

#include "tokenloc.hpp"
#include "scanner.hpp"

namespace Compiler
{
    FileParser::FileParser (ErrorHandler& errorHandler, Context& context)
    : Parser (errorHandler, context), mState (BeginState)
    {}

    std::string FileParser::getName() const
    {
        return mName;
    }

    bool FileParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==NameState)
        {
            mName = name;
            mState = BeginCompleteState;
            return true;
        }
        
        if (mState==EndNameState)
        {
            // optional repeated name after end statement
            if (mName!=name)
                reportWarning ("Names for script " + mName + " do not match", loc);
            
            mState = EndCompleteState;
            return true;
        }
        
        return Parser::parseName (name, loc, scanner);
    }

    bool FileParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==BeginState && keyword==Scanner::K_begin)
        {
            mState = NameState;
            return true;
        }
        
        if (mState==EndState && keyword==Scanner::K_end)
        {
            mState = EndNameState;
            return true;
        }
        
        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool FileParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
        {
            if (mState==BeginCompleteState)
            {
                // TODO: add script parser here
                mState = EndState;
                return true;
            }
            
            if (mState==EndCompleteState || mState==EndNameState)
            {
                // we are done here -> ignore the rest of the script
                return false;
            }
        }
        
        return Parser::parseSpecial (code, loc, scanner);
    }

    void FileParser::parseEOF (Scanner& scanner)
    {
        if (mState!=EndNameState && mState!=EndCompleteState)
            Parser::parseEOF (scanner);
    }
    
    void FileParser::reset()
    {
        mState = BeginState;
        mName.clear();
    }
}

