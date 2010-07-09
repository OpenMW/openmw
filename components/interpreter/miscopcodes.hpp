#ifndef INTERPRETER_MISCOPCODES_H_INCLUDED
#define INTERPRETER_MISCOPCODES_H_INCLUDED

#include <cstdlib>
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
            
    class OpMenuMode : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                runtime.push (runtime.getContext().menuMode());
            }            
    };
            
    class OpRandom : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                double r = static_cast<double> (std::rand()) / RAND_MAX; // [0, 1)
                
                Type_Integer limit = *reinterpret_cast<Type_Integer *> (&runtime[0]);
                
                if (limit<0)
                    throw std::runtime_error (
                        "random: argument out of range (Don't be so negative!)");
                
                Type_Integer value = static_cast<Type_Integer> (r*limit); // [o, limit)
                
                runtime[0] = *reinterpret_cast<Type_Data *> (&value);
            }    
    };    
                
    class OpGetSecondsPassed : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                float duration = runtime.getContext().getSecondsPassed();
                
                runtime.push (*reinterpret_cast<Type_Data *> (&duration));
            }            
    };
    
    class OpEnable : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                runtime.getContext().enable();
            }            
    };    
    
    class OpDisable : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                runtime.getContext().disable();
            }            
    };    
    
    class OpGetDisabled : public Opcode0
    {
        public:
        
            virtual void execute (Runtime& runtime)
            {
                runtime.push (runtime.getContext().isDisabled());
            }            
    };       
    
}

#endif

