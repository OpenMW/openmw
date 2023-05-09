#ifndef MWLUA_CELLBINDINGS_H
#define MWLUA_CELLBINDINGS_H

#include "context.hpp"

namespace MWLua
{
    void initCellBindingsForLocalScripts(const Context&);
    void initCellBindingsForGlobalScripts(const Context&);
}

#endif // MWLUA_CELLBINDINGS_H
