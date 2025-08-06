#include "scope.hpp"

#include <string_view>

#include <components/esm/refid.hpp>
#include <components/misc/strings/algorithm.hpp>

namespace CSMWorld
{
    namespace
    {
        struct GetScope
        {
            Scope operator()(ESM::StringRefId v) const
            {
                std::string_view namespaceValue;

                const std::string::size_type i = v.getValue().find("::");

                if (i != std::string::npos)
                    namespaceValue = std::string_view(v.getValue()).substr(0, i);

                if (Misc::StringUtils::ciEqual(namespaceValue, "project"))
                    return Scope_Project;

                if (Misc::StringUtils::ciEqual(namespaceValue, "session"))
                    return Scope_Session;

                return Scope_Content;
            }

            template <class T>
            Scope operator()(const T& /*v*/) const
            {
                return Scope_Content;
            }
        };
    }
}

CSMWorld::Scope CSMWorld::getScopeFromId(ESM::RefId id)
{
    return visit(GetScope{}, id);
}
