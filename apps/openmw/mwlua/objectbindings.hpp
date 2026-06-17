#ifndef MWLUA_OBJECTBINDINGS_H
#define MWLUA_OBJECTBINDINGS_H

namespace MWLua
{
    struct Context;

    void initObjectBindingsForLocalScripts(const Context& context);
    void initObjectBindingsForGlobalScripts(const Context& context);
}

#endif // MWLUA_OBJECTBINDINGS_H
