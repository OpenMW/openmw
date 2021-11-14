#include "actions.hpp"

#include <cstring>

#include <components/debug/debuglog.hpp>
#include <components/lua/luastate.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"

namespace MWLua
{

#ifdef NDEBUG
    Action::Action(LuaUtil::LuaState* state) {}
#else
    Action::Action(LuaUtil::LuaState* state) : mCallerTraceback(state->debugTraceback()) {}
#endif

    void Action::safeApply(WorldView& w) const
    {
        try
        {
            apply(w);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Error in " << this->toString() << ": " << e.what();
#ifdef NDEBUG
            Log(Debug::Error) << "Traceback is available only in debug builds";
#else
            Log(Debug::Error) << "Caller " << mCallerTraceback;
#endif
        }
    }

    void TeleportAction::apply(WorldView& worldView) const
    {
        MWWorld::CellStore* cell = worldView.findCell(mCell, mPos);
        if (!cell)
            throw std::runtime_error(std::string("cell not found: '") + mCell + "'");

        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr obj = worldView.getObjectRegistry()->getPtr(mObject, false);
        const MWWorld::Class& cls = obj.getClass();
        bool isPlayer = obj == world->getPlayerPtr();
        if (cls.isActor())
            cls.getCreatureStats(obj).land(isPlayer);
        if (isPlayer)
        {
            ESM::Position esmPos;
            static_assert(sizeof(esmPos) == sizeof(osg::Vec3f) * 2);
            std::memcpy(esmPos.pos, &mPos, sizeof(osg::Vec3f));
            std::memcpy(esmPos.rot, &mRot, sizeof(osg::Vec3f));
            world->getPlayer().setTeleported(true);
            if (cell->isExterior())
                world->changeToExteriorCell(esmPos, true);
            else
                world->changeToInteriorCell(mCell, esmPos, true);
        }
        else
        {
            MWWorld::Ptr newObj = world->moveObject(obj, cell, mPos);
            world->rotateObject(newObj, mRot);
        }
    }

    void SetEquipmentAction::apply(WorldView& worldView) const
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
                if (old_it != store.end() && old_it->getCellRef().getRefIdRef() == recordId)
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

}
