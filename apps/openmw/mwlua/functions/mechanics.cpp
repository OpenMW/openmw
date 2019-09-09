#include "mechanics.hpp"

#include "../sol.hpp"
#include "../luamanager.hpp"
#include "../luautil.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/journal.hpp"
#include "../../mwbase/statemanager.hpp"
#include "../../mwbase/mechanicsmanager.hpp"
#include "../../mwbase/windowmanager.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwmechanics/actorutil.hpp"
#include "../../mwmechanics/creaturestats.hpp"
#include "../../mwmechanics/aicast.hpp"
#include "../../mwmechanics/spellcasting.hpp"

#include "../../mwworld/class.hpp"
#include "../../mwworld/inventorystore.hpp"
#include "../../mwworld/esmstore.hpp"

namespace mwse
{
    namespace lua
    {
        void bindTES3Mechanics()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            state["omw"]["getPlayer"] = []() -> sol::object
            {
                return makeLuaObject(MWMechanics::getPlayer());
            };

            state["omw"]["getPlayerTarget"] = []() -> sol::object
            {
                return makeLuaObject(MWBase::Environment::get().getWorld()->getFacedObject());
            };

            state["omw"]["getReference"] = [](sol::optional<const char*> id)
            {
                if (id)
                {
                    return makeLuaObject(MWBase::Environment::get().getWorld()->getPtr(id.value(), false));
                }
                else
                {
                    return makeLuaObject(LuaManager::getInstance().getCurrentReference());
                }
            };

            state["omw"]["saveGame"] = [](sol::optional<sol::table> params)
            {
                std::string fileName(getOptionalParam<const char*>(params, "file", ""));
                MWBase::StateManager::State state = MWBase::Environment::get().getStateManager()->getState();
                if (state != MWBase::StateManager::State_Running)
                    return false;

                if (fileName.empty())
                    MWBase::Environment::get().getStateManager()->quickSave();
                else
                    MWBase::Environment::get().getStateManager()->saveGame(fileName);
                return true;
            };

            state["omw"]["loadGame"] = [](const char* fileName)
            {
                MWBase::Environment::get().getStateManager()->simpleGameLoad(fileName);
            };

            state["omw"]["newGame"] = []()
            {
                MWBase::Environment::get().getStateManager()->newGame(false);
            };

            state["omw"]["is3rdPerson"] = []()
            {
                return !MWBase::Environment::get().getWorld()->isFirstPerson();
            };

            state["omw"]["getPlayerGold"] = []() -> int
            {
                MWWorld::Ptr player = MWMechanics::getPlayer();
                return player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);
            };

            state["omw"]["triggerCrime"] = [](sol::table params)
            {
                MWBase::MechanicsManager::OffenseType crimeType = MWBase::MechanicsManager::OT_Theft;

                // Look at the given type.
                int crimeTypeInt = getOptionalParam<int>(params, "type", 3);
                if (crimeTypeInt < 1 || crimeTypeInt > 7)
                {
                    throw std::invalid_argument("Invalid type given. Value must be between 1 and 7.");
                }

                switch (crimeTypeInt)
                {
                case 1:
                    crimeType = MWBase::MechanicsManager::OT_Assault;
                    break;
                case 2:
                    crimeType = MWBase::MechanicsManager::OT_Murder;
                    break;
                case 3:
                    break;
                case 4:
                    crimeType = MWBase::MechanicsManager::OT_Pickpocket;
                    break;
                case 5:
                    break;
                case 6:
                    crimeType = MWBase::MechanicsManager::OT_Trespassing;
                    break;
                case 7:
                    // FIXME: a special case in the MechanicsManager::setWerewolf()
                    //crimeEvent.typeString = "werewolf";
                    break;
                }

                // FIXME: penalty works only for theft now
                int penalty = getOptionalParam<int>(params, "value", 0);

                MWWorld::Ptr victim = getOptionalParamReference(params, "victim");

                MWBase::Environment::get().getMechanicsManager()->commitCrime(MWMechanics::getPlayer(), victim, crimeType, penalty, true);
            };

            state["omw"]["getLocked"] = [](sol::table params) -> bool
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return false;

