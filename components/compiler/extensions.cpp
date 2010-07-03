
#include "extensions.hpp"

#include <cassert>
#include <stdexcept>

#include "generator.hpp"

namespace Compiler
{
    Extensions::Extensions() : mNextKeywordIndex (-1) {}

    int Extensions::searchKeyword (const std::string& keyword) const
    {
        std::map<std::string, int>::const_iterator iter = mKeywords.find (keyword);
        
        if (iter==mKeywords.end())
            return 0;
            
        return iter->second;
    }
        
    bool Extensions::isFunction (int keyword, char& returnType, std::string& argumentType) const
    {
        std::map<int, Function>::const_iterator iter = mFunctions.find (keyword);
        
        if (iter==mFunctions.end())
            return false;
            
        returnType = iter->second.mReturn;
        argumentType = iter->second.mArguments;
        return true;
    }
    
    void Extensions::registerFunction (const std::string& keyword, char returnType,
        const std::string& argumentType, int segment5code)
    {
        assert (segment5code>=33554432 && segment5code<=67108863);
    
        int code = mNextKeywordIndex--;
        
        mKeywords.insert (std::make_pair (keyword, code));
        
        Function function;
        function.mReturn = returnType;
        function.mArguments = argumentType;
        function.mCode = segment5code;
        
        mFunctions.insert (std::make_pair (code, function));
    }
    
    void Extensions::generateFunctionCode (int keyword, std::vector<Interpreter::Type_Code>& code)
        const
    {
        std::map<int, Function>::const_iterator iter = mFunctions.find (keyword);
        
        if (iter==mFunctions.end())
            throw std::logic_error ("unknown custom function keyword");
            
        Generator::segment5 (iter->second.mCode);
    }
}
