#ifndef OPENMW_APPS_OPENMW_MWLUA_TYPES_MODELPROPERTY_H
#define OPENMW_APPS_OPENMW_MWLUA_TYPES_MODELPROPERTY_H

#include <components/esm/path.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

#include <sol/property.hpp>
#include <sol/usertype.hpp>

#include "../contentbindings.hpp"

namespace MWLua
{
    template <class T>
    struct ModelPropertyImpl
    {
        std::string operator()(const T& recordValue) const
        {
            return Misc::ResourceHelpers::correctMeshPath(recordValue.mModel.getNormalized()).value();
        }
    };

    template <class T>
    void addModelProperty(sol::usertype<T>& recordType)
    {
        recordType["model"] = sol::readonly_property(ModelPropertyImpl<T>{});
    }

    template <class T>
    void addMutableModelProperty(sol::usertype<MutableRecord<T>>& recordType)
    {
        recordType["model"] = sol::property(
            [](const MutableRecord<T>& mutRec) -> std::string { return ModelPropertyImpl<T>{}(mutRec.find()); },
            [](MutableRecord<T>& mutRec, std::string_view path) {
                T& recordValue = mutRec.find();
                recordValue.mModel = Misc::ResourceHelpers::meshPathForESM3(path);
            });
    }
}

#endif
