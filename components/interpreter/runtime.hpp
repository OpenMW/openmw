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
            const Interpreter::Type_Code *mCode;
            int mCodeSize;
            
        public:
        
            Runtime (Context& context);
        
            void configure (const Interpreter::Type_Code *code, int codeSize);
            ///< \a context and \a code must exist as least until either configure, clear or
            /// the destructor is called. \a codeSize is given in 32-bit words.
            
            void clear();
    
    };
}

#endif
