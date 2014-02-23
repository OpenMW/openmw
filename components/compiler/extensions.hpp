#ifndef COMPILER_EXTENSIONS_H_INCLUDED
#define COMPILER_EXTENSIONS_H_INCLUDED

#include <string>
#include <map>
#include <vector>

#include <components/interpreter/types.hpp>

namespace Compiler
{
    class Literals;

    /// \brief Collection of compiler extensions

    class Extensions
    {
            struct Function
            {
                char mReturn;
                std::string mArguments;
                int mCode;
                int mCodeExplicit;
                int mSegment;
            };

            struct Instruction
            {
                std::string mArguments;
                int mCode;
                int mCodeExplicit;
                int mSegment;
            };

            int mNextKeywordIndex;
            std::map<std::string, int> mKeywords;
            std::map<int, Function> mFunctions;
            std::map<int, Instruction> mInstructions;

        public:

            Extensions();

            int searchKeyword (const std::string& keyword) const;
            ///< Return extension keyword code, that is assigned to the string \a keyword.
            /// - if no match is found 0 is returned.
            /// - keyword must be all lower case.

            bool isFunction (int keyword, char& returnType, std::string& argumentType,
                bool& explicitReference) const;
            ///< Is this keyword registered with a function? If yes, return return and argument
            /// types.
            /// \param explicitReference In: has explicit reference; Out: set to false, if
            /// explicit reference is not available for this instruction.

            bool isInstruction (int keyword, std::string& argumentType,
                bool& explicitReference) const;
            ///< Is this keyword registered with a function? If yes, return argument types.
            /// \param explicitReference In: has explicit reference; Out: set to false, if
            /// explicit reference is not available for this instruction.

            void registerFunction (const std::string& keyword, char returnType,
                const std::string& argumentType, int code, int codeExplicit = -1);
            ///< Register a custom function
            /// - keyword must be all lower case.
            /// - keyword must be unique
            /// - if explicit references are not supported, segment5codeExplicit must be set to -1
            /// \note Currently only segment 3 and segment 5 opcodes are supported.

            void registerInstruction (const std::string& keyword,
                const std::string& argumentType, int code, int codeExplicit = -1);
            ///< Register a custom instruction
            /// - keyword must be all lower case.
            /// - keyword must be unique
            /// - if explicit references are not supported, segment5codeExplicit must be set to -1
            /// \note Currently only segment 3 and segment 5 opcodes are supported.

            void generateFunctionCode (int keyword, std::vector<Interpreter::Type_Code>& code,
                Literals& literals, const std::string& id, int optionalArguments) const;
            ///< Append code for function to \a code.

            void generateInstructionCode (int keyword, std::vector<Interpreter::Type_Code>& code,
                Literals& literals, const std::string& id, int optionalArguments) const;
            ///< Append code for function to \a code.

            void listKeywords (std::vector<std::string>& keywords) const;
            ///< Append all known keywords to \æ kaywords.
    };
}

#endif
