#include "scope.hpp"

#include <stdexcept>

#include <components/misc/stringops.hpp>

CSMWorld::Scope CSMWorld::getScopeFromId (const std::string& id)
{
    // get root namespace
    std::string namespace_;

    std::string::size_type i = id.find ("::");

    if (i!=std::string::npos)
        namespace_ = Misc::StringUtils::lowerCase (id.substr (0, i));

    if (namespace_=="project")
        return Scope_Project;

    if (namespace_=="session")
        return Scope_Session;

    return Scope_Content;
}
