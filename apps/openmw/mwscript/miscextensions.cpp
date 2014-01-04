
#include "miscextensions.hpp"

#include <cstdlib>

#include <libs/openengine/ogre/fader.hpp>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>
#include <components/compiler/locals.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

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

                bool allowSkipping = runtime[0].mInteger;
                runtime.pop();

                MWBase::Environment::get().getWorld ()->playVideo (name, allowSkipping);
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

        class OpOnActivate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    runtime.push (context.hasBeenActivated (ptr));
                }
        };

        class OpActivate : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    context.executeActivation();
                }
        };

        template<class R>
        class OpLock : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer lockLevel = 100;

                    if (arg0==1)
                    {
                        lockLevel = runtime[0].mInteger;
                        runtime.pop();
                    }

                    MWWorld::Class::get (ptr).lock (ptr, lockLevel);
                }
        };

        template<class R>
        class OpUnlock : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Class::get (ptr).unlock (ptr);
                }
        };

        class OpToggleCollisionDebug : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_CollisionDebug);

                    context.report (enabled ?
                        "Collision Mesh Rendering -> On" : "Collision Mesh Rendering -> Off");
                }
        };


        class OpToggleCollisionBoxes : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_BoundingBoxes);

                    context.report (enabled ?
                        "Bounding Box Rendering -> On" : "Bounding Box Rendering -> Off");
                }
        };

        class OpToggleWireframe : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled =
                        MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_Wireframe);

                    context.report (enabled ?
                        "Wireframe Rendering -> On" : "Wireframe Rendering -> Off");
                }
        };

        class OpTogglePathgrid : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                InterpreterContext& context =
                    static_cast<InterpreterContext&> (runtime.getContext());

                bool enabled =
                    MWBase::Environment::get().getWorld()->toggleRenderMode (MWBase::World::Render_Pathgrid);

                context.report (enabled ?
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

                    MWBase::Environment::get().getWorld()->getFader()->fadeIn(time);
                }
        };

        class OpFadeOut : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    Interpreter::Type_Float time = runtime[0].mFloat;
                    runtime.pop();

                    MWBase::Environment::get().getWorld()->getFader()->fadeOut(time);
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

                    MWBase::Environment::get().getWorld()->getFader()->fadeTo(alpha, time);
                }
        };

        class OpToggleWater : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWorld()->toggleWater();
                }
        };

        class OpDontSaveObject : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    // We are ignoring the DontSaveObject statement for now. Probably not worth
                    /// bothering with. The incompatibility we are creating should be marginal at most.
                }
        };

        class OpToggleVanityMode : public Interpreter::Opcode0
        {
            static bool sActivate;

        public:

            virtual void execute(Interpreter::Runtime &runtime)
            {
                InterpreterContext& context =
                    static_cast<InterpreterContext&> (runtime.getContext());

                MWBase::World *world =
                    MWBase::Environment::get().getWorld();

                if (world->toggleVanityMode(sActivate)) {
                    context.report(sActivate ? "Vanity Mode -> On" : "Vanity Mode -> Off");
                    sActivate = !sActivate;
                } else {
                    context.report("Vanity Mode -> No");
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

                    runtime.push (ptr.getCellRef ().mLockLevel > 0);
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

                    char *end;
                    long key = strtol(effect.c_str(), &end, 10);
                    if(key < 0 || key > 32767 || *end != '\0')
                        key = ESM::MagicEffect::effectStringToId(effect);

                    runtime.push(MWWorld::Class::get(ptr).getCreatureStats(ptr).getMagicEffects().get(
                                      MWMechanics::EffectKey(key)).mMagnitude > 0);
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

                    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                    store.get<ESM::Creature>().find(creature); // This line throws an exception if it can't find the creature

                    MWWorld::Ptr item = *ptr.getClass().getContainerStore(ptr).add(gem, 1, ptr);
                    item.getCellRef().mSoul = creature;
                }
        };

        template<class R>
        class OpRemoveSoulGem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    std::string soul = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);

                    store.remove(soul, 1, ptr);
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

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);


                    int toRemove = amount;
                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    {
                        if (::Misc::StringUtils::ciEqual(iter->getCellRef().mRefID, item))
                        {
                            int removed = store.remove(*iter, toRemove, ptr);
                            MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter, removed);

                            toRemove -= removed;

                            if (toRemove <= 0)
                                break;
                        }
                    }
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

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);


                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    {
                        if (::Misc::StringUtils::ciEqual(iter->getCellRef().mSoul, soul))
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

                    runtime.push(MWWorld::Class::get(ptr).getCreatureStats (ptr).getAttacked ());
                }
        };

        template <class R>
        class OpGetWeaponDrawn : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push(ptr.getClass().getNpcStats (ptr).getDrawState () == MWMechanics::DrawState_Weapon);
                }
        };

        template <class R>
        class OpGetSpellReadied : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push(ptr.getClass().getNpcStats (ptr).getDrawState () == MWMechanics::DrawState_Spell);
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

                    runtime.push(MWWorld::Class::get(ptr).getCreatureStats(ptr).getActiveSpells().isSpellActive(id));
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

                    MWMechanics::CreatureStats &stats = MWWorld::Class::get(ptr).getCreatureStats(ptr);
                    runtime.push(::Misc::StringUtils::ciEqual(objectID, stats.getLastHitObject()));
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
        class OpShowVars : public Interpreter::Opcode0
        {
            void printLocalVars(Interpreter::Runtime &runtime, const MWWorld::Ptr &ptr)
            {
                std::stringstream str;

                const std::string script = MWWorld::Class::get(ptr).getScript(ptr);
                if(script.empty())
                    str<< ptr.getCellRef().mRefID<<" ("<<ptr.getRefData().getHandle()<<") does not have a script.";
                else
                {
                    str<< "Local variables for "<<ptr.getCellRef().mRefID<<" ("<<ptr.getRefData().getHandle()<<")";

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
                std::vector<std::string> names = world->getGlobals();
                for(size_t i = 0;i < names.size();++i)
                {
                    char type = world->getGlobalVariableType(names[i]);
                    if(type == 's')
                        str<<std::endl<< "  "<<names[i]<<" = "<<world->getGlobalVariable(names[i]).mShort<<" (short)";
                    else if(type == 'l')
                        str<<std::endl<< "  "<<names[i]<<" = "<<world->getGlobalVariable(names[i]).mLong<<" (long)";
                    else if(type == 'f')
                        str<<std::endl<< "  "<<names[i]<<" = "<<world->getGlobalVariable(names[i]).mFloat<<" (float)";
                }

                runtime.getContext().report(str.str());
            }

        public:
            virtual void execute(Interpreter::Runtime& runtime)
            {
                // No way to tell if we have a reference before trying to get it, and it will
                // cause an exception is there isn't one :(
                try {
                    MWWorld::Ptr ptr = R()(runtime);
                    printLocalVars(runtime, ptr);
                }
                catch(std::runtime_error&) {
                    // No reference, no problem.
                    printGlobalVars(runtime);
                }
            }
        };

        class OpToggleGodMode : public Interpreter::Opcode0
        {
            public:
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context = static_cast<InterpreterContext&> (runtime.getContext());

                    bool enabled = MWBase::Environment::get().getWorld()->toggleGodMode();

                    context.report (enabled ? "God Mode -> On" : "God Mode -> Off");
                }
        };

        template <class R>
        class OpCast : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr ptr = R()(runtime);

                std::string spell = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                std::string targetId = ::Misc::StringUtils::lowerCase(runtime.getStringLiteral (runtime[0].mInteger));
                runtime.pop();

                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr (targetId, false);

                MWMechanics::CastSpell cast(ptr, target);
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

                std::string spell = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                MWMechanics::CastSpell cast(ptr, ptr);
                cast.cast(spell);
            }
        };

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Misc::opcodeXBox, new OpXBox);
            interpreter.installSegment5 (Compiler::Misc::opcodeOnActivate, new OpOnActivate);
            interpreter.installSegment5 (Compiler::Misc::opcodeActivate, new OpActivate);
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
            interpreter.installSegment5 (Compiler::Misc::opcodeDontSaveObject, new OpDontSaveObject);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleVanityMode, new OpToggleVanityMode);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetPcSleep, new OpGetPcSleep);
            interpreter.installSegment5 (Compiler::Misc::opcodeWakeUpPc, new OpWakeUpPc);
            interpreter.installSegment5 (Compiler::Misc::opcodePlayBink, new OpPlayBink);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetLocked, new OpGetLocked<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetLockedExplicit, new OpGetLocked<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetEffect, new OpGetEffect<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeGetEffectExplicit, new OpGetEffect<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeAddSoulGem, new OpAddSoulGem<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeAddSoulGemExplicit, new OpAddSoulGem<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeRemoveSoulGem, new OpRemoveSoulGem<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeRemoveSoulGemExplicit, new OpRemoveSoulGem<ExplicitRef>);
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
            interpreter.installSegment5 (Compiler::Misc::opcodeGetWindSpeed, new OpGetWindSpeed);
            interpreter.installSegment5 (Compiler::Misc::opcodeHitOnMe, new OpHitOnMe<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeHitOnMeExplicit, new OpHitOnMe<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeDisableTeleporting, new OpEnableTeleporting<false>);
            interpreter.installSegment5 (Compiler::Misc::opcodeEnableTeleporting, new OpEnableTeleporting<true>);
            interpreter.installSegment5 (Compiler::Misc::opcodeShowVars, new OpShowVars<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeShowVarsExplicit, new OpShowVars<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeToggleGodMode, new OpToggleGodMode);
            interpreter.installSegment5 (Compiler::Misc::opcodeDisableLevitation, new OpEnableLevitation<false>);
            interpreter.installSegment5 (Compiler::Misc::opcodeEnableLevitation, new OpEnableLevitation<true>);
            interpreter.installSegment5 (Compiler::Misc::opcodeCast, new OpCast<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeCastExplicit, new OpCast<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeExplodeSpell, new OpExplodeSpell<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Misc::opcodeExplodeSpellExplicit, new OpExplodeSpell<ExplicitRef>);
        }
    }
}
