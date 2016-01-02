#ifndef COMPILER_EXTENSIONS_H_INCLUDED
#define COMPILER_EXTENSIONS_H_INCLUDED

#include <string>
#include <map>
#include <vector>

#include <components/interpreter/types.hpp>

namespace Compiler
{
    class Literals;

    /// Typedef for script arguments string
    /** Every character reperesents an argument to the command. All arguments are required until a /, after which
        every argument is optional. <BR>
        Eg: fff/f represents 3 required floats followed by one optional float <BR>
        f - Float <BR>
        c - String, case smashed <BR>
        l - Integer <BR>
        s - Short <BR>
        S - String, case preserved <BR>
        x - Optional, ignored string argument. Emits a parser warning when this argument is supplied. <BR>
        X - Optional, ignored numeric expression. Emits a parser warning when this argument is supplied. <BR>
        z - Optional, ignored string or numeric argument. Emits a parser warning when this argument is supplied. <BR>
        j - A piece of junk (either . or a specific keyword)
    **/
    typedef std::string ScriptArgs;

    /// Typedef for script return char
    /** The character represents the type of data being returned. <BR>
        f - float <BR>
        S - String (Cell names) <BR>
        l - Integer
    **/
    typedef char ScriptReturn;

    /// \brief Collection of compiler extensions
    class Extensions
    {

            struct Function
            {
                char mReturn;
                ScriptArgs mArguments;
                int mCode;
                int mCodeExplicit;
                int mSegment;
            };

            struct Instruction
            {
                ScriptArgs mArguments;
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

            bool isFunction (int keyword, ScriptReturn& returnType, ScriptArgs& argumentType,
                bool& explicitReference) const;
            ///< Is this keyword registered with a function? If yes, return return and argument
            /// types.
            /// \param explicitReference In: has explicit reference; Out: set to false, if
            /// explicit reference is not available for this instruction.

            bool isInstruction (int keyword, ScriptArgs& argumentType,
                bool& explicitReference) const;
            ///< Is this keyword registered with a function? If yes, return argument types.
            /// \param explicitReference In: has explicit reference; Out: set to false, if
            /// explicit reference is not available for this instruction.

            void registerFunction (const std::string& keyword, ScriptReturn returnType,
                const ScriptArgs& argumentType, int code, int codeExplicit = -1);
            ///< Register a custom function
            /// - keyword must be all lower case.
            /// - keyword must be unique
            /// - if explicit references are not supported, segment5codeExplicit must be set to -1
            /// \note Currently only segment 3 and segment 5 opcodes are supported.

            void registerInstruction (const std::string& keyword,
                const ScriptArgs& argumentType, int code, int codeExplicit = -1);
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
            ///< Append all known keywords to \a kaywords.
    };
}

#endif
