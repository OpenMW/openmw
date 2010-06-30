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
                            
            void report();
            ///< Write state to std::cout   
    };
}

#endif
