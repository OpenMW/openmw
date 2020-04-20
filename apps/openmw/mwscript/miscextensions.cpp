#include "miscextensions.hpp"

#include <cstdlib>
#include <iomanip>

#include <components/compiler/opcodes.hpp>
#include <components/compiler/locals.hpp>

#include <components/debug/debuglog.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include <components/esm/loadmgef.hpp>
#include <components/esm/loadcrea.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwmechanics/aicast.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace
{

    void addToLevList(ESM::LevelledListBase* list, const std::string& itemId, int level)
    {
        for (std::vector<ESM::LevelledListBase::LevelItem>::iterator it = list->mList.begin(); it != list->mList.end(); ++it)
        {
            if (it->mLevel == level && itemId == it->mId)
                return;
        }

        ESM::LevelledListBase::LevelItem item;
        item.mId = itemId;
        item.mLevel = level;
        list->mList.push_back(item);
    }

    void removeFromLevList(ESM::LevelledListBase* list, const std::string& itemId, int level)
    {
        // level of -1 removes all items with that itemId
        for (std::vector<ESM::LevelledListBase::LevelItem>::iterator it = list->mList.begin(); it != list->mList.end();)
        {
            if (level != -1 && it->mLevel != level)
            {
                ++it;
                continue;
            }
            if (Misc::StringUtils::ciEqual(itemId, it->mId))
                it = list->mList.erase(it);
            else
                ++it;
        }
    }

}

namespace MWScript
{
    namespace Misc
    {
        class OpPlayBink : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                std::string name = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                bool allowSkipping = runtime[0].mInteger != 0;
                runtime.pop();

