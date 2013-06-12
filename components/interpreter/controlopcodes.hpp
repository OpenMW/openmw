#ifndef INTERPRETER_CONTROLOPCODES_H_INCLUDED
#define INTERPRETER_CONTROLOPCODES_H_INCLUDED

#include <stdexcept>

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    class OpReturn : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                runtime.setPC (-1);
            }
    };

    class OpSkipZero : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                Type_Integer data = runtime[0].mInteger;
                runtime.pop();

                if (data==0)
                    runtime.setPC (runtime.getPC()+1);
            }
    };

    class OpSkipNonZero : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                Type_Integer data = runtime[0].mInteger;
                runtime.pop();

                if (data!=0)
                    runtime.setPC (runtime.getPC()+1);
            }
    };

    class OpJumpForward : public Opcode1
    {
        public:

            virtual void execute (Runtime& runtime, unsigned int arg0)
            {
                if (arg0==0)
                    throw std::logic_error ("infinite loop");

                runtime.setPC (runtime.getPC()+arg0-1);
            }
    };

    class OpJumpBackward : public Opcode1
    {
        public:

            virtual void execute (Runtime& runtime, unsigned int arg0)
            {
                if (arg0==0)
                    throw std::logic_error ("infinite loop");

                runtime.setPC (runtime.getPC()-arg0-1);
            }
    };
}

#endif
