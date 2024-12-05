#include "aiextensions.hpp"

#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aiactivate.hpp"
#include "../mwmechanics/aiescort.hpp"
#include "../mwmechanics/aiface.hpp"
#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/aiwander.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Ai
    {
        template <class R>
        class OpAiActivate : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId objectID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                // The value of the reset argument doesn't actually matter
                bool repeat = arg0;
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiActivate activatePackage(objectID, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(activatePackage, ptr);
                Log(Debug::Info) << "AiActivate";
            }
        };

        template <class R>
        class OpAiTravel : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float x = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float y = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float z = runtime[0].mFloat;
                runtime.pop();

                // The value of the reset argument doesn't actually matter
                bool repeat = arg0;
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiTravel travelPackage(x, y, z, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(travelPackage, ptr);

                Log(Debug::Info) << "AiTravel: " << x << ", " << y << ", " << z;
            }
        };

        template <class R>
        class OpAiEscort : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId actorID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Float duration = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float x = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float y = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float z = runtime[0].mFloat;
                runtime.pop();

                // The value of the reset argument doesn't actually matter
                bool repeat = arg0;
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiEscort escortPackage(actorID, static_cast<int>(duration), x, y, z, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(escortPackage, ptr);

                Log(Debug::Info) << "AiEscort: " << x << ", " << y << ", " << z << ", " << duration;
            }
        };

        template <class R>
        class OpAiEscortCell : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId actorID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                std::string_view cellID = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                Interpreter::Type_Float duration = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float x = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float y = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float z = runtime[0].mFloat;
                runtime.pop();

                // The value of the reset argument doesn't actually matter
                bool repeat = arg0;
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                if (cellID.empty())
                    return;

                if (!MWBase::Environment::get().getESMStore()->get<ESM::Cell>().search(cellID))
                    return;

                MWMechanics::AiEscort escortPackage(actorID, cellID, static_cast<int>(duration), x, y, z, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(escortPackage, ptr);

                Log(Debug::Info) << "AiEscort: " << x << ", " << y << ", " << z << ", " << duration;
            }
        };

        template <class R>
        class OpGetAiPackageDone : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                bool done = false;
                if (ptr.getClass().isActor())
                    done = ptr.getClass().getCreatureStats(ptr).getAiSequence().isPackageDone();

                runtime.push(done);
            }
        };

        template <class R>
        class OpAiWander : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer range = static_cast<Interpreter::Type_Integer>(runtime[0].mFloat);
                runtime.pop();

                Interpreter::Type_Integer duration = static_cast<Interpreter::Type_Integer>(runtime[0].mFloat);
                runtime.pop();

                Interpreter::Type_Integer time = static_cast<Interpreter::Type_Integer>(runtime[0].mFloat);
                runtime.pop();

                // Chance for Idle is unused
                if (arg0)
                {
                    --arg0;
                    runtime.pop();
                }

                std::vector<unsigned char> idleList;
                bool repeat = false;

                // Chances for Idle2-Idle9
                for (int i = 2; i <= 9 && arg0; ++i)
                {
                    if (!repeat)
                        repeat = true;
                    Interpreter::Type_Integer idleValue = std::clamp(runtime[0].mInteger, 0, 255);
                    idleList.push_back(idleValue);
                    runtime.pop();
                    --arg0;
                }

                if (arg0)
                {
                    repeat = runtime[0].mInteger != 0;
                    runtime.pop();
                    --arg0;
                }

                // discard additional arguments, because we have no idea what they mean.
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiWander wanderPackage(range, duration, time, idleList, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(wanderPackage, ptr);
            }
        };

        template <class R>
        class OpGetAiSetting : public Interpreter::Opcode0
        {
            MWMechanics::AiSetting mIndex;

        public:
            OpGetAiSetting(MWMechanics::AiSetting index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = 0;
                if (ptr.getClass().isActor())
                    value = ptr.getClass().getCreatureStats(ptr).getAiSetting(mIndex).getModified(false);
                runtime.push(value);
            }
        };
        template <class R>
        class OpModAiSetting : public Interpreter::Opcode0
        {
            MWMechanics::AiSetting mIndex;

        public:
            OpModAiSetting(MWMechanics::AiSetting index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                int modified = ptr.getClass().getCreatureStats(ptr).getAiSetting(mIndex).getBase() + value;

                ptr.getClass().getCreatureStats(ptr).setAiSetting(mIndex, modified);
                ptr.getClass().setBaseAISetting(ptr.getCellRef().getRefId(), mIndex, modified);
            }
        };
        template <class R>
        class OpSetAiSetting : public Interpreter::Opcode0
        {
            MWMechanics::AiSetting mIndex;

        public:
            OpSetAiSetting(MWMechanics::AiSetting index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();
                if (ptr.getClass().isActor())
                {
                    ptr.getClass().getCreatureStats(ptr).setAiSetting(mIndex, value);
                    ptr.getClass().setBaseAISetting(ptr.getCellRef().getRefId(), mIndex, value);
                }
            }
        };

        template <class R>
        class OpAiFollow : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId actorID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Float duration = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float x = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float y = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float z = runtime[0].mFloat;
                runtime.pop();

                // The value of the reset argument doesn't actually matter
                bool repeat = arg0;
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiFollow followPackage(actorID, duration, x, y, z, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(followPackage, ptr);

                Log(Debug::Info) << "AiFollow: " << actorID << ", " << x << ", " << y << ", " << z << ", " << duration;
            }
        };

        template <class R>
        class OpAiFollowCell : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId actorID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                std::string_view cellID = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                Interpreter::Type_Float duration = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float x = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float y = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float z = runtime[0].mFloat;
                runtime.pop();

                // The value of the reset argument doesn't actually matter
                bool repeat = arg0;
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor() || ptr == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiFollow followPackage(actorID, cellID, duration, x, y, z, repeat);
                ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(followPackage, ptr);
                Log(Debug::Info) << "AiFollow: " << actorID << ", " << x << ", " << y << ", " << z << ", " << duration;
            }
        };

        template <class R>
        class OpGetCurrentAIPackage : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = -1;
                if (ptr.getClass().isActor())
                {
                    const auto& stats = ptr.getClass().getCreatureStats(ptr);
                    if (!stats.isDead() || !stats.isDeathAnimationFinished())
                    {
                        value = static_cast<Interpreter::Type_Integer>(stats.getAiSequence().getLastRunTypeId());
                    }
                }

                runtime.push(value);
            }
        };

        template <class R>
        class OpGetDetected : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr observer = R()(runtime, false); // required=false

                ESM::RefId actorID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWWorld::Ptr actor = MWBase::Environment::get().getWorld()->searchPtr(actorID, true, false);

                Interpreter::Type_Integer value = 0;
                if (!actor.isEmpty())
                    value = MWBase::Environment::get().getMechanicsManager()->isActorDetected(actor, observer);

                runtime.push(value);
            }
        };

        template <class R>
        class OpGetLineOfSight : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {

                MWWorld::Ptr source = R()(runtime);

                ESM::RefId actorID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWWorld::Ptr dest = MWBase::Environment::get().getWorld()->searchPtr(actorID, true, false);
                bool value = false;
                if (!dest.isEmpty() && source.getClass().isActor() && dest.getClass().isActor())
                {
                    value = MWBase::Environment::get().getWorld()->getLOS(source, dest);
                }
                runtime.push(value);
            }
        };

        template <class R>
        class OpGetTarget : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr actor = R()(runtime);
                ESM::RefId testedTargetId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                bool targetsAreEqual = false;
                if (actor.getClass().isActor())
                {
                    const MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);
                    MWWorld::Ptr targetPtr;
                    if (creatureStats.getAiSequence().getCombatTarget(targetPtr))
                    {
                        if (!targetPtr.isEmpty() && targetPtr.getCellRef().getRefId() == testedTargetId)
                            targetsAreEqual = true;
                    }
                    else if (testedTargetId == "Player") // Currently the player ID is hardcoded
                    {
                        MWBase::MechanicsManager* mechMgr = MWBase::Environment::get().getMechanicsManager();
                        bool greeting = mechMgr->getGreetingState(actor) == MWMechanics::Greet_InProgress;
                        bool sayActive = MWBase::Environment::get().getSoundManager()->sayActive(actor);
                        targetsAreEqual = (greeting && sayActive) || mechMgr->isTurningToPlayer(actor);
                    }
                }
                runtime.push(targetsAreEqual);
            }
        };

        template <class R>
        class OpStartCombat : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr actor = R()(runtime);
                ESM::RefId targetID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(targetID, true, false);
                if (!target.isEmpty() && !target.getBase()->isDeleted()
                    && !target.getClass().getCreatureStats(target).isDead())
                    MWBase::Environment::get().getMechanicsManager()->startCombat(actor, target, nullptr);
            }
        };

        template <class R>
        class OpStopCombat : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr actor = R()(runtime);
                if (!actor.getClass().isActor())
                    return;
                MWBase::Environment::get().getMechanicsManager()->stopCombat(actor);
            }
        };

        class OpToggleAI : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getMechanicsManager()->toggleAI();

                runtime.getContext().report(enabled ? "AI -> On" : "AI -> Off");
            }
        };

        template <class R>
        class OpFace : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr actor = R()(runtime);

                Interpreter::Type_Float x = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float y = runtime[0].mFloat;
                runtime.pop();

                if (!actor.getClass().isActor() || actor == MWMechanics::getPlayer())
                    return;

                MWMechanics::AiFace facePackage(x, y);
                actor.getClass().getCreatureStats(actor).getAiSequence().stack(facePackage, actor);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment3<OpAiActivate<ImplicitRef>>(Compiler::Ai::opcodeAIActivate);
            interpreter.installSegment3<OpAiActivate<ExplicitRef>>(Compiler::Ai::opcodeAIActivateExplicit);
            interpreter.installSegment3<OpAiTravel<ImplicitRef>>(Compiler::Ai::opcodeAiTravel);
            interpreter.installSegment3<OpAiTravel<ExplicitRef>>(Compiler::Ai::opcodeAiTravelExplicit);
            interpreter.installSegment3<OpAiEscort<ImplicitRef>>(Compiler::Ai::opcodeAiEscort);
            interpreter.installSegment3<OpAiEscort<ExplicitRef>>(Compiler::Ai::opcodeAiEscortExplicit);
            interpreter.installSegment3<OpAiEscortCell<ImplicitRef>>(Compiler::Ai::opcodeAiEscortCell);
            interpreter.installSegment3<OpAiEscortCell<ExplicitRef>>(Compiler::Ai::opcodeAiEscortCellExplicit);
            interpreter.installSegment3<OpAiWander<ImplicitRef>>(Compiler::Ai::opcodeAiWander);
            interpreter.installSegment3<OpAiWander<ExplicitRef>>(Compiler::Ai::opcodeAiWanderExplicit);
            interpreter.installSegment3<OpAiFollow<ImplicitRef>>(Compiler::Ai::opcodeAiFollow);
            interpreter.installSegment3<OpAiFollow<ExplicitRef>>(Compiler::Ai::opcodeAiFollowExplicit);
            interpreter.installSegment3<OpAiFollowCell<ImplicitRef>>(Compiler::Ai::opcodeAiFollowCell);
            interpreter.installSegment3<OpAiFollowCell<ExplicitRef>>(Compiler::Ai::opcodeAiFollowCellExplicit);
            interpreter.installSegment5<OpGetAiPackageDone<ImplicitRef>>(Compiler::Ai::opcodeGetAiPackageDone);

            interpreter.installSegment5<OpGetAiPackageDone<ExplicitRef>>(Compiler::Ai::opcodeGetAiPackageDoneExplicit);
            interpreter.installSegment5<OpGetCurrentAIPackage<ImplicitRef>>(Compiler::Ai::opcodeGetCurrentAiPackage);
            interpreter.installSegment5<OpGetCurrentAIPackage<ExplicitRef>>(
                Compiler::Ai::opcodeGetCurrentAiPackageExplicit);
            interpreter.installSegment5<OpGetDetected<ImplicitRef>>(Compiler::Ai::opcodeGetDetected);
            interpreter.installSegment5<OpGetDetected<ExplicitRef>>(Compiler::Ai::opcodeGetDetectedExplicit);
            interpreter.installSegment5<OpGetLineOfSight<ImplicitRef>>(Compiler::Ai::opcodeGetLineOfSight);
            interpreter.installSegment5<OpGetLineOfSight<ExplicitRef>>(Compiler::Ai::opcodeGetLineOfSightExplicit);
            interpreter.installSegment5<OpGetTarget<ImplicitRef>>(Compiler::Ai::opcodeGetTarget);
            interpreter.installSegment5<OpGetTarget<ExplicitRef>>(Compiler::Ai::opcodeGetTargetExplicit);
            interpreter.installSegment5<OpStartCombat<ImplicitRef>>(Compiler::Ai::opcodeStartCombat);
            interpreter.installSegment5<OpStartCombat<ExplicitRef>>(Compiler::Ai::opcodeStartCombatExplicit);
            interpreter.installSegment5<OpStopCombat<ImplicitRef>>(Compiler::Ai::opcodeStopCombat);
            interpreter.installSegment5<OpStopCombat<ExplicitRef>>(Compiler::Ai::opcodeStopCombatExplicit);
            interpreter.installSegment5<OpToggleAI>(Compiler::Ai::opcodeToggleAI);

            interpreter.installSegment5<OpSetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeSetHello, MWMechanics::AiSetting::Hello);
            interpreter.installSegment5<OpSetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeSetHelloExplicit, MWMechanics::AiSetting::Hello);
            interpreter.installSegment5<OpSetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeSetFight, MWMechanics::AiSetting::Fight);
            interpreter.installSegment5<OpSetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeSetFightExplicit, MWMechanics::AiSetting::Fight);
            interpreter.installSegment5<OpSetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeSetFlee, MWMechanics::AiSetting::Flee);
            interpreter.installSegment5<OpSetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeSetFleeExplicit, MWMechanics::AiSetting::Flee);
            interpreter.installSegment5<OpSetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeSetAlarm, MWMechanics::AiSetting::Alarm);
            interpreter.installSegment5<OpSetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeSetAlarmExplicit, MWMechanics::AiSetting::Alarm);

            interpreter.installSegment5<OpModAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeModHello, MWMechanics::AiSetting::Hello);
            interpreter.installSegment5<OpModAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeModHelloExplicit, MWMechanics::AiSetting::Hello);
            interpreter.installSegment5<OpModAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeModFight, MWMechanics::AiSetting::Fight);
            interpreter.installSegment5<OpModAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeModFightExplicit, MWMechanics::AiSetting::Fight);
            interpreter.installSegment5<OpModAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeModFlee, MWMechanics::AiSetting::Flee);
            interpreter.installSegment5<OpModAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeModFleeExplicit, MWMechanics::AiSetting::Flee);
            interpreter.installSegment5<OpModAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeModAlarm, MWMechanics::AiSetting::Alarm);
            interpreter.installSegment5<OpModAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeModAlarmExplicit, MWMechanics::AiSetting::Alarm);

            interpreter.installSegment5<OpGetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeGetHello, MWMechanics::AiSetting::Hello);
            interpreter.installSegment5<OpGetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeGetHelloExplicit, MWMechanics::AiSetting::Hello);
            interpreter.installSegment5<OpGetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeGetFight, MWMechanics::AiSetting::Fight);
            interpreter.installSegment5<OpGetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeGetFightExplicit, MWMechanics::AiSetting::Fight);
            interpreter.installSegment5<OpGetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeGetFlee, MWMechanics::AiSetting::Flee);
            interpreter.installSegment5<OpGetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeGetFleeExplicit, MWMechanics::AiSetting::Flee);
            interpreter.installSegment5<OpGetAiSetting<ImplicitRef>>(
                Compiler::Ai::opcodeGetAlarm, MWMechanics::AiSetting::Alarm);
            interpreter.installSegment5<OpGetAiSetting<ExplicitRef>>(
                Compiler::Ai::opcodeGetAlarmExplicit, MWMechanics::AiSetting::Alarm);

            interpreter.installSegment5<OpFace<ImplicitRef>>(Compiler::Ai::opcodeFace);
            interpreter.installSegment5<OpFace<ExplicitRef>>(Compiler::Ai::opcodeFaceExplicit);
        }
    }
}
