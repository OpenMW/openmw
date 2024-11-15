#ifndef OPENMW_APPS_OPENMW_MWLUA_TYPES_MODELPROPERTY_H
#define OPENMW_APPS_OPENMW_MWLUA_TYPES_MODELPROPERTY_H

#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

#include <sol/property.hpp>
#include <sol/usertype.hpp>

namespace MWLua
{
    template <class T>
    void addModelProperty(sol::usertype<T>& recordType)
    {
        recordType["model"] = sol::readonly_property([](const T& recordValue) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(recordValue.mModel)).value();
        });
    }
}

#endif
