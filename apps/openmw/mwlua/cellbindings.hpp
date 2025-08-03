#ifndef MWLUA_CELLBINDINGS_H
#define MWLUA_CELLBINDINGS_H

namespace MWLua
{
    struct Context;

    void initCellBindingsForLocalScripts(const Context& context);
    void initCellBindingsForGlobalScripts(const Context& context);
}

#endif // MWLUA_CELLBINDINGS_H
