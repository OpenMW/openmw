#include "types.hpp"

#include <components/lua/luastate.hpp>

#include <apps/openmw/mwmechanics/drawstate.hpp>
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <apps/openmw/mwworld/class.hpp>

#include "../luabindings.hpp"
#include "../localscripts.hpp"
#include "../luamanagerimp.hpp"
#include "../stats.hpp"

namespace MWLua
{
    namespace
    {
        class SetEquipmentAction final : public LuaManager::Action
        {
        public:
            using Item = std::variant<std::string, ObjectId>;  // recordId or ObjectId
            using Equipment = std::map<int, Item>;  // slot to item

            SetEquipmentAction(LuaUtil::LuaState* state, ObjectId actor, Equipment equipment)
                : Action(state), mActor(actor), mEquipment(std::move(equipment)) {}

            void apply(WorldView& worldView) const override
            {
                MWWorld::Ptr actor = worldView.getObjectRegistry()->getPtr(mActor, false);
                MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
                std::array<bool, MWWorld::InventoryStore::Slots> usedSlots;
                std::fill(usedSlots.begin(), usedSlots.end(), false);

                static constexpr int anySlot = -1;
                auto tryEquipToSlot = [&actor, &store, &usedSlots, &worldView](int slot, const Item& item) -> bool
                {
                    auto old_it = slot != anySlot ? store.getSlot(slot) : store.end();
                    MWWorld::Ptr itemPtr;
                    if (std::holds_alternative<ObjectId>(item))
                    {
                        itemPtr = worldView.getObjectRegistry()->getPtr(std::get<ObjectId>(item), false);
                        if (old_it != store.end() && *old_it == itemPtr)
                            return true;  // already equipped
                        if (itemPtr.isEmpty() || itemPtr.getRefData().getCount() == 0 ||
                            itemPtr.getContainerStore() != static_cast<const MWWorld::ContainerStore*>(&store))
                        {
                            Log(Debug::Warning) << "Object" << idToString(std::get<ObjectId>(item)) << " is not in inventory";
                            return false;
                        }
                    }
                    else
                    {
                        const std::string& recordId = std::get<std::string>(item);
                        if (old_it != store.end() && old_it->getCellRef().getRefId() == recordId)
                            return true;  // already equipped
                        itemPtr = store.search(recordId);
                        if (itemPtr.isEmpty() || itemPtr.getRefData().getCount() == 0)
                        {
                            Log(Debug::Warning) << "There is no object with recordId='" << recordId << "' in inventory";
                            return false;
                        }
                    }

                    auto [allowedSlots, _] = itemPtr.getClass().getEquipmentSlots(itemPtr);
                    bool requestedSlotIsAllowed = std::find(allowedSlots.begin(), allowedSlots.end(), slot) != allowedSlots.end();
                    if (!requestedSlotIsAllowed)
                    {
                        auto firstAllowed = std::find_if(allowedSlots.begin(), allowedSlots.end(), [&](int s) { return !usedSlots[s]; });
                        if (firstAllowed == allowedSlots.end())
                        {
                            Log(Debug::Warning) << "No suitable slot for " << ptrToString(itemPtr);
                            return false;
                        }
                        slot = *firstAllowed;
                    }

                    // TODO: Refactor InventoryStore to accept Ptr and get rid of this linear search.
                    MWWorld::ContainerStoreIterator it = std::find(store.begin(), store.end(), itemPtr);
                    if (it == store.end())  // should never happen
                        throw std::logic_error("Item not found in container");

                    store.equip(slot, it, actor);
                    return requestedSlotIsAllowed;  // return true if equipped to requested slot and false if slot was changed
                };

                for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                {
                    auto old_it = store.getSlot(slot);
                    auto new_it = mEquipment.find(slot);
                    if (new_it == mEquipment.end())
                    {
                        if (old_it != store.end())
                            store.unequipSlot(slot, actor);
                        continue;
                    }
                    if (tryEquipToSlot(slot, new_it->second))
                        usedSlots[slot] = true;
                }
                for (const auto& [slot, item] : mEquipment)
                    if (slot >= MWWorld::InventoryStore::Slots)
                        tryEquipToSlot(anySlot, item);
            }

            std::string toString() const override { return "SetEquipmentAction"; }

        private:
            ObjectId mActor;
            Equipment mEquipment;
        };
    }
    
    using SelfObject = LocalScripts::SelfObject;

    void addActorBindings(sol::table actor, const Context& context)
    {
        actor["STANCE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"Nothing", MWMechanics::DrawState_Nothing},
            {"Weapon", MWMechanics::DrawState_Weapon},
            {"Spell", MWMechanics::DrawState_Spell},
        }));
        actor["EQUIPMENT_SLOT"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
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
        auto getAllEquipment = [context](const Object& o)
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
        auto getEquipmentFromSlot = [context](const Object& o, int slot) -> sol::object
        {
            const MWWorld::Ptr& ptr = o.ptr();
            sol::table equipment(context.mLua->sol(), sol::create);
            if (!ptr.getClass().hasInventoryStore(ptr))
                return sol::nil;
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            auto it = store.getSlot(slot);
            if (it == store.end())
                return sol::nil;
            context.mWorldView->getObjectRegistry()->registerPtr(*it);
            return o.getObject(context.mLua->sol(), getId(*it));
        };
        actor["equipment"] = sol::overload(getAllEquipment, getEquipmentFromSlot);
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
