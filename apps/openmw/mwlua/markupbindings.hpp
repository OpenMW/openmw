#ifndef MWLUA_MARKUPBINDINGS_H
#define MWLUA_MARKUPBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initMarkupPackage(const Context&);
}

#endif // MWLUA_MARKUPBINDINGS_H
