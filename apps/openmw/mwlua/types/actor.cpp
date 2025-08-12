#include "types.hpp"

#include <array>

#include <components/detournavigator/agentbounds.hpp>
#include <components/lua/luastate.hpp>
#include <components/settings/values.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/mechanicsmanager.hpp"
#include "apps/openmw/mwbase/windowmanager.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwmechanics/actorutil.hpp"
#include "apps/openmw/mwmechanics/creaturestats.hpp"
#include "apps/openmw/mwmechanics/drawstate.hpp"
#include "apps/openmw/mwworld/class.hpp"
#include "apps/openmw/mwworld/inventorystore.hpp"
#include "apps/openmw/mwworld/worldmodel.hpp"

#include "../localscripts.hpp"
#include "../luamanagerimp.hpp"
#include "../magicbindings.hpp"
#include "../stats.hpp"

namespace MWLua
{
    using EquipmentItem = std::variant<std::string, ObjectId>;
    using Equipment = std::map<int, EquipmentItem>;
    static constexpr int sAnySlot = -1;

    static std::pair<MWWorld::ContainerStoreIterator, bool> findInInventory(
        MWWorld::InventoryStore& store, const EquipmentItem& item, int slot = sAnySlot)
    {
        auto oldIt = slot != sAnySlot ? store.getSlot(slot) : store.end();
        MWWorld::Ptr itemPtr;

        if (std::holds_alternative<ObjectId>(item))
        {
            itemPtr = MWBase::Environment::get().getWorldModel()->getPtr(std::get<ObjectId>(item));
            if (oldIt != store.end() && *oldIt == itemPtr)
                return { oldIt, true }; // already equipped
            if (itemPtr.isEmpty() || itemPtr.getCellRef().getCount() == 0
                || itemPtr.getContainerStore() != static_cast<const MWWorld::ContainerStore*>(&store))
            {
                Log(Debug::Warning) << "Object" << std::get<ObjectId>(item).toString() << " is not in inventory";
                return { store.end(), false };
            }
        }
        else
        {
            const auto& stringId = std::get<std::string>(item);
            ESM::RefId recordId = ESM::RefId::deserializeText(stringId);
            if (oldIt != store.end() && oldIt->getCellRef().getRefId() == recordId)
                return { oldIt, true }; // already equipped
            itemPtr = store.search(recordId);
            if (itemPtr.isEmpty() || itemPtr.getCellRef().getCount() == 0)
            {
                Log(Debug::Warning) << "There is no object with recordId='" << stringId << "' in inventory";
                return { store.end(), false };
            }
        }

        // TODO: Refactor InventoryStore to accept Ptr and get rid of this linear search.
        MWWorld::ContainerStoreIterator it = std::find(store.begin(), store.end(), itemPtr);
        if (it == store.end()) // should never happen
            throw std::logic_error("Item not found in container");

        return { it, false };
    }

