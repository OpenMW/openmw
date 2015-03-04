#ifndef GAME_MWSCRIPT_REF_H
#define GAME_MWSCRIPT_REF_H

#include <string>

#include "../mwworld/ptr.hpp"

namespace Interpreter
{
    class Runtime;
}

namespace MWScript
{
    struct ExplicitRef
    {
        static const bool implicit = false;

        MWWorld::Ptr operator() (Interpreter::Runtime& runtime, bool required = true,
            bool activeOnly = false) const;
    };

    struct ImplicitRef
    {
        static const bool implicit = true;

        MWWorld::Ptr operator() (Interpreter::Runtime& runtime, bool required = true,
            bool activeOnly = false) const;
    };
}

#endif
