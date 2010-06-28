#ifndef INTERPRETER_RUNTIME_H_INCLUDED
#define INTERPRETER_RUNTIME_H_INCLUDED

#include "types.hpp"

namespace Interpreter
{
    class Context;

    /// Runtime data and engine interface
    
    class Runtime
    {
            Context& mContext;
            const Type_Code *mCode;
            int mCodeSize;
            int mPC;
            
        public:
        
            Runtime (Context& context);
        
            int getPC() const;
            ///< return program counter.
        
            void configure (const Interpreter::Type_Code *code, int codeSize);
            ///< \a context and \a code must exist as least until either configure, clear or
            /// the destructor is called. \a codeSize is given in 32-bit words.
            
            void clear();
            
            void setPC (int PC);
            ///< set program counter.
    
    };
}

#endif
