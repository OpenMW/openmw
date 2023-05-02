#ifndef MWLUA_OBJECTBINDINGS_H
#define MWLUA_OBJECTBINDINGS_H

#include "context.hpp"

namespace MWLua
{
    void initObjectBindingsForLocalScripts(const Context&);
    void initObjectBindingsForGlobalScripts(const Context&);
}

#endif // MWLUA_OBJECTBINDINGS_H
