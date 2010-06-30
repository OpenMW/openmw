#ifndef INTERPRETER_MISCOPCODES_H_INCLUDED
#define INTERPRETER_MISCOPCODES_H_INCLUDED

#include <stdexcept>
#include <vector>
#include <string>

#include "opcodes.hpp"
#include "runtime.hpp"

namespace Interpreter
{
    class OpMessageBox : public Opcode1
    {
        public:
        
            virtual void execute (Runtime& runtime, unsigned int arg0)
            {
                if (arg0!=0)
                    throw std::logic_error ("message box buttons not implemented yet");
                    
                int index = runtime[0];
                runtime.pop();
                std::vector<std::string> buttons;
                runtime.getContext().messageBox (runtime.getStringLiteral (index), buttons);
            }        
    };
}

#endif

