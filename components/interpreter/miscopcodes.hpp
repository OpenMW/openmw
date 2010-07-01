#ifndef INTERPRETER_MISCOPCODES_H_INCLUDED
#define INTERPRETER_MISCOPCODES_H_INCLUDED

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>

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
                
                // message    
                int index = runtime[0];
                runtime.pop();
                std::string message = runtime.getStringLiteral (index);
                
                // additional parameters
                std::string formattedMessage;
                
                for (std::size_t i=0; i<message.size(); ++i)
                {
                    char c = message[i];
                    
                    if (c!='%')
                        formattedMessage += c;
                    else
                    {
                        ++i;
                        if (i<message.size())
                        {
                            c = message[i];
                            
                            if (c=='S' || c=='s')
                            {
                                int index = runtime[0];
                                runtime.pop();
                                formattedMessage += runtime.getStringLiteral (index);
                            }
                            else if (c=='g' || c=='G')
                            {
                                int value = *reinterpret_cast<const int *> (&runtime[0]);
                                runtime.pop();
                                
                                std::ostringstream out;
                                out << value;
                                formattedMessage += out.str();
                            }
                            else if (c=='f' || c=='F' || c=='.')
                            {
                                while (c!='f' && i<message.size())
                                {
                                    ++i;
                                }
                            
                                float value = *reinterpret_cast<const float *> (&runtime[0]);
                                runtime.pop();
                                
                                std::ostringstream out;
                                out << value;
                                formattedMessage += out.str();
                            }
                            else if (c=='%')
                                formattedMessage += "%";
                            else
                            {
                                formattedMessage += "%"; 
                                formattedMessage += c;                    
                            }
                        }
                    }
                }
                
                // buttons (not implemented)
                std::vector<std::string> buttons;
                
                runtime.getContext().messageBox (formattedMessage, buttons);
            }        
    };
}

#endif

