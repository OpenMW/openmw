#ifndef INTERPRETER_CONTEXT_H_INCLUDED
#define INTERPRETER_CONTEXT_H_INCLUDED

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
    };
}

#endif