                bool isLocked = ptr.getCellRef().getLockLevel() > 0;
                return isLocked;
            };

            state["omw"]["setLockLevel"] = [](sol::table params) -> bool
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return false;

                int level = getOptionalParam<int>(params, "level", -1);
                if (level >= 0)
                {
                    ptr.getClass().lock(ptr, level);
                    return true;
                }

                return false;
            };

            state["omw"]["getLockLevel"] = [](sol::table params) -> sol::optional<int>
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return sol::optional<int>();

                return ptr.getCellRef().getLockLevel();
            };

            state["omw"]["lock"] = [](sol::table params) -> bool
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return false;

                // Set the lock level if one was provided.
                int lockLevel = getOptionalParam<int>(params, "level", ptr.getCellRef().getLockLevel());

                if(ptr.getCellRef().getLockLevel() == 0)
                {
                    //no lock level was ever set, set to 100 as default
                    lockLevel = 100;
                }

                ptr.getClass().lock (ptr, lockLevel);

                // Instantly reset door to closed state
                // This is done when using Lock in scripts, but not when using Lock spells.
                if (ptr.getTypeName() == typeid(ESM::Door).name() && !ptr.getCellRef().getTeleport())
                {
                    MWBase::Environment::get().getWorld()->activateDoor(ptr, MWWorld::DoorState::Idle);
                }

                return true;
            };

            state["omw"]["unlock"] = [](sol::table params) -> bool
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return false;

                ptr.getClass().unlock (ptr);
                return true;
            };

            state["omw"]["getTrap"] = [](sol::table params) -> sol::optional<std::string>
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return std::string();

                return ptr.getCellRef().getTrap();
            };

            state["omw"]["setTrap"] = [](sol::table params) -> bool
            {
                const char* spell = getOptionalParam<const char*>(params, "spell", nullptr);
                if (spell == nullptr)
                    return false;

                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty() || !ptr.getClass().canLock(ptr))
                    return false;

                ptr.getCellRef().setTrap(spell);
                return true;
            };

            state["omw"]["checkMerchantTradesItem"] = [](sol::table params) -> bool
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty())
                {
                    throw std::invalid_argument("Invalid reference parameter provided: Can't be empty.");
                }

                //FIXME: support for Id parameter
                /*
                TES3::Item* item = getOptionalParamObject<TES3::Item>(params, "item");
                if (item == nullptr) {
                    throw std::invalid_argument("Invalid item parameter provided: Can't be nil.");
                }
                */

                if (!ptr.getClass().isActor())
                {
                    throw std::invalid_argument("Invalid reference parameter provided: Base object must be an actor.");
                }

                MWWorld::Ptr item = getOptionalParamReference(params, "item");
                if (item.isEmpty())
                {
                    throw std::invalid_argument("Invalid item parameter provided: Can't be empty.");
                }

                int services = ptr.getClass().getServices(ptr);
                return item.getClass().canSell(item, services);
            };

            state["omw"]["getJournalIndex"] = [](const char* quest) -> sol::optional<int>
            {
                int index = MWBase::Environment::get().getJournal()->getJournalIndex (Misc::StringUtils::lowerCase(quest));
                return index;
            };

            state["omw"]["setJournalIndex"] = [](sol::table params) -> bool
            {
                sol::optional<std::string> id = params["id"];
                sol::optional<int> index = params["index"];
                if (!id || !index)
                {
                    return false;
                }

                MWBase::Environment::get().getJournal()->setJournalIndex (Misc::StringUtils::lowerCase(id.value()), index.value());

                return true;
            };

            state["omw"]["updateJournal"] = [](sol::table params) -> bool
            {
                sol::optional<std::string> id = params["id"];
                sol::optional<int> index = params["index"];
                if (!id || !index)
                {
                    return false;
                }

                MWWorld::Ptr actor = getOptionalParamReference(params, "speaker");
                if (!actor.isEmpty())
                {
                    actor = MWMechanics::getPlayer();
                }

                // Invoking Journal with a non-existing index is allowed, and triggers no errors. Seriously? :(
                try
                {
                    MWBase::Environment::get().getJournal()->addEntry (Misc::StringUtils::lowerCase(id.value()), index.value(), actor);
                }
                catch (...)
                {
                    if (MWBase::Environment::get().getJournal()->getJournalIndex(Misc::StringUtils::lowerCase(id.value())) < index.value())
                        MWBase::Environment::get().getJournal()->setJournalIndex(Misc::StringUtils::lowerCase(id.value()), index.value());
                }

                return true;
            };

            state["omw"]["cast"] = [](sol::table params)
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty())
                {
                    throw std::invalid_argument("Invalid reference parameter provided.");
                }

                // FIXME: references support
                std::string spellId = params["spell"];

                std::string targetId = params["target"];

                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId);
                if (!spell)
                {
                    throw std::invalid_argument("Invalid spell parameter provided.");
                }

                if (spell->mData.mType != ESM::Spell::ST_Spell && spell->mData.mType != ESM::Spell::ST_Power)
                {
                    throw std::invalid_argument("Invalid spell parameter provided.");
                }

                if (ptr == MWMechanics::getPlayer())
                {
                    MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
                    store.setSelectedEnchantItem(store.end());
                    MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, ptr)));
                    MWBase::Environment::get().getWindowManager()->updateSpellWindow();
                    return true;
                }

                if (ptr.getClass().isActor())
                {
                    MWMechanics::AiCast castPackage(targetId, spellId, true);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(castPackage, ptr);

                    return true;
                }

                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr (targetId, false);

                MWMechanics::CastSpell cast(ptr, target, false, true);
                cast.playSpellCastingEffects(spell->mId, false);
                cast.mHitPosition = target.getRefData().getPosition().asVec3();
                cast.mAlwaysSucceed = true;
                cast.cast(spell);

                return true;
            };

            state["omw"]["hasOwnershipAccess"] = [](sol::table params)
            {
                // Who are we checking ownership for? The player by default.
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty())
                {
                    ptr = MWMechanics::getPlayer();
                }

                // What are we checking ownership of?
                MWWorld::Ptr target = getOptionalParamReference(params, "target");
                if (target.isEmpty())
                {
                    throw std::invalid_argument("Invalid target parameter provided.");
                }

                MWWorld::Ptr victim;
                return MWBase::Environment::get().getMechanicsManager()->isAllowedToUse(ptr, target, victim);
            };

            state["omw"]["setEnabled"] = [](sol::table params)
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty())
                {
                    throw std::invalid_argument("Invalid reference parameter provided.");
                }

                // Allow toggling.
                if (getOptionalParam<bool>(params, "toggle", false))
                {
                    if (ptr.getRefData().isEnabled())
                    {
                        MWBase::Environment::get().getWorld()->disable(ptr);
                    }
                    else
                    {
                        MWBase::Environment::get().getWorld()->enable(ptr);
                    }
                }
                // Otherwise base it on enabled (default true).
                else
                {
                    if (getOptionalParam<bool>(params, "enabled", true))
                    {
                        MWBase::Environment::get().getWorld()->enable(ptr);
                    }
                    else
                    {
                        MWBase::Environment::get().getWorld()->disable(ptr);
                    }
                }
            };

            state["omw"]["playAnimation"] = [](sol::table params)
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty())
                {
                    throw std::invalid_argument("Invalid reference parameter provided.");
                }

                if (!ptr.getRefData().isEnabled())
                    return;

                const char* group = getOptionalParam<const char*>(params, "group", nullptr);
                if (group == nullptr)
                {
                    throw std::invalid_argument("Invalid 'group' parameter provided: must be not empty.");
                }

                int mode = getOptionalParam<int>(params, "startFlag", 0);
                int loopCount = getOptionalParam<int>(params, "loopCount", std::numeric_limits<int>::max());

                MWBase::Environment::get().getMechanicsManager()->playAnimationGroup (ptr, group, mode, loopCount, true);
            };

            state["omw"]["skipAnimationFrame"] = [](sol::table params)
            {
                MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
                if (ptr.isEmpty())
                {
                    throw std::invalid_argument("Invalid reference parameter provided.");
                }

                MWBase::Environment::get().getMechanicsManager()->skipAnimation (ptr);
            };
        }
    }
}
