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
            
            virtual bool menuMode() = 0;

            virtual int getGlobalShort (const std::string& name) const = 0;

            virtual int getGlobalLong (const std::string& name) const = 0;

            virtual float getGlobalFloat (const std::string& name) const = 0;

            virtual void setGlobalShort (const std::string& name, int value) = 0;        

            virtual void setGlobalLong (const std::string& name, int value) = 0;        

            virtual void setGlobalFloat (const std::string& name, float value) = 0;     
            
            virtual bool isScriptRunning (const std::string& name) const = 0;
            
            virtual void startScript (const std::string& name) = 0;
            
            virtual void stopScript (const std::string& name) = 0;
            
            virtual float getDistance (const std::string& name) const = 0; 
            
            virtual float getSecondsPassed() const = 0;
            
            virtual bool isDisabled() const = 0;
            
            virtual void enable() = 0;
            
            virtual void disable() = 0;    
    };
}

#endif


