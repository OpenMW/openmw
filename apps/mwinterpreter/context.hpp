#ifndef SAINTERPRETER_CONTEXT_H_INCLUDED
#define SAINTERPRETER_CONTEXT_H_INCLUDED

#include <string>
#include <vector>

#include <components/interpreter/context.hpp>
#include <components/interpreter/types.hpp>

namespace SAInterpreter
{
    class Context : public Interpreter::Context
    {
            std::vector<Interpreter::Type_Short> mShorts;
            std::vector<Interpreter::Type_Integer> mLongs;
            std::vector<Interpreter::Type_Float> mFloats;
            std::vector<std::string> mNames;
            
        public:
        
            Context (const std::string& filename);
            ///< Create context from file
            /// \note A context for an integreted interpreter will typically not
            /// configure at construction, but will offer a separate function.
    
            virtual int getLocalShort (int index) const;

            virtual int getLocalLong (int index) const;

            virtual float getLocalFloat (int index) const;

            virtual void setLocalShort (int index, int value);        

            virtual void setLocalLong (int index, int value);        

            virtual void setLocalFloat (int index, float value);    
            
            virtual void messageBox (const std::string& message,
                const std::vector<std::string>& buttons); 
                            
            virtual bool menuMode();

            virtual int getGlobalShort (const std::string& name) const;

            virtual int getGlobalLong (const std::string& name) const;

            virtual float getGlobalFloat (const std::string& name) const;

            virtual void setGlobalShort (const std::string& name, int value);        

            virtual void setGlobalLong (const std::string& name, int value);        

            virtual void setGlobalFloat (const std::string& name, float value);
            
            virtual bool isScriptRunning (const std::string& name) const;
            
            virtual void startScript (const std::string& name);
            
            virtual void stopScript (const std::string& name);
            
            virtual float getDistance (const std::string& name) const;
            
            virtual float getSecondsPassed() const;
            
            virtual bool isDisabled (const std::string& id = "") const;
            
            virtual void enable (const std::string& id = "");
            
            virtual void disable (const std::string& id = "");
                        
            void report();
            ///< Write state to std::cout   
    };
}

#endif
