#ifndef OPENMW_APPS_OPENMW_MWLUA_TYPES_MODELPROPERTY_H
#define OPENMW_APPS_OPENMW_MWLUA_TYPES_MODELPROPERTY_H

#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

#include <sol/property.hpp>
#include <sol/usertype.hpp>

#include "../contentbindings.hpp"

namespace MWLua
{
    namespace ModelPropertyImpl
    {
        template <class T>
        std::string getMeshPath(const T& recordValue)
        {
            return Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(recordValue.mModel)).value();
        }
    }

    template <class T>
    void addModelProperty(sol::usertype<T>& recordType)
    {
        recordType["model"] = sol::readonly_property(&ModelPropertyImpl::getMeshPath<T>);
    }

    template <class T>
    void addMutableModelProperty(sol::usertype<MutableRecord<T>>& recordType)
    {
        recordType["model"] = sol::property(
            [](const MutableRecord<T>& mutRec) -> std::string { return ModelPropertyImpl::getMeshPath(mutRec.find()); },
            [](MutableRecord<T>& mutRec, std::string_view path) {
                T& recordValue = mutRec.find();
                recordValue.mModel = Misc::ResourceHelpers::meshPathForESM3(path);
            });
    }
}

#endif
