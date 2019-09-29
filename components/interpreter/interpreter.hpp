#ifndef INTERPRETER_INTERPRETER_H_INCLUDED
#define INTERPRETER_INTERPRETER_H_INCLUDED

#include <map>
#include <stack>

#include "runtime.hpp"
#include "types.hpp"

namespace Interpreter
{
    class Opcode0;
    class Opcode1;

    class Interpreter
    {
            std::stack<Runtime> mCallstack;
            bool mRunning;
            Runtime mRuntime;
            std::map<int, Opcode1 *> mSegment0;
            std::map<int, Opcode1 *> mSegment2;
            std::map<int, Opcode1 *> mSegment3;
            std::map<int, Opcode0 *> mSegment5;

            // not implemented
            Interpreter (const Interpreter&);
            Interpreter& operator= (const Interpreter&);

            void execute (Type_Code code);

            void abortUnknownCode (int segment, int opcode);

            void abortUnknownSegment (Type_Code code);

            void begin();

            void end();

        public:

            Interpreter();

            ~Interpreter();

            void installSegment0 (int code, Opcode1 *opcode);
            ///< ownership of \a opcode is transferred to *this.

            void installSegment2 (int code, Opcode1 *opcode);
            ///< ownership of \a opcode is transferred to *this.

            void installSegment3 (int code, Opcode1 *opcode);
            ///< ownership of \a opcode is transferred to *this.

            void installSegment5 (int code, Opcode0 *opcode);
            ///< ownership of \a opcode is transferred to *this.

            void run (const Type_Code *code, int codeSize, Context& context);
    };
}

#endif
