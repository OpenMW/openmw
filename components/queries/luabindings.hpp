#include <limits> // missing from sol/sol.hpp
#include <sol/sol.hpp>

#include "query.hpp"

namespace Queries
{
    void registerQueryBindings(sol::state& lua);
}
