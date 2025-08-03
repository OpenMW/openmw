#ifndef MWLUA_ACTOR_H
#define MWLUA_ACTOR_H

#include <sol/sol.hpp>

#include <components/esm/defs.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/misc/convert.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"

#include "../context.hpp"

#include "servicesoffered.hpp"
namespace MWLua
{

    template <class T>
    void addActorServicesBindings(sol::usertype<T>& record, const Context& context)
    {
        record["servicesOffered"] = sol::readonly_property([context](const T& rec) -> sol::table {
            sol::state_view lua = context.sol();
            sol::table providedServices(lua, sol::create);

            int services = rec.mAiData.mServices;
            if constexpr (std::is_same_v<T, ESM::NPC>)
            {
                if (rec.mFlags & ESM::NPC::Autocalc)
                    services
                        = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(rec.mClass)->mData.mServices;
            }
            for (const auto& [flag, name] : ServiceNames)
            {
                providedServices[name] = (services & flag) != 0;
            }

            providedServices["Travel"] = !rec.getTransport().empty();
            return LuaUtil::makeReadOnly(providedServices);
        });

        record["travelDestinations"] = sol::readonly_property([context](const T& rec) -> sol::table {
            sol::state_view lua = context.sol();
            sol::table travelDests(lua, sol::create);
            if (!rec.getTransport().empty())
            {
                int index = 1;
                for (const auto& dest : rec.getTransport())
                {
                    sol::table travelDest(lua, sol::create);

                    ESM::RefId cellId;
                    if (dest.mCellName.empty())
                    {
                        const ESM::ExteriorCellLocation cellIndex
                            = ESM::positionToExteriorCellLocation(dest.mPos.pos[0], dest.mPos.pos[1]);
                        cellId = ESM::RefId::esm3ExteriorCell(cellIndex.mX, cellIndex.mY);
                    }
                    else
                        cellId = ESM::RefId::stringRefId(dest.mCellName);
                    travelDest["rotation"] = LuaUtil::asTransform(Misc::Convert::makeOsgQuat(dest.mPos.rot));
                    travelDest["position"] = dest.mPos.asVec3();
                    travelDest["cellId"] = cellId.serializeText();

                    travelDests[index] = LuaUtil::makeReadOnly(travelDest);
                    index++;
                }
            }
            return LuaUtil::makeReadOnly(travelDests);
        });
    }
}
#endif // MWLUA_ACTOR_H