    static void setEquipment(const MWWorld::Ptr& actor, const Equipment& equipment)
    {
        bool isPlayer = actor == MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        std::array<bool, MWWorld::InventoryStore::Slots> usedSlots;
        std::fill(usedSlots.begin(), usedSlots.end(), false);

        auto tryEquipToSlot = [&store, &usedSlots, isPlayer](int slot, const EquipmentItem& item) -> bool {
            auto [it, alreadyEquipped] = findInInventory(store, item, slot);
            if (alreadyEquipped)
                return true;
            if (it == store.end())
                return false;
            MWWorld::Ptr itemPtr = *it;

            auto [allowedSlots, _] = itemPtr.getClass().getEquipmentSlots(itemPtr);
            bool requestedSlotIsAllowed
                = std::find(allowedSlots.begin(), allowedSlots.end(), slot) != allowedSlots.end();
            if (!requestedSlotIsAllowed)
            {
                auto firstAllowed
                    = std::find_if(allowedSlots.begin(), allowedSlots.end(), [&](int s) { return !usedSlots[s]; });
                if (firstAllowed == allowedSlots.end())
                {
                    Log(Debug::Warning) << "No suitable slot for " << itemPtr.toString();
                    return false;
                }
                slot = *firstAllowed;
            }

            bool skipEquip = false;

            if (isPlayer)
            {
                const ESM::RefId& script = itemPtr.getClass().getScript(itemPtr);
                if (!script.empty())
                {
                    MWScript::Locals& locals = itemPtr.getRefData().getLocals();
                    locals.setVarByInt(script, "onpcequip", 1);
                    skipEquip = locals.getIntVar(script, "pcskipequip") == 1;
                }
            }

            if (!skipEquip)
                store.equip(slot, it);

            return requestedSlotIsAllowed; // return true if equipped to requested slot and false if slot was changed
        };

        for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            auto oldIt = store.getSlot(slot);
            auto newIt = equipment.find(slot);
            if (newIt == equipment.end())
            {
                if (oldIt != store.end())
                    store.unequipSlot(slot);
                continue;
            }
            if (tryEquipToSlot(slot, newIt->second))
                usedSlots[slot] = true;
        }
        for (const auto& [slot, item] : equipment)
            if (slot >= MWWorld::InventoryStore::Slots)
                tryEquipToSlot(sAnySlot, item);
    }

    static void setSelectedEnchantedItem(const MWWorld::Ptr& actor, const EquipmentItem& item)
    {
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        // We're not passing in a specific slot, so ignore the already equipped return value
        auto [it, _] = findInInventory(store, item, sAnySlot);
        if (it == store.end())
            return;

        MWWorld::Ptr itemPtr = *it;

        // Equip the item if applicable
        auto slots = itemPtr.getClass().getEquipmentSlots(itemPtr);
        if (!slots.first.empty())
        {
            bool alreadyEquipped = false;
            for (auto slot : slots.first)
            {
                if (store.getSlot(slot) == it)
                    alreadyEquipped = true;
            }

            if (!alreadyEquipped)
            {
                MWBase::Environment::get().getWindowManager()->useItem(itemPtr);
                // make sure that item was successfully equipped
                if (!store.isEquipped(itemPtr))
                    return;
            }
        }

        store.setSelectedEnchantItem(it);
        // to reset WindowManager::mSelectedSpell immediately
        MWBase::Environment::get().getWindowManager()->setSelectedEnchantItem(*it);
    }

    void addActorBindings(sol::table actor, const Context& context)
    {
        sol::state_view lua = context.sol();
        actor["STANCE"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, MWMechanics::DrawState>(lua,
                {
                    { "Nothing", MWMechanics::DrawState::Nothing },
                    { "Weapon", MWMechanics::DrawState::Weapon },
                    { "Spell", MWMechanics::DrawState::Spell },
                }));
        actor["EQUIPMENT_SLOT"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            { { "Helmet", MWWorld::InventoryStore::Slot_Helmet }, { "Cuirass", MWWorld::InventoryStore::Slot_Cuirass },
                { "Greaves", MWWorld::InventoryStore::Slot_Greaves },
                { "LeftPauldron", MWWorld::InventoryStore::Slot_LeftPauldron },
                { "RightPauldron", MWWorld::InventoryStore::Slot_RightPauldron },
                { "LeftGauntlet", MWWorld::InventoryStore::Slot_LeftGauntlet },
                { "RightGauntlet", MWWorld::InventoryStore::Slot_RightGauntlet },
                { "Boots", MWWorld::InventoryStore::Slot_Boots }, { "Shirt", MWWorld::InventoryStore::Slot_Shirt },
                { "Pants", MWWorld::InventoryStore::Slot_Pants }, { "Skirt", MWWorld::InventoryStore::Slot_Skirt },
                { "Robe", MWWorld::InventoryStore::Slot_Robe }, { "LeftRing", MWWorld::InventoryStore::Slot_LeftRing },
                { "RightRing", MWWorld::InventoryStore::Slot_RightRing },
                { "Amulet", MWWorld::InventoryStore::Slot_Amulet }, { "Belt", MWWorld::InventoryStore::Slot_Belt },
                { "CarriedRight", MWWorld::InventoryStore::Slot_CarriedRight },
                { "CarriedLeft", MWWorld::InventoryStore::Slot_CarriedLeft },
                { "Ammunition", MWWorld::InventoryStore::Slot_Ammunition } }));

        actor["getStance"] = [](const Object& o) {
            const MWWorld::Class& cls = o.ptr().getClass();
            if (cls.isActor())
                return cls.getCreatureStats(o.ptr()).getDrawState();
            else
                throw std::runtime_error("Actor expected");
        };
        actor["stance"] = actor["getStance"]; // for compatibility; should be removed later
        actor["setStance"] = [](const SelfObject& self, int stance) {
            const MWWorld::Class& cls = self.ptr().getClass();
            if (!cls.isActor())
                throw std::runtime_error("Actor expected");
            auto& stats = cls.getCreatureStats(self.ptr());
            if (stance != static_cast<int>(MWMechanics::DrawState::Nothing)
                && stance != static_cast<int>(MWMechanics::DrawState::Weapon)
                && stance != static_cast<int>(MWMechanics::DrawState::Spell))
            {
                throw std::runtime_error("Incorrect stance");
            }
            MWMechanics::DrawState newDrawState = static_cast<MWMechanics::DrawState>(stance);
            if (stats.getDrawState() == newDrawState)
                return;
            if (newDrawState == MWMechanics::DrawState::Spell)
            {
                bool hasSelectedSpell;
                if (self.ptr() == MWBase::Environment::get().getWorld()->getPlayerPtr())
                    // For the player selecting spell in UI doesn't change selected spell in CreatureStats (was
                    // implemented this way to prevent changing spell during casting, probably should be refactored), so
                    // we have to handle the player separately.
                    hasSelectedSpell = !MWBase::Environment::get().getWindowManager()->getSelectedSpell().empty();
                else
                    hasSelectedSpell = !stats.getSpells().getSelectedSpell().empty();
                if (!hasSelectedSpell)
                {
                    if (!cls.hasInventoryStore(self.ptr()))
                        return; // No selected spell and no items; can't use magic stance.
                    MWWorld::InventoryStore& store = cls.getInventoryStore(self.ptr());
                    if (store.getSelectedEnchantItem() == store.end())
                        return; // No selected spell and no selected enchanted item; can't use magic stance.
                }
            }
            MWBase::MechanicsManager* mechanics = MWBase::Environment::get().getMechanicsManager();
            // We want to interrupt animation only if attack is preparing, but still is not triggered.
            // Otherwise we will get a "speedshooting" exploit, when player can skip reload animation by hitting "Toggle
            // Weapon" key twice.
            if (mechanics->isAttackPreparing(self.ptr()))
                stats.setAttackingOrSpell(false); // interrupt attack
            else if (mechanics->isAttackingOrSpell(self.ptr()))
                return; // can't be interrupted; ignore setStance
            stats.setDrawState(newDrawState);
        };

        actor["getSelectedEnchantedItem"] = [](sol::this_state lua, const Object& o) -> sol::object {
            const MWWorld::Ptr& ptr = o.ptr();
            if (!ptr.getClass().hasInventoryStore(ptr))
                return sol::nil;
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            auto it = store.getSelectedEnchantItem();
            if (it == store.end())
                return sol::nil;
            MWBase::Environment::get().getWorldModel()->registerPtr(*it);
            if (dynamic_cast<const GObject*>(&o))
                return sol::make_object(lua, GObject(*it));
            else
                return sol::make_object(lua, LObject(*it));
        };
        actor["setSelectedEnchantedItem"] = [context](const SelfObject& obj, const sol::object& item) {
            const MWWorld::Ptr& ptr = obj.ptr();
            if (!ptr.getClass().hasInventoryStore(ptr))
                return;

            EquipmentItem ei;
            if (item.is<Object>())
            {
                ei = LuaUtil::cast<Object>(item).id();
            }
            else
            {
                ei = LuaUtil::cast<std::string>(item);
            }
            context.mLuaManager->addAction(
                [obj = Object(ptr), ei = std::move(ei)] { setSelectedEnchantedItem(obj.ptr(), ei); },
                "setSelectedEnchantedItemAction");
        };

        actor["canMove"] = [](const Object& o) {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getMaxSpeed(o.ptr()) > 0;
        };
        actor["getRunSpeed"] = [](const Object& o) {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getRunSpeed(o.ptr());
        };
        actor["getWalkSpeed"] = [](const Object& o) {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getWalkSpeed(o.ptr());
        };
        actor["getCurrentSpeed"] = [](const Object& o) {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getCurrentSpeed(o.ptr());
        };

        // for compatibility; should be removed later
        actor["runSpeed"] = actor["getRunSpeed"];
        actor["walkSpeed"] = actor["getWalkSpeed"];
        actor["currentSpeed"] = actor["getCurrentSpeed"];

        actor["isOnGround"]
            = [](const LObject& o) { return MWBase::Environment::get().getWorld()->isOnGround(o.ptr()); };
        actor["isSwimming"]
            = [](const LObject& o) { return MWBase::Environment::get().getWorld()->isSwimming(o.ptr()); };

        actor["inventory"] = sol::overload([](const LObject& o) { return Inventory<LObject>{ o }; },
            [](const GObject& o) { return Inventory<GObject>{ o }; });
        auto getAllEquipment = [](sol::this_state lua, const Object& o) {
            const MWWorld::Ptr& ptr = o.ptr();
            sol::table equipment(lua, sol::create);
            if (!ptr.getClass().hasInventoryStore(ptr))
                return equipment;

            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
            {
                auto it = store.getSlot(slot);
                if (it == store.end())
                    continue;
                MWBase::Environment::get().getWorldModel()->registerPtr(*it);
                if (dynamic_cast<const GObject*>(&o))
                    equipment[slot] = sol::make_object(lua, GObject(*it));
                else
                    equipment[slot] = sol::make_object(lua, LObject(*it));
            }
            return equipment;
        };
        auto getEquipmentFromSlot = [](sol::this_state lua, const Object& o, int slot) -> sol::object {
            const MWWorld::Ptr& ptr = o.ptr();
            if (!ptr.getClass().hasInventoryStore(ptr))
                return sol::nil;
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            auto it = store.getSlot(slot);
            if (it == store.end())
                return sol::nil;
            MWBase::Environment::get().getWorldModel()->registerPtr(*it);
            if (dynamic_cast<const GObject*>(&o))
                return sol::make_object(lua, GObject(*it));
            else
                return sol::make_object(lua, LObject(*it));
        };
        actor["getEquipment"] = sol::overload(getAllEquipment, getEquipmentFromSlot);
        actor["equipment"] = actor["getEquipment"]; // for compatibility; should be removed later
        actor["hasEquipped"] = [](const Object& o, const Object& item) {
            const MWWorld::Ptr& ptr = o.ptr();
            if (!ptr.getClass().hasInventoryStore(ptr))
                return false;
            MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
            return store.isEquipped(item.ptr());
        };
        actor["setEquipment"] = [context](const SelfObject& obj, const sol::table& equipment) {
            const MWWorld::Ptr& ptr = obj.ptr();
            if (!ptr.getClass().hasInventoryStore(ptr))
            {
                if (!equipment.empty())
                    throw std::runtime_error(obj.toString() + " has no equipment slots");
                return;
            }
            Equipment eqp;
            for (auto& [key, value] : equipment)
            {
                int slot = LuaUtil::cast<int>(key);
                if (value.is<Object>())
                    eqp[slot] = LuaUtil::cast<Object>(value).id();
                else
                    eqp[slot] = LuaUtil::cast<std::string>(value);
            }
            context.mLuaManager->addAction(
                [obj = Object(ptr), eqp = std::move(eqp)] { setEquipment(obj.ptr(), eqp); }, "SetEquipmentAction");
        };
        actor["getPathfindingAgentBounds"] = [](sol::this_state lua, const LObject& o) {
            const DetourNavigator::AgentBounds agentBounds
                = MWBase::Environment::get().getWorld()->getPathfindingAgentBounds(o.ptr());
            sol::table result(lua, sol::create);
            result["shapeType"] = agentBounds.mShapeType;
            result["halfExtents"] = agentBounds.mHalfExtents;
            return result;
        };
        actor["isInActorsProcessingRange"] = [](const Object& o) {
            const MWWorld::Ptr player = MWMechanics::getPlayer();
            const auto& target = o.ptr();
            if (target == player)
                return true;

            if (!target.getClass().isActor())
                throw std::runtime_error("Actor expected");

            if (target.getCell()->getCell()->getWorldSpace() != player.getCell()->getCell()->getWorldSpace())
                return false;

            const int actorsProcessingRange = Settings::game().mActorsProcessingRange;
            const osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();

            const float dist = (playerPos - target.getRefData().getPosition().asVec3()).length();
            return dist <= actorsProcessingRange;
        };

        actor["isDead"] = [](const Object& o) {
            const auto& target = o.ptr();
            return target.getClass().getCreatureStats(target).isDead();
        };

        actor["isDeathFinished"] = [](const Object& o) {
            const auto& target = o.ptr();
            return target.getClass().getCreatureStats(target).isDeathAnimationFinished();
        };

        actor["getEncumbrance"] = [](const Object& actor) -> float {
            const MWWorld::Ptr ptr = actor.ptr();
            return ptr.getClass().getEncumbrance(ptr);
        };

        actor["getCapacity"] = [](const Object& actor) -> float {
            const MWWorld::Ptr ptr = actor.ptr();
            return ptr.getClass().getCapacity(ptr);
        };

        actor["_onHit"] = [context](const SelfObject& self, const sol::table& options) {
            sol::optional<sol::table> damageLua = options.get<sol::optional<sol::table>>("damage");
            std::map<std::string, float> damageCpp;
            if (damageLua)
            {
                for (auto& [key, value] : damageLua.value())
                {
                    damageCpp[key.as<std::string>()] = value.as<float>();
                }
            }
            std::string sourceTypeStr = options.get_or<std::string>("sourceType", "unspecified");
            MWMechanics::DamageSourceType sourceType = MWMechanics::DamageSourceType::Unspecified;
            if (sourceTypeStr == "melee")
                sourceType = MWMechanics::DamageSourceType::Melee;
            else if (sourceTypeStr == "ranged")
                sourceType = MWMechanics::DamageSourceType::Ranged;
            else if (sourceTypeStr == "magic")
                sourceType = MWMechanics::DamageSourceType::Magical;
            sol::optional<Object> weapon = options.get<sol::optional<Object>>("weapon");
            sol::optional<Object> ammo = options.get<sol::optional<Object>>("ammo");

            context.mLuaManager->addAction(
                [self = Object(self), damages = std::move(damageCpp),
                    attacker = options.get<sol::optional<Object>>("attacker"), weapon = ammo ? ammo : weapon,
                    successful = options.get<bool>("successful"), sourceType = sourceType] {
                    MWWorld::Ptr attackerPtr;
                    MWWorld::Ptr weaponPtr;
                    if (attacker)
                        attackerPtr = attacker->ptr();
                    if (weapon)
                        weaponPtr = weapon->ptr();
                    self.ptr().getClass().onHit(self.ptr(), damages, weaponPtr, attackerPtr, successful, sourceType);
                },
                "HitAction");
        };

        addActorStatsBindings(actor, context);
        addActorMagicBindings(actor, context);
    }

}
