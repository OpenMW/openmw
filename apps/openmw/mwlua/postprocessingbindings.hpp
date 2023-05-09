#ifndef MWLUA_POSTPROCESSINGBINDINGS_H
#define MWLUA_POSTPROCESSINGBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initPostprocessingPackage(const Context&);
}

#endif // MWLUA_POSTPROCESSINGBINDINGS_H
