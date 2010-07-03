#ifndef COMPILER_EXTENSIONS_H_INCLUDED
#define COMPILER_EXTENSINOS_H_INCLUDED

#include <string>
#include <map>
#include <vector>

#include <components/interpreter/types.hpp>

namespace Compiler
{
    /// \brief Collection of compiler extensions
    
    class Extensions
    {
            struct Function
            {
                char mReturn;
                std::string mArguments;
                int mCode;
            };
    
            int mNextKeywordIndex;
            std::map<std::string, int> mKeywords;
            std::map<int, Function> mFunctions;
        
        public:
    
            Extensions();
    
            int searchKeyword (const std::string& keyword) const;
            ///< Return extension keyword code, that is assigned to the string \a keyword.
            /// - if no match is found 0 is returned.
            /// - keyword must be all lower case.
            
            bool isFunction (int keyword, char& returnType, std::string& argumentType) const;
            ///< Is this keyword registered with a function? If yes, return return and argument
            /// types.
                        
            void registerFunction (const std::string& keyword, char returnType,
                const std::string& argumentType, int segment5code);
            ///< Register a custom function
            /// - keyword must be all lower case.
            /// - keyword must be unique
            /// \note Currently only segment 5 opcodes are supported.
            
            void generateFunctionCode (int keyword, std::vector<Interpreter::Type_Code>& code)
                const;
            ///< Append code for function to \a code.
    };
}

#endif