                MWBase::Environment::get().getWindowManager()->playVideo (name, allowSkipping);
            }
        };

        class OpGetPcSleep : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                runtime.push (MWBase::Environment::get().getWindowManager ()->getPlayerSleeping());
            }
        };

        class OpGetPcJumping : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                runtime.push (world->getPlayer().getJumping());
            }
        };

        class OpWakeUpPc : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::Environment::get().getWindowManager ()->wakeUpPlayer();
            }
        };

        class OpXBox : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (0);
                }
        };

        template <class R>
        class OpOnActivate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (ptr.getRefData().onActivate());
                }
        };

        template <class R>
        class OpActivate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = R()(runtime);

                    if (ptr.getRefData().activateByScript())
                        context.executeActivation(ptr, MWMechanics::getPlayer());
                }
        };

        template<class R>
        class OpLock : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer lockLevel = ptr.getCellRef().getLockLevel();
                    if(lockLevel==0) { //no lock level was ever set, set to 100 as default
                        lockLevel = 100;
                    }

                    if (arg0==1)
                    {
                        lockLevel = runtime[0].mInteger;
                        runtime.pop();
                    }

                    ptr.getCellRef().lock (lockLevel);

                    // Instantly reset door to closed state
                    // This is done when using Lock in scripts, but not when using Lock spells.
                    if (ptr.getTypeName() == typeid(ESM::Door).name() && !ptr.getCellRef().getTeleport())
                    {
                        MWBase::Environment::get().getWorld()->activateDoor(ptr, MWWorld::DoorState::Idle);
                    }
                }
        };

        template<class R>
        class OpUnlock : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    ptr.getCellRef().unlock ();
                }
        };

        class OpToggleCollisionDebug : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_CollisionDebug);

                    runtime.getContext().report (enabled ?
                        "Collision Mesh Rendering -> On" : "Collision Mesh Rendering -> Off");
                }
        };


        class OpToggleCollisionBoxes : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_CollisionDebug);

                    runtime.getContext().report (enabled ?
                        "Collision Mesh Rendering -> On" : "Collision Mesh Rendering -> Off");
                }
        };

        class OpToggleWireframe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_Wireframe);

                    runtime.getContext().report (enabled ?
                        "Wireframe Rendering -> On" : "Wireframe Rendering -> Off");
                }
        };

        class OpToggleBorders : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleBorders();

                    runtime.getContext().report (enabled ?
                        "Border Rendering -> On" : "Border Rendering -> Off");
                }
        };

        class OpTogglePathgrid : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                bool enabled =
                    MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_Pathgrid);

                runtime.getContext().report (enabled ?
                    "Path Grid rendering -> On" : "Path Grid Rendering -> Off");
            }
        };

        class OpFadeIn : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWindowManager()->fadeScreenIn(time, false);
                }
        };

        class OpFadeOut : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWindowManager()->fadeScreenOut(time, false);
                }
        };

        class OpFadeTo : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float alpha = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWindowManager()->fadeScreenTo(static_cast<int>(alpha), time, false);
                }
        };

        class OpToggleWater : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.getContext().report(MWBase::Environment::get().getWorld()->toggleWater() ? "Water -> On"
                                                                                                     : "Water -> Off");
                }
        };

        class OpToggleWorld : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.getContext().report(MWBase::Environment::get().getWorld()->toggleWorld() ? "World -> On"
                                                                                                     : "World -> Off");
                }
        };

        class OpDontSaveObject : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    // We are ignoring the DontSaveObject statement for now. Probably not worth
                    // bothering with. The incompatibility we are creating should be marginal at most.
                }
        };

        class OpPcForce1stPerson : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                if (!MWBase::Environment::get().getWorld()->isFirstPerson())
                    MWBase::Environment::get().getWorld()->togglePOV(true);
            }
        };

        class OpPcForce3rdPerson : public Interpreter::Opcode0
        {
            virtual void execute (Interpreter::Runtime& runtime)
            {
                if (MWBase::Environment::get().getWorld()->isFirstPerson())
                    MWBase::Environment::get().getWorld()->togglePOV(true);
            }
        };

        class OpPcGet3rdPerson : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime& runtime)
            {
                runtime.push(!MWBase::Environment::get().getWorld()->isFirstPerson());
            }
        };

        class OpToggleVanityMode : public Interpreter::Opcode0
        {
            static bool sActivate;

        public:

            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWBase::World *world =
                    MWBase::Environment::get().getWorld();

                if (world->toggleVanityMode(sActivate)) {
                    runtime.getContext().report(sActivate ? "Vanity Mode -> On" : "Vanity Mode -> Off");
                    sActivate = !sActivate;
                } else {
                    runtime.getContext().report("Vanity Mode -> No");
                }
            }
        };
        bool OpToggleVanityMode::sActivate = true;

        template <class R>
        class OpGetLocked : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (ptr.getCellRef().getLockLevel() > 0);
                }
        };

        template <class R>
        class OpGetEffect : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string effect = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();

                    if (!ptr.getClass().isActor())
                    {
                        runtime.push(0);
                        return;
                    }

                    char *end;
                    long key = strtol(effect.c_str(), &end, 10);
                    if(key < 0 || key > 32767 || *end != '\0')
                        key = ESM::MagicEffect::effectStringToId(effect);

                    const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

                    MWMechanics::MagicEffects effects = stats.getSpells().getMagicEffects();
                    effects += stats.getActiveSpells().getMagicEffects();
                    if (ptr.getClass().hasInventoryStore(ptr) && !stats.isDeathAnimationFinished())
                    {
                        MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
                        effects += store.getMagicEffects();
                    }

                    for (MWMechanics::MagicEffects::Collection::const_iterator it = effects.begin(); it != effects.end(); ++it)
                    {
                        if (it->first.mId == key && it->second.getModifier() > 0)
                        {
                            runtime.push(1);
                            return;
                        }
                    }
                    runtime.push(0);
               }
        };

        template<class R>
        class OpAddSoulGem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string creature = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string gem = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if (!ptr.getClass().hasInventoryStore(ptr))
                        return;

                    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                    store.get<ESM::Creature>().find(creature); // This line throws an exception if it can't find the creature

                    MWWorld::Ptr item = *ptr.getClass().getContainerStore(ptr).add(gem, 1, ptr);

                    // Set the soul on just one of the gems, not the whole stack
                    item.getContainerStore()->unstack(item, ptr);
                    item.getCellRef().setSoul(creature);

                    // Restack the gem with other gems with the same soul
                    item.getContainerStore()->restack(item);
                }
        };

        template<class R>
        class OpRemoveSoulGem : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string soul = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    // throw away additional arguments
                    for (unsigned int i=0; i<arg0; ++i)
                        runtime.pop();

                    if (!ptr.getClass().hasInventoryStore(ptr))
                        return;

                    MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
                    for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
                    {
                        if (::Misc::StringUtils::ciEqual(it->getCellRef().getSoul(), soul))
                        {
                            store.remove(*it, 1, ptr);
                            return;
                        }
                    }
                }
        };

        template<class R>
        class OpDrop : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer amount = runtime[0].mInteger;
                    runtime.pop();

                    if (amount<0)
                        throw std::runtime_error ("amount must be non-negative");

                    // no-op
                    if (amount == 0)
                        return;

                    if (!ptr.getClass().isActor())
                        return;

                    if (ptr.getClass().hasInventoryStore(ptr))
                    {
                        // Prefer dropping unequipped items first; re-stack if possible by unequipping items before dropping them.
                        MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);
                        int numNotEquipped = store.count(item);
                        for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                        {
                            MWWorld::ConstContainerStoreIterator it = store.getSlot (slot);
                            if (it != store.end() && ::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), item))
                            {
                                numNotEquipped -= it->getRefData().getCount();
                            }
                        }

                        for (int slot = 0; slot < MWWorld::InventoryStore::Slots && amount > numNotEquipped; ++slot)
                        {
                            MWWorld::ContainerStoreIterator it = store.getSlot (slot);
                            if (it != store.end() && ::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), item))
                            {
                                int numToRemove = std::min(amount - numNotEquipped, it->getRefData().getCount());
                                store.unequipItemQuantity(*it, ptr, numToRemove);
                                numNotEquipped += numToRemove;
                            }
                        }

                        for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                        {
                            if (::Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), item) && !store.isEquipped(*iter))
                            {
                                int removed = store.remove(*iter, amount, ptr);
                                MWWorld::Ptr dropped = MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter, removed);
                                dropped.getCellRef().setOwner("");

                                amount -= removed;

                                if (amount <= 0)
                                    break;
                            }
                        }
                    }

                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), item, 1);
                    MWWorld::Ptr itemPtr(ref.getPtr());
                    if (amount > 0)
                    {
                        if (itemPtr.getClass().getScript(itemPtr).empty())
                        {
                            MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, itemPtr, amount);
                        }
                        else
                        {
                            // Dropping one item per time to prevent making stacks of scripted items
                            for (int i = 0; i < amount; i++)
                                MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, itemPtr, 1);
                        }
                    }

                    MWBase::Environment::get().getSoundManager()->playSound3D(ptr, itemPtr.getClass().getDownSoundId(itemPtr), 1.f, 1.f);
                }
        };

        template<class R>
        class OpDropSoulGem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    std::string soul = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if (!ptr.getClass().hasInventoryStore(ptr))
                        return;

                    MWWorld::InventoryStore& store = ptr.getClass().getInventoryStore(ptr);

                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    {
                        if (::Misc::StringUtils::ciEqual(iter->getCellRef().getSoul(), soul))
                        {
                            MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter, 1);
                            store.remove(*iter, 1, ptr);
                            break;
                        }
                    }
                }
        };

        template <class R>
        class OpGetAttacked : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push(ptr.getClass().getCreatureStats (ptr).getAttacked ());
                }
        };

        template <class R>
        class OpGetWeaponDrawn : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push((ptr.getClass().hasInventoryStore(ptr) || ptr.getClass().isBipedal(ptr)) &&
                                ptr.getClass().getCreatureStats (ptr).getDrawState () == MWMechanics::DrawState_Weapon);
                }
        };

        template <class R>
        class OpGetSpellReadied : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push(ptr.getClass().getCreatureStats (ptr).getDrawState () == MWMechanics::DrawState_Spell);
                }
        };

        template <class R>
        class OpGetSpellEffects : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    std::string id = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();

                    if (!ptr.getClass().isActor())
                    {
                        runtime.push(0);
                        return;
                    }

                    const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                    runtime.push(stats.getActiveSpells().isSpellActive(id) || stats.getSpells().isSpellActive(id));
                }
        };

        class OpGetCurrentTime : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                runtime.push(MWBase::Environment::get().getWorld()->getTimeStamp().getHour());
            }
        };

        template <class R>
        class OpSetDelete : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    int parameter = runtime[0].mInteger;
                    runtime.pop();

                    if (parameter == 1)
                        MWBase::Environment::get().getWorld()->deleteObject(ptr);
                    else if (parameter == 0)
                        MWBase::Environment::get().getWorld()->undeleteObject(ptr);
                    else
                        throw std::runtime_error("SetDelete: unexpected parameter");
                }
        };

        class OpGetSquareRoot : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    float param = runtime[0].mFloat;
                    runtime.pop();

                    runtime.push(std::sqrt (param));
                }
        };

        template <class R>
        class OpFall : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                }
        };

        template <class R>
        class OpGetStandingPc : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    runtime.push (MWBase::Environment::get().getWorld()->getPlayerStandingOn(ptr));
                }
        };

        template <class R>
        class OpGetStandingActor : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    runtime.push (MWBase::Environment::get().getWorld()->getActorStandingOn(ptr));
                }
        };

        template <class R>
        class OpGetCollidingPc : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    runtime.push (MWBase::Environment::get().getWorld()->getPlayerCollidingWith(ptr));
                }
        };

        template <class R>
        class OpGetCollidingActor : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    runtime.push (MWBase::Environment::get().getWorld()->getActorCollidingWith(ptr));
                }
        };

        template <class R>
        class OpHurtStandingActor : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    float healthDiffPerSecond = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->hurtStandingActors(ptr, healthDiffPerSecond);
                }
        };

        template <class R>
        class OpHurtCollidingActor : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    float healthDiffPerSecond = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->hurtCollidingActors(ptr, healthDiffPerSecond);
                }
        };

        class OpGetWindSpeed : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push(MWBase::Environment::get().getWorld()->getWindSpeed());
                }
        };

        template <class R>
        class OpHitOnMe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string objectID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);
                    runtime.push(::Misc::StringUtils::ciEqual(objectID, stats.getLastHitObject()));

                    stats.setLastHitObject(std::string());
                }
        };

        template <class R>
        class OpHitAttemptOnMe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string objectID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWMechanics::CreatureStats &stats = ptr.getClass().getCreatureStats(ptr);
                    runtime.push(::Misc::StringUtils::ciEqual(objectID, stats.getLastHitAttemptObject()));

                    stats.setLastHitAttemptObject(std::string());
                }
        };

        template <bool Enable>
        class OpEnableTeleporting : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    world->enableTeleporting(Enable);
                }
        };

        template <bool Enable>
        class OpEnableLevitation : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    world->enableLevitation(Enable);
                }
        };

        template <class R>
        class OpShow : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr ptr = R()(runtime, false);
                std::string var = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                std::stringstream output;

                if (!ptr.isEmpty())
                {
                    const std::string& script = ptr.getClass().getScript(ptr);
                    if (!script.empty())
                    {
                        const Compiler::Locals& locals =
                            MWBase::Environment::get().getScriptManager()->getLocals(script);
                        char type = locals.getType(var);
                        std::string refId = ptr.getCellRef().getRefId();
                        if (refId.find(' ') != std::string::npos)
                            refId = '"' + refId + '"';
                        switch (type)
                        {
                        case 'l':
                        case 's':
                            output << refId << "." << var << " = " << ptr.getRefData().getLocals().getIntVar(script, var);
                            break;
                        case 'f':
                            output << refId << "." << var << " = " << ptr.getRefData().getLocals().getFloatVar(script, var);
                            break;
                        }
                    }
                }
                if (output.rdbuf()->in_avail() == 0)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    char type = world->getGlobalVariableType (var);

                    switch (type)
                    {
                    case 's':
                        output << var << " = " << runtime.getContext().getGlobalShort (var);
                        break;
                    case 'l':
                        output << var << " = " << runtime.getContext().getGlobalLong (var);
                        break;
                    case 'f':
                        output << var << " = " << runtime.getContext().getGlobalFloat (var);
                        break;
                    default:
                        output << "unknown variable";
                    }
                }
                runtime.getContext().report(output.str());
            }
        };

        template <class R>
        class OpShowVars : public Interpreter::Opcode0
        {
            void printLocalVars(Interpreter::Runtime &runtime, const MWWorld::Ptr &ptr)
            {
                std::stringstream str;

                const std::string script = ptr.getClass().getScript(ptr);
                if(script.empty())
                    str<< ptr.getCellRef().getRefId()<<" does not have a script.";
                else
                {
                    str<< "Local variables for "<<ptr.getCellRef().getRefId();

                    const Locals &locals = ptr.getRefData().getLocals();
                    const Compiler::Locals &complocals = MWBase::Environment::get().getScriptManager()->getLocals(script);

                    const std::vector<std::string> *names = &complocals.get('s');
                    for(size_t i = 0;i < names->size();++i)
                    {
                        if(i >= locals.mShorts.size())
                            break;
                        str<<std::endl<< "  "<<(*names)[i]<<" = "<<locals.mShorts[i]<<" (short)";
                    }
                    names = &complocals.get('l');
                    for(size_t i = 0;i < names->size();++i)
                    {
                        if(i >= locals.mLongs.size())
                            break;
                        str<<std::endl<< "  "<<(*names)[i]<<" = "<<locals.mLongs[i]<<" (long)";
                    }
                    names = &complocals.get('f');
                    for(size_t i = 0;i < names->size();++i)
                    {
                        if(i >= locals.mFloats.size())
                            break;
                        str<<std::endl<< "  "<<(*names)[i]<<" = "<<locals.mFloats[i]<<" (float)";
                    }
                }

                runtime.getContext().report(str.str());
            }

            void printGlobalVars(Interpreter::Runtime &runtime)
            {
                std::stringstream str;
                str<< "Global variables:";

                MWBase::World *world = MWBase::Environment::get().getWorld();
                std::vector<std::string> names = runtime.getContext().getGlobals();
                for(size_t i = 0;i < names.size();++i)
                {
                    char type = world->getGlobalVariableType (names[i]);
                    str << std::endl << " " << names[i] << " = ";

                    switch (type)
                    {
                        case 's':

                            str << runtime.getContext().getGlobalShort (names[i]) << " (short)";
                            break;

                        case 'l':

                            str << runtime.getContext().getGlobalLong (names[i]) << " (long)";
                            break;

                        case 'f':

                            str << runtime.getContext().getGlobalFloat (names[i]) << " (float)";
                            break;

                        default:

                            str << "<unknown type>";
                    }
                }

                runtime.getContext().report (str.str());
            }

        public:
            virtual void execute(Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr ptr = R()(runtime, false);
                if (!ptr.isEmpty())
                    printLocalVars(runtime, ptr);
                else
                {
                    // No reference, no problem.
                    printGlobalVars(runtime);
                }
            }
        };

        class OpToggleScripts : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleScripts();

                runtime.getContext().report(enabled ? "Scripts -> On" : "Scripts -> Off");
            }
        };

        class OpToggleGodMode : public Interpreter::Opcode0
        {
            public:
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled = MWBase::Environment::get().getWorld()->toggleGodMode();

                    runtime.getContext().report (enabled ? "God Mode -> On" : "God Mode -> Off");
                }
        };

        template <class R>
        class OpCast : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr ptr = R()(runtime);

                std::string spellId = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                std::string targetId = ::Misc::StringUtils::lowerCase(runtime.getStringLiteral (runtime[0].mInteger));
                runtime.pop();

                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId);
                if (!spell)
                {
                    runtime.getContext().report("spellcasting failed: cannot find spell \""+spellId+"\"");
                    return;
                }

                if (ptr == MWMechanics::getPlayer())
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setSelectedSpell(spellId);
                    return;
                }

                if (ptr.getClass().isActor())
                {
                    MWMechanics::AiCast castPackage(targetId, spellId, true);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(castPackage, ptr);
                    return;
                }

                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(targetId, false, false);
                if (target.isEmpty())
                    return;

                MWMechanics::CastSpell cast(ptr, target, false, true);
                cast.playSpellCastingEffects(spell->mId, false);
                cast.mHitPosition = target.getRefData().getPosition().asVec3();
                cast.mAlwaysSucceed = true;
                cast.cast(spell);
            }
        };

        template <class R>
        class OpExplodeSpell : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr ptr = R()(runtime);

                std::string spellId = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId);
                if (!spell)
                {
                    runtime.getContext().report("spellcasting failed: cannot find spell \""+spellId+"\"");
                    return;
                }

                if (ptr == MWMechanics::getPlayer())
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setSelectedSpell(spellId);
                    return;
                }

                if (ptr.getClass().isActor())
                {
                    MWMechanics::AiCast castPackage(ptr.getCellRef().getRefId(), spellId, true);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(castPackage, ptr);
                    return;
                }

                MWMechanics::CastSpell cast(ptr, ptr, false, true);
                cast.mHitPosition = ptr.getRefData().getPosition().asVec3();
                cast.mAlwaysSucceed = true;
                cast.cast(spell);
            }
        };

        class OpGoToJail : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                world->goToJail();
            }
        };

        class OpPayFine : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWWorld::Ptr player = MWMechanics::getPlayer();
                player.getClass().getNpcStats(player).setBounty(0);
                MWBase::Environment::get().getWorld()->confiscateStolenItems(player);
                MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
            }
        };

        class OpPayFineThief : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWWorld::Ptr player = MWMechanics::getPlayer();
                player.getClass().getNpcStats(player).setBounty(0);
                MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
            }
        };

        class OpGetPcInJail : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime &runtime)
                {
                    runtime.push (MWBase::Environment::get().getWorld()->isPlayerInJail());
                }
        };

        class OpGetPcTraveling : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime &runtime)
                {
                    runtime.push (MWBase::Environment::get().getWorld()->isPlayerTraveling());
                }
        };

        template <class R>
        class OpBetaComment : public Interpreter::Opcode1
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime, unsigned int arg0)
            {
                MWWorld::Ptr ptr = R()(runtime);

                std::stringstream msg;

                msg << "Report time: ";

                std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                msg << std::put_time(std::gmtime(&currentTime), "%Y.%m.%d %T UTC") << std::endl;

                msg << "Content file: ";

                if (!ptr.getCellRef().hasContentFile())
                    msg << "[None]" << std::endl;
                else
                {
                    std::vector<std::string> contentFiles = MWBase::Environment::get().getWorld()->getContentFiles();

                    msg << contentFiles.at (ptr.getCellRef().getRefNum().mContentFile) << std::endl;
                    msg << "RefNum: " << ptr.getCellRef().getRefNum().mIndex << std::endl;
                }

                if (ptr.getRefData().isDeletedByContentFile())
                    msg << "[Deleted by content file]" << std::endl;
                if (!ptr.getRefData().getCount())
                    msg << "[Deleted]" << std::endl;

                msg << "RefID: " << ptr.getCellRef().getRefId() << std::endl;
                msg << "Memory address: " << ptr.getBase() << std::endl;

                if (ptr.isInCell())
                {
                    MWWorld::CellStore* cell = ptr.getCell();
                    msg << "Cell: " << MWBase::Environment::get().getWorld()->getCellName(cell) << std::endl;
                    if (cell->getCell()->isExterior())
                        msg << "Grid: " << cell->getCell()->getGridX() << " " << cell->getCell()->getGridY() << std::endl;
                    osg::Vec3f pos (ptr.getRefData().getPosition().asVec3());
                    msg << "Coordinates: " << pos.x() << " " << pos.y() << " " << pos.z() << std::endl;
                    msg << "Model: " << ptr.getClass().getModel(ptr) << std::endl;
                    if (!ptr.getClass().getScript(ptr).empty())
                        msg << "Script: " << ptr.getClass().getScript(ptr) << std::endl;
                }

                while (arg0 > 0)
                {
                    std::string notes = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    if (!notes.empty())
                        msg << "Notes: " << notes << std::endl;
                    --arg0;
                }

                Log(Debug::Warning) << "\n" << msg.str();

                runtime.getContext().report(msg.str());
            }
        };

        class OpAddToLevCreature : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                const std::string& levId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                const std::string& creatureId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::CreatureLevList listCopy = *MWBase::Environment::get().getWorld()->getStore().get<ESM::CreatureLevList>().find(levId);
                addToLevList(&listCopy, creatureId, level);
                MWBase::Environment::get().getWorld()->createOverrideRecord(listCopy);
            }
        };

        class OpRemoveFromLevCreature : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                const std::string& levId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                const std::string& creatureId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::CreatureLevList listCopy = *MWBase::Environment::get().getWorld()->getStore().get<ESM::CreatureLevList>().find(levId);
                removeFromLevList(&listCopy, creatureId, level);
                MWBase::Environment::get().getWorld()->createOverrideRecord(listCopy);
            }
        };

        class OpAddToLevItem : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                const std::string& levId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                const std::string& itemId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::ItemLevList listCopy = *MWBase::Environment::get().getWorld()->getStore().get<ESM::ItemLevList>().find(levId);
                addToLevList(&listCopy, itemId, level);
                MWBase::Environment::get().getWorld()->createOverrideRecord(listCopy);
            }
        };

        class OpRemoveFromLevItem : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                const std::string& levId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                const std::string& itemId = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::ItemLevList listCopy = *MWBase::Environment::get().getWorld()->getStore().get<ESM::ItemLevList>().find(levId);
                removeFromLevList(&listCopy, itemId, level);
                MWBase::Environment::get().getWorld()->createOverrideRecord(listCopy);
            }
        };

        template <class R>
        class OpShowSceneGraph : public Interpreter::Opcode1
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime, unsigned int arg0)
            {
                MWWorld::Ptr ptr = R()(runtime, false);

                int confirmed = 0;
                if (arg0==1)
                {
                    confirmed = runtime[0].mInteger;
                    runtime.pop();
                }

                if (ptr.isEmpty() && !confirmed)
                    runtime.getContext().report("Exporting the entire scene graph will result in a large file. Confirm this action using 'showscenegraph 1' or select an object instead.");
                else
                {
                    const std::string& filename = MWBase::Environment::get().getWorld()->exportSceneGraph(ptr);
                    runtime.getContext().report("Wrote '" + filename + "'");
                }
            }
        };

        class OpToggleNavMesh : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_NavMesh);

                    runtime.getContext().report (enabled ?
                        "Navigation Mesh Rendering -> On" : "Navigation Mesh Rendering -> Off");
                }
        };

        class OpToggleActorsPaths : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_ActorsPaths);

                    runtime.getContext().report (enabled ?
                        "Agents Paths Rendering -> On" : "Agents Paths Rendering -> Off");
                }
        };

        class OpSetNavMeshNumberToRender : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    const auto navMeshNumber = runtime[0].mInteger;
                    runtime.pop();

                    if (navMeshNumber < 0)
                    {
                        runtime.getContext().report("Invalid navmesh number: use not less than zero values");
                        return;
                    }

                    MWBase::Environment::get().getWorld()->setNavMeshNumberToRender(static_cast<std::size_t>(navMeshNumber));
                }
        };

        template <class R>
        class OpRepairedOnMe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    // Broken in vanilla and deliberately no-op.
                    runtime.push(0);
                }
        };

        class OpToggleRecastMesh : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWRender::Render_RecastMesh);

                    runtime.getContext().report (enabled ?
                        "Recast Mesh Rendering -> On" : "Recast Mesh Rendering -> Off");
                }
        };

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Misc::opcodeXBox, new OpXBox);
            interpreter.installSegment5 (Compiler::Misc::opcodeOnActivate, new OpOnActivate<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeOnActivateExplicit, new OpOnActivate<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeActivate, new OpActivate<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeActivateExplicit, new OpActivate<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Misc::opcodeLock, new OpLock<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Misc::opcodeLockExplicit, new OpLock<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeUnlock, new OpUnlock<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeUnlockExplicit, new OpUnlock<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleCollisionDebug, new OpToggleCollisionDebug);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleCollisionBoxes, new OpToggleCollisionBoxes);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleWireframe, new OpToggleWireframe);
            interpreter.installSegment5 (Compiler::Misc::opcodeFadeIn, new OpFadeIn);
            interpreter.installSegment5 (Compiler::Misc::opcodeFadeOut, new OpFadeOut);
            interpreter.installSegment5 (Compiler::Misc::opcodeFadeTo, new OpFadeTo);
            interpreter.installSegment5 (Compiler::Misc::opcodeTogglePathgrid, new OpTogglePathgrid);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleWater, new OpToggleWater);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleWorld, new OpToggleWorld);
            interpreter.installSegment5 (Compiler::Misc::opcodeDontSaveObject, new OpDontSaveObject);
            interpreter.installSegment5 (Compiler::Misc::opcodePcForce1stPerson, new OpPcForce1stPerson);
            interpreter.installSegment5 (Compiler::Misc::opcodePcForce3rdPerson, new OpPcForce3rdPerson);
            interpreter.installSegment5 (Compiler::Misc::opcodePcGet3rdPerson, new OpPcGet3rdPerson);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleVanityMode, new OpToggleVanityMode);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetPcSleep, new OpGetPcSleep);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetPcJumping, new OpGetPcJumping);
            interpreter.installSegment5 (Compiler::Misc::opcodeWakeUpPc, new OpWakeUpPc);
            interpreter.installSegment5 (Compiler::Misc::opcodePlayBink, new OpPlayBink);
            interpreter.installSegment5 (Compiler::Misc::opcodePayFine, new OpPayFine);
            interpreter.installSegment5 (Compiler::Misc::opcodePayFineThief, new OpPayFineThief);
            interpreter.installSegment5 (Compiler::Misc::opcodeGoToJail, new OpGoToJail);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetLocked, new OpGetLocked<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetLockedExplicit, new OpGetLocked<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetEffect, new OpGetEffect<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetEffectExplicit, new OpGetEffect<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeAddSoulGem, new OpAddSoulGem<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeAddSoulGemExplicit, new OpAddSoulGem<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Misc::opcodeRemoveSoulGem, new OpRemoveSoulGem<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Misc::opcodeRemoveSoulGemExplicit, new OpRemoveSoulGem<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeDrop, new OpDrop<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeDropExplicit, new OpDrop<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeDropSoulGem, new OpDropSoulGem<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeDropSoulGemExplicit, new OpDropSoulGem<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetAttacked, new OpGetAttacked<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetAttackedExplicit, new OpGetAttacked<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetWeaponDrawn, new OpGetWeaponDrawn<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetWeaponDrawnExplicit, new OpGetWeaponDrawn<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetSpellReadied, new OpGetSpellReadied<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetSpellReadiedExplicit, new OpGetSpellReadied<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetSpellEffects, new OpGetSpellEffects<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetSpellEffectsExplicit, new OpGetSpellEffects<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetCurrentTime, new OpGetCurrentTime);
            interpreter.installSegment5 (Compiler::Misc::opcodeSetDelete, new OpSetDelete<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeSetDeleteExplicit, new OpSetDelete<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetSquareRoot, new OpGetSquareRoot);
            interpreter.installSegment5 (Compiler::Misc::opcodeFall, new OpFall<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeFallExplicit, new OpFall<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetStandingPc, new OpGetStandingPc<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetStandingPcExplicit, new OpGetStandingPc<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetStandingActor, new OpGetStandingActor<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetStandingActorExplicit, new OpGetStandingActor<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetCollidingPc, new OpGetCollidingPc<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetCollidingPcExplicit, new OpGetCollidingPc<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetCollidingActor, new OpGetCollidingActor<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetCollidingActorExplicit, new OpGetCollidingActor<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHurtStandingActor, new OpHurtStandingActor<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHurtStandingActorExplicit, new OpHurtStandingActor<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHurtCollidingActor, new OpHurtCollidingActor<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHurtCollidingActorExplicit, new OpHurtCollidingActor<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetWindSpeed, new OpGetWindSpeed);
            interpreter.installSegment5 (Compiler::Misc::opcodeHitOnMe, new OpHitOnMe<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHitOnMeExplicit, new OpHitOnMe<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHitAttemptOnMe, new OpHitAttemptOnMe<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHitAttemptOnMeExplicit, new OpHitAttemptOnMe<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeDisableTeleporting, new OpEnableTeleporting<false>);
            interpreter.installSegment5 (Compiler::Misc::opcodeEnableTeleporting, new OpEnableTeleporting<true>);
            interpreter.installSegment5 (Compiler::Misc::opcodeShowVars, new OpShowVars<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeShowVarsExplicit, new OpShowVars<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeShow, new OpShow<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeShowExplicit, new OpShow<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleGodMode, new OpToggleGodMode);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleScripts, new OpToggleScripts);
            interpreter.installSegment5 (Compiler::Misc::opcodeDisableLevitation, new OpEnableLevitation<false>);
            interpreter.installSegment5 (Compiler::Misc::opcodeEnableLevitation, new OpEnableLevitation<true>);
            interpreter.installSegment5 (Compiler::Misc::opcodeCast, new OpCast<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeCastExplicit, new OpCast<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeExplodeSpell, new OpExplodeSpell<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeExplodeSpellExplicit, new OpExplodeSpell<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetPcInJail, new OpGetPcInJail);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetPcTraveling, new OpGetPcTraveling);
            interpreter.installSegment3 (Compiler::Misc::opcodeBetaComment, new OpBetaComment<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Misc::opcodeBetaCommentExplicit, new OpBetaComment<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeAddToLevCreature, new OpAddToLevCreature);
            interpreter.installSegment5 (Compiler::Misc::opcodeRemoveFromLevCreature, new OpRemoveFromLevCreature);
            interpreter.installSegment5 (Compiler::Misc::opcodeAddToLevItem, new OpAddToLevItem);
            interpreter.installSegment5 (Compiler::Misc::opcodeRemoveFromLevItem, new OpRemoveFromLevItem);
            interpreter.installSegment3 (Compiler::Misc::opcodeShowSceneGraph, new OpShowSceneGraph<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Misc::opcodeShowSceneGraphExplicit, new OpShowSceneGraph<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleBorders, new OpToggleBorders);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleNavMesh, new OpToggleNavMesh);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleActorsPaths, new OpToggleActorsPaths);
            interpreter.installSegment5 (Compiler::Misc::opcodeSetNavMeshNumberToRender, new OpSetNavMeshNumberToRender);
            interpreter.installSegment5 (Compiler::Misc::opcodeRepairedOnMe, new OpRepairedOnMe<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeRepairedOnMeExplicit, new OpRepairedOnMe<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleRecastMesh, new OpToggleRecastMesh);
        }
    }
}
