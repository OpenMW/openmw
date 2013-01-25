
#include "miscextensions.hpp"

#include <libs/openengine/ogre/fader.hpp>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/creaturestats.hpp"

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

                    MWWorld::Ptr ptr = context.getReference();

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

                if (world->toggleVanityMode(sActivate, true)) {
                    context.report(
                        (sActivate) ? "Vanity Mode -> On" : "Vanity Mode -> Off"
                    );
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

                    int key = runtime[0].mInteger;
                    runtime.pop();

                    runtime.push (MWWorld::Class::get(ptr).getCreatureStats (ptr).getMagicEffects ().get (
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

                    MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), gem);

                    ref.getPtr().getRefData().setCount (1);

                    ref.getPtr().getCellRef().mSoul = creature;

                    MWWorld::Class::get (ptr).getContainerStore (ptr).add (ref.getPtr());

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


                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    {
                        if (::Misc::StringUtils::ciEqual(iter->getCellRef().mSoul, soul))
                        {
                            if (iter->getRefData().getCount() <= 1)
                                iter->getRefData().setCount (0);
                            else
                                iter->getRefData().setCount (iter->getRefData().getCount() - 1);
                            break;
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

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);


                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    {
                        if (::Misc::StringUtils::ciEqual(iter->getCellRef().mRefID, item))
                        {
                            if(iter->getRefData().getCount() <= amount)
                            {
                                MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter);
                                iter->getRefData().setCount(0);
                            }
                            else
                            {
                                int original = iter->getRefData().getCount();
                                iter->getRefData().setCount(amount);
                                MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter);
                                iter->getRefData().setCount(original - amount);
                            }

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

                            if(iter->getRefData().getCount() <= 1)
                            {
                                MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter);
                                iter->getRefData().setCount(0);
                            }
                            else
                            {
                                int original = iter->getRefData().getCount();
                                iter->getRefData().setCount(1);
                                MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter);
                                iter->getRefData().setCount(original - 1);
                            }

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

                    runtime.push(MWWorld::Class::get(ptr).getNpcStats (ptr).getDrawState () == MWMechanics::DrawState_Weapon);
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
                    {
                        if (ptr.isInCell())
                            MWBase::Environment::get().getWorld()->deleteObject (ptr);
                        else
                            ptr.getRefData().setCount(0);
                    }
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

        const int opcodeXBox = 0x200000c;
        const int opcodeOnActivate = 0x200000d;
        const int opcodeActivate = 0x2000075;
        const int opcodeLock = 0x20004;
        const int opcodeLockExplicit = 0x20005;
        const int opcodeUnlock = 0x200008c;
        const int opcodeUnlockExplicit = 0x200008d;
        const int opcodeToggleCollisionDebug = 0x2000132;
        const int opcodeToggleCollisionBoxes = 0x20001ac;
        const int opcodeToggleWireframe = 0x200013b;
        const int opcodeFadeIn = 0x200013c;
        const int opcodeFadeOut = 0x200013d;
        const int opcodeFadeTo = 0x200013e;
        const int opcodeToggleWater = 0x2000144;
        const int opcodeTogglePathgrid = 0x2000146;
        const int opcodeDontSaveObject = 0x2000153;
        const int opcodeToggleVanityMode = 0x2000174;
        const int opcodeGetPcSleep = 0x200019f;
        const int opcodeWakeUpPc = 0x20001a2;
        const int opcodeGetLocked = 0x20001c7;
        const int opcodeGetLockedExplicit = 0x20001c8;
        const int opcodeGetEffect = 0x20001cf;
        const int opcodeGetEffectExplicit = 0x20001d0;
        const int opcodeAddSoulGem = 0x20001f3;
        const int opcodeAddSoulGemExplicit = 0x20001f4;
        const int opcodeRemoveSoulGem = 0x20001f5;
        const int opcodeRemoveSoulGemExplicit = 0x20001f6;
        const int opcodeDrop = 0x20001f8;
        const int opcodeDropExplicit = 0x20001f9;
        const int opcodeDropSoulGem = 0x20001fa;
        const int opcodeDropSoulGemExplicit = 0x20001fb;
        const int opcodeGetAttacked = 0x20001d3;
        const int opcodeGetAttackedExplicit = 0x20001d4;
        const int opcodeGetWeaponDrawn = 0x20001d7;
        const int opcodeGetWeaponDrawnExplicit = 0x20001d8;
        const int opcodeGetSpellEffects = 0x20001db;
        const int opcodeGetSpellEffectsExplicit = 0x20001dc;
        const int opcodeGetCurrentTime = 0x20001dd;
        const int opcodeSetDelete = 0x20001e5;
        const int opcodeSetDeleteExplicit = 0x20001e6;
        const int opcodeGetSquareRoot = 0x20001e7;

        const int opcodePlayBink = 0x20001f7;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("xbox", 'l', "", opcodeXBox);
            extensions.registerFunction ("onactivate", 'l', "", opcodeOnActivate);
            extensions.registerInstruction ("activate", "", opcodeActivate);
            extensions.registerInstruction ("lock", "/l", opcodeLock, opcodeLockExplicit);
            extensions.registerInstruction ("unlock", "", opcodeUnlock, opcodeUnlockExplicit);
            extensions.registerInstruction ("togglecollisionboxes", "", opcodeToggleCollisionBoxes);
            extensions.registerInstruction ("togglecollisiongrid", "", opcodeToggleCollisionDebug);
            extensions.registerInstruction ("tcb", "", opcodeToggleCollisionBoxes);
            extensions.registerInstruction ("tcg", "", opcodeToggleCollisionDebug);
            extensions.registerInstruction ("twf", "", opcodeToggleWireframe);
            extensions.registerInstruction ("togglewireframe", "", opcodeToggleWireframe);
            extensions.registerInstruction ("fadein", "f", opcodeFadeIn);
            extensions.registerInstruction ("fadeout", "f", opcodeFadeOut);
            extensions.registerInstruction ("fadeto", "ff", opcodeFadeTo);
            extensions.registerInstruction ("togglewater", "", opcodeToggleWater);
            extensions.registerInstruction ("twa", "", opcodeToggleWater);
            extensions.registerInstruction ("togglepathgrid", "", opcodeTogglePathgrid);
            extensions.registerInstruction ("tpg", "", opcodeTogglePathgrid);
            extensions.registerInstruction ("dontsaveobject", "", opcodeDontSaveObject);
            extensions.registerInstruction ("togglevanitymode", "", opcodeToggleVanityMode);
            extensions.registerInstruction ("tvm", "", opcodeToggleVanityMode);
            extensions.registerFunction ("getpcsleep", 'l', "", opcodeGetPcSleep);
            extensions.registerInstruction ("wakeuppc", "", opcodeWakeUpPc);
            extensions.registerInstruction ("playbink", "Sl", opcodePlayBink);
            extensions.registerFunction ("getlocked", 'l', "", opcodeGetLocked, opcodeGetLockedExplicit);
            extensions.registerFunction ("geteffect", 'l', "l", opcodeGetEffect, opcodeGetEffectExplicit);
            extensions.registerInstruction ("addsoulgem", "cc", opcodeAddSoulGem, opcodeAddSoulGemExplicit);
            extensions.registerInstruction ("removesoulgem", "c", opcodeRemoveSoulGem, opcodeRemoveSoulGemExplicit);
            extensions.registerInstruction ("drop", "cl", opcodeDrop, opcodeDropExplicit);
            extensions.registerInstruction ("dropsoulgem", "c", opcodeDropSoulGem, opcodeDropSoulGemExplicit);
            extensions.registerFunction ("getattacked", 'l', "", opcodeGetAttacked, opcodeGetAttackedExplicit);
            extensions.registerFunction ("getweapondrawn", 'l', "", opcodeGetWeaponDrawn, opcodeGetWeaponDrawnExplicit);
            extensions.registerFunction ("getspelleffects", 'l', "c", opcodeGetSpellEffects, opcodeGetSpellEffectsExplicit);
            extensions.registerFunction ("getcurrenttime", 'f', "", opcodeGetCurrentTime);
            extensions.registerInstruction ("setdelete", "l", opcodeSetDelete, opcodeSetDeleteExplicit);
            extensions.registerFunction ("getsquareroot", 'f', "f", opcodeGetSquareRoot);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeXBox, new OpXBox);
            interpreter.installSegment5 (opcodeOnActivate, new OpOnActivate);
            interpreter.installSegment5 (opcodeActivate, new OpActivate);
            interpreter.installSegment3 (opcodeLock, new OpLock<ImplicitRef>);
            interpreter.installSegment3 (opcodeLockExplicit, new OpLock<ExplicitRef>);
            interpreter.installSegment5 (opcodeUnlock, new OpUnlock<ImplicitRef>);
            interpreter.installSegment5 (opcodeUnlockExplicit, new OpUnlock<ExplicitRef>);
            interpreter.installSegment5 (opcodeToggleCollisionDebug, new OpToggleCollisionDebug);
            interpreter.installSegment5 (opcodeToggleCollisionBoxes, new OpToggleCollisionBoxes);
            interpreter.installSegment5 (opcodeToggleWireframe, new OpToggleWireframe);
            interpreter.installSegment5 (opcodeFadeIn, new OpFadeIn);
            interpreter.installSegment5 (opcodeFadeOut, new OpFadeOut);
            interpreter.installSegment5 (opcodeFadeTo, new OpFadeTo);
            interpreter.installSegment5 (opcodeTogglePathgrid, new OpTogglePathgrid);
            interpreter.installSegment5 (opcodeToggleWater, new OpToggleWater);
            interpreter.installSegment5 (opcodeDontSaveObject, new OpDontSaveObject);
            interpreter.installSegment5 (opcodeToggleVanityMode, new OpToggleVanityMode);
            interpreter.installSegment5 (opcodeGetPcSleep, new OpGetPcSleep);
            interpreter.installSegment5 (opcodeWakeUpPc, new OpWakeUpPc);
            interpreter.installSegment5 (opcodePlayBink, new OpPlayBink);
            interpreter.installSegment5 (opcodeGetLocked, new OpGetLocked<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetLockedExplicit, new OpGetLocked<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetEffect, new OpGetEffect<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetEffectExplicit, new OpGetEffect<ExplicitRef>);
            interpreter.installSegment5 (opcodeAddSoulGem, new OpAddSoulGem<ImplicitRef>);
            interpreter.installSegment5 (opcodeAddSoulGemExplicit, new OpAddSoulGem<ExplicitRef>);
            interpreter.installSegment5 (opcodeRemoveSoulGem, new OpRemoveSoulGem<ImplicitRef>);
            interpreter.installSegment5 (opcodeRemoveSoulGemExplicit, new OpRemoveSoulGem<ExplicitRef>);
            interpreter.installSegment5 (opcodeDrop, new OpDrop<ImplicitRef>);
            interpreter.installSegment5 (opcodeDropExplicit, new OpDrop<ExplicitRef>);
            interpreter.installSegment5 (opcodeDropSoulGem, new OpDropSoulGem<ImplicitRef>);
            interpreter.installSegment5 (opcodeDropSoulGemExplicit, new OpDropSoulGem<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetAttacked, new OpGetAttacked<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetAttackedExplicit, new OpGetAttacked<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetWeaponDrawn, new OpGetWeaponDrawn<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetWeaponDrawnExplicit, new OpGetWeaponDrawn<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetSpellEffects, new OpGetSpellEffects<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetSpellEffectsExplicit, new OpGetSpellEffects<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetCurrentTime, new OpGetCurrentTime);
            interpreter.installSegment5 (opcodeSetDelete, new OpSetDelete<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetDeleteExplicit, new OpSetDelete<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetSquareRoot, new OpGetSquareRoot);
        }
    }
}
