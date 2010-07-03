#ifndef INTERPRETER_CONTEXT_H_INCLUDED
#define INTERPRETER_CONTEXT_H_INCLUDED

#include <string>
#include <vector>

namespace Interpreter
{
    class Context
    {
        public:

            virtual ~Context() {}

            virtual int getLocalShort (int index) const = 0;

            virtual int getLocalLong (int index) const = 0;

            virtual float getLocalFloat (int index) const = 0;

            virtual void setLocalShort (int index, int value) = 0;        

            virtual void setLocalLong (int index, int value) = 0;        

            virtual void setLocalFloat (int index, float value) = 0;
            
            virtual void messageBox (const std::string& message,
                const std::vector<std::string>& buttons) = 0; 
                
            void messageBox (const std::string& message)
            {
                std::vector<std::string> empty;
                messageBox (message, empty);
            }
    };
}

#endif


