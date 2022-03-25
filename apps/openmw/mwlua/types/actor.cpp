#include "types.hpp"

#include <components/lua/luastate.hpp>

#include <apps/openmw/mwmechanics/drawstate.hpp>
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <apps/openmw/mwworld/class.hpp>

#include "../actions.hpp"
#include "../luabindings.hpp"
#include "../localscripts.hpp"
#include "../luamanagerimp.hpp"
#include "../stats.hpp"

namespace MWLua
{
    using SelfObject = LocalScripts::SelfObject;

    void addActorBindings(sol::table actor, const Context& context)
    {
        actor["STANCE"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"Nothing", MWMechanics::DrawState_Nothing},
            {"Weapon", MWMechanics::DrawState_Weapon},
            {"Spell", MWMechanics::DrawState_Spell},
        }));
        actor["EQUIPMENT_SLOT"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"Helmet", MWWorld::InventoryStore::Slot_Helmet},
            {"Cuirass", MWWorld::InventoryStore::Slot_Cuirass},
            {"Greaves", MWWorld::InventoryStore::Slot_Greaves},
            {"LeftPauldron", MWWorld::InventoryStore::Slot_LeftPauldron},
            {"RightPauldron", MWWorld::InventoryStore::Slot_RightPauldron},
            {"LeftGauntlet", MWWorld::InventoryStore::Slot_LeftGauntlet},
            {"RightGauntlet", MWWorld::InventoryStore::Slot_RightGauntlet},
            {"Boots", MWWorld::InventoryStore::Slot_Boots},
            {"Shirt", MWWorld::InventoryStore::Slot_Shirt},
            {"Pants", MWWorld::InventoryStore::Slot_Pants},
            {"Skirt", MWWorld::InventoryStore::Slot_Skirt},
            {"Robe", MWWorld::InventoryStore::Slot_Robe},
            {"LeftRing", MWWorld::InventoryStore::Slot_LeftRing},
            {"RightRing", MWWorld::InventoryStore::Slot_RightRing},
            {"Amulet", MWWorld::InventoryStore::Slot_Amulet},
            {"Belt", MWWorld::InventoryStore::Slot_Belt},
            {"CarriedRight", MWWorld::InventoryStore::Slot_CarriedRight},
            {"CarriedLeft", MWWorld::InventoryStore::Slot_CarriedLeft},
            {"Ammunition", MWWorld::InventoryStore::Slot_Ammunition}
        }));

        actor["stance"] = [](const Object& o)
        {
            const MWWorld::Class& cls = o.ptr().getClass();
            if (cls.isActor())
                return cls.getCreatureStats(o.ptr()).getDrawState();
            else
                throw std::runtime_error("Actor expected");
        };

        actor["canMove"] = [](const Object& o)
        {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getMaxSpeed(o.ptr()) > 0;
        };
        actor["runSpeed"] = [](const Object& o)
        {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getRunSpeed(o.ptr());
        };
        actor["walkSpeed"] = [](const Object& o)
        {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getWalkSpeed(o.ptr());
        };
        actor["currentSpeed"] = [](const Object& o)
        {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getCurrentSpeed(o.ptr());
        };

        actor["isOnGround"] = [](const LObject& o)
        {
            return MWBase::Environment::get().getWorld()->isOnGround(o.ptr());
        };
        actor["isSwimming"] = [](const LObject& o)
        {
            return MWBase::Environment::get().getWorld()->isSwimming(o.ptr());
        };

        actor["inventory"] = sol::overload(
            [](const LObject& o) { return Inventory<LObject>{o}; },
            [](const GObject& o) { return Inventory<GObject>{o}; }
        );
        actor["equipment"] = [context](const Object& o)
        {
            const MWWorld::Ptr& ptr = o.ptr();
            sol::table equipment(context.mLua->sol(), sol::create);
            if (!ptr.getClass().hasInventoryStore(ptr))
                return equipment;

            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
            {
                auto it = store.getSlot(slot);
                if (it == store.end())
                    continue;
                context.mWorldView->getObjectRegistry()->registerPtr(*it);
                equipment[slot] = o.getObject(context.mLua->sol(), getId(*it));
            }
            return equipment;
        };
        actor["hasEquipped"] = [](const Object& o, const Object& item)
        {
            const MWWorld::Ptr& ptr = o.ptr();
            if (!ptr.getClass().hasInventoryStore(ptr))
                return false;
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            return store.isEquipped(item.ptr());
        };
        actor["setEquipment"] = [context](const SelfObject& obj, const sol::table& equipment)
        {
            if (!obj.ptr().getClass().hasInventoryStore(obj.ptr()))
            {
                if (!equipment.empty())
                    throw std::runtime_error(ptrToString(obj.ptr()) + " has no equipment slots");
                return;
            }
            SetEquipmentAction::Equipment eqp;
            for (auto& [key, value] : equipment)
            {
                int slot = key.as<int>();
                if (value.is<Object>())
                    eqp[slot] = value.as<Object>().id();
                else
                    eqp[slot] = value.as<std::string>();
            }
            context.mLuaManager->addAction(std::make_unique<SetEquipmentAction>(context.mLua, obj.id(), std::move(eqp)));
        };

        addActorStatsBindings(actor, context);
    }

}
