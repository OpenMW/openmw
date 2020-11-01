#include "aiextensions.hpp"

#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/aiactivate.hpp"
#include "../mwmechanics/aiescort.hpp"
#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/aiwander.hpp"
#include "../mwmechanics/aiface.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"


namespace MWScript
{
    namespace Ai
    {
        template<class R>
        class OpAiActivate : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string objectID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    MWMechanics::AiActivate activatePackage(objectID);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(activatePackage, ptr);
                    Log(Debug::Info) << "AiActivate";
                }
        };

        template<class R>
        class OpAiTravel : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    MWMechanics::AiTravel travelPackage(x, y, z);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(travelPackage, ptr);

                    Log(Debug::Info) << "AiTravel: " << x << ", " << y << ", " << z;
                }
        };

        template<class R>
        class OpAiEscort : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string actorID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float duration = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    MWMechanics::AiEscort escortPackage(actorID, static_cast<int>(duration), x, y, z);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(escortPackage, ptr);

                    Log(Debug::Info) << "AiEscort: " << x << ", " << y << ", " << z << ", " << duration;
                }
        };

        template<class R>
        class OpAiEscortCell : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string actorID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string cellID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float duration = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    if (cellID.empty())
                        throw std::runtime_error("AiEscortCell: no cell ID given");

                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>().find(cellID);

                    MWMechanics::AiEscort escortPackage(actorID, cellID, static_cast<int>(duration), x, y, z);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(escortPackage, ptr);

                    Log(Debug::Info) << "AiEscort: " << x << ", " << y << ", " << z << ", " << duration;
                }
        };

        template<class R>
        class OpGetAiPackageDone : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = ptr.getClass().getCreatureStats (ptr).getAiSequence().isPackageDone();

                    runtime.push (value);
                }
        };

        template<class R>
        class OpAiWander : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
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
                    for(int i=2; i<=9 && arg0; ++i)
                    {
                        if(!repeat)
                            repeat = true;
                        Interpreter::Type_Integer idleValue = runtime[0].mInteger;
                        idleValue = std::min(255, std::max(0, idleValue));
                        idleList.push_back(idleValue);
                        runtime.pop();
                        --arg0;
                    }

                    if(arg0)
                    {
                        repeat = runtime[0].mInteger != 0;
                        runtime.pop();
                        --arg0;
                    }

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    MWMechanics::AiWander wanderPackage(range, duration, time, idleList, repeat);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(wanderPackage, ptr);
                }
        };

        template<class R>
        class OpGetAiSetting : public Interpreter::Opcode0
        {
            MWMechanics::CreatureStats::AiSetting mIndex;
            public:
                OpGetAiSetting(MWMechanics::CreatureStats::AiSetting index) : mIndex(index) {}

                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push(ptr.getClass().getCreatureStats (ptr).getAiSetting (mIndex).getModified());
                }
        };
        template<class R>
        class OpModAiSetting : public Interpreter::Opcode0
        {
            MWMechanics::CreatureStats::AiSetting mIndex;
            public:
                OpModAiSetting(MWMechanics::CreatureStats::AiSetting index) : mIndex(index) {}

                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    int modified = ptr.getClass().getCreatureStats (ptr).getAiSetting (mIndex).getBase() + value;

                    ptr.getClass().getCreatureStats (ptr).setAiSetting (mIndex, modified);
                    ptr.getClass().setBaseAISetting(ptr.getCellRef().getRefId(), mIndex, modified);
                }
        };
        template<class R>
        class OpSetAiSetting : public Interpreter::Opcode0
        {
            MWMechanics::CreatureStats::AiSetting mIndex;
            public:
                OpSetAiSetting(MWMechanics::CreatureStats::AiSetting index) : mIndex(index) {}

                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::Stat<int> stat = ptr.getClass().getCreatureStats(ptr).getAiSetting(mIndex);
                    stat.setModified(value, 0);
                    ptr.getClass().getCreatureStats(ptr).setAiSetting(mIndex, stat);
                    ptr.getClass().setBaseAISetting(ptr.getCellRef().getRefId(), mIndex, value);
                }
        };

        template<class R>
        class OpAiFollow : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string actorID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float duration = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    MWMechanics::AiFollow followPackage(actorID, duration, x, y ,z);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(followPackage, ptr);

                    Log(Debug::Info) << "AiFollow: " << actorID << ", " << x << ", " << y << ", " << z << ", " << duration;
                }
        };

        template<class R>
        class OpAiFollowCell : public Interpreter::Opcode1
        {
            public:

                void execute (Interpreter::Runtime& runtime, unsigned int arg0) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string actorID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string cellID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Float duration = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();

                    // discard additional arguments (reset), because we have no idea what they mean.
                    for (unsigned int i=0; i<arg0; ++i) runtime.pop();

                    MWMechanics::AiFollow followPackage(actorID, cellID, duration, x, y ,z);
                    ptr.getClass().getCreatureStats (ptr).getAiSequence().stack(followPackage, ptr);
                    Log(Debug::Info) << "AiFollow: " << actorID << ", " << x << ", " << y << ", " << z << ", " << duration;
                }
        };

        template<class R>
        class OpGetCurrentAIPackage : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    const auto value = static_cast<Interpreter::Type_Integer>(ptr.getClass().getCreatureStats (ptr).getAiSequence().getLastRunTypeId());

                    runtime.push (value);
                }
        };

        template<class R>
        class OpGetDetected : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr observer = R()(runtime, false); // required=false

                    std::string actorID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr actor = MWBase::Environment::get().getWorld()->searchPtr(actorID, true, false);

                    Interpreter::Type_Integer value = 0;
                    if (!actor.isEmpty())
                        value = MWBase::Environment::get().getMechanicsManager()->isActorDetected(actor, observer);

                    runtime.push (value);
                }
        };

        template<class R>
        class OpGetLineOfSight : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {

                    MWWorld::Ptr source = R()(runtime);

                    std::string actorID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();


                    MWWorld::Ptr dest = MWBase::Environment::get().getWorld()->searchPtr(actorID, true, false);
                    bool value = false;
                    if (!dest.isEmpty() && source.getClass().isActor() && dest.getClass().isActor())
                    {
                        value = MWBase::Environment::get().getWorld()->getLOS(source,dest);
                    }
                    runtime.push (value);
                }
        };

        template<class R>
        class OpGetTarget : public Interpreter::Opcode0
        {
            public:
                void execute (Interpreter::Runtime &runtime) override
                {
                    MWWorld::Ptr actor = R()(runtime);
                    std::string testedTargetId = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    const MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);

                    bool targetsAreEqual = false;
                    MWWorld::Ptr targetPtr;
                    if (creatureStats.getAiSequence().getCombatTarget (targetPtr))
                    {
                        if (!targetPtr.isEmpty() && targetPtr.getCellRef().getRefId() == testedTargetId)
                            targetsAreEqual = true;
                    }
                    else if (testedTargetId == "player") // Currently the player ID is hardcoded
                    {
                        MWBase::MechanicsManager* mechMgr = MWBase::Environment::get().getMechanicsManager();
                        bool greeting = mechMgr->getGreetingState(actor) == MWMechanics::Greet_InProgress;
                        bool sayActive = MWBase::Environment::get().getSoundManager()->sayActive(actor);
                        targetsAreEqual = (greeting && sayActive) || mechMgr->isTurningToPlayer(actor);
                    }
                    runtime.push(int(targetsAreEqual));
                }
        };

        template<class R>
        class OpStartCombat : public Interpreter::Opcode0
        {
            public:
                void execute (Interpreter::Runtime &runtime) override
                {
                    MWWorld::Ptr actor = R()(runtime);
                    std::string targetID = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(targetID, true, false);
                    if (!target.isEmpty())
                        MWBase::Environment::get().getMechanicsManager()->startCombat(actor, target);
                }
        };

        template<class R>
        class OpStopCombat : public Interpreter::Opcode0
        {
            public:
                void execute (Interpreter::Runtime& runtime) override
                {
                    MWWorld::Ptr actor = R()(runtime);
                    MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);
                    creatureStats.getAiSequence().stopCombat();
                }
        };

        class OpToggleAI : public Interpreter::Opcode0
        {
            public:

                void execute (Interpreter::Runtime& runtime) override
                {
                    bool enabled = MWBase::Environment::get().getMechanicsManager()->toggleAI();

                    runtime.getContext().report (enabled ? "AI -> On" : "AI -> Off");
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

                MWMechanics::AiFace facePackage(x, y);
                actor.getClass().getCreatureStats(actor).getAiSequence().stack(facePackage, actor);
            }
        };

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment3 (Compiler::Ai::opcodeAIActivate, new OpAiActivate<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAIActivateExplicit, new OpAiActivate<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiTravel, new OpAiTravel<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiTravelExplicit, new OpAiTravel<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiEscort, new OpAiEscort<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiEscortExplicit, new OpAiEscort<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiEscortCell, new OpAiEscortCell<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiEscortCellExplicit, new OpAiEscortCell<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiWander, new OpAiWander<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiWanderExplicit, new OpAiWander<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiFollow, new OpAiFollow<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiFollowExplicit, new OpAiFollow<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiFollowCell, new OpAiFollowCell<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Ai::opcodeAiFollowCellExplicit, new OpAiFollowCell<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetAiPackageDone, new OpGetAiPackageDone<ImplicitRef>);

            interpreter.installSegment5 (Compiler::Ai::opcodeGetAiPackageDoneExplicit,
                new OpGetAiPackageDone<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetCurrentAiPackage, new OpGetCurrentAIPackage<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetCurrentAiPackageExplicit, new OpGetCurrentAIPackage<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetDetected, new OpGetDetected<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetDetectedExplicit, new OpGetDetected<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetLineOfSight, new OpGetLineOfSight<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetLineOfSightExplicit, new OpGetLineOfSight<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetTarget, new OpGetTarget<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeGetTargetExplicit, new OpGetTarget<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeStartCombat, new OpStartCombat<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeStartCombatExplicit, new OpStartCombat<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeStopCombat, new OpStopCombat<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeStopCombatExplicit, new OpStopCombat<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeToggleAI, new OpToggleAI);

            interpreter.installSegment5 (Compiler::Ai::opcodeSetHello, new OpSetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Hello));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetHelloExplicit, new OpSetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Hello));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetFight, new OpSetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Fight));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetFightExplicit, new OpSetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Fight));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetFlee, new OpSetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Flee));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetFleeExplicit, new OpSetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Flee));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetAlarm, new OpSetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Alarm));
            interpreter.installSegment5 (Compiler::Ai::opcodeSetAlarmExplicit, new OpSetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Alarm));

            interpreter.installSegment5 (Compiler::Ai::opcodeModHello, new OpModAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Hello));
            interpreter.installSegment5 (Compiler::Ai::opcodeModHelloExplicit, new OpModAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Hello));
            interpreter.installSegment5 (Compiler::Ai::opcodeModFight, new OpModAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Fight));
            interpreter.installSegment5 (Compiler::Ai::opcodeModFightExplicit, new OpModAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Fight));
            interpreter.installSegment5 (Compiler::Ai::opcodeModFlee, new OpModAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Flee));
            interpreter.installSegment5 (Compiler::Ai::opcodeModFleeExplicit, new OpModAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Flee));
            interpreter.installSegment5 (Compiler::Ai::opcodeModAlarm, new OpModAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Alarm));
            interpreter.installSegment5 (Compiler::Ai::opcodeModAlarmExplicit, new OpModAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Alarm));

            interpreter.installSegment5 (Compiler::Ai::opcodeGetHello, new OpGetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Hello));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetHelloExplicit, new OpGetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Hello));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetFight, new OpGetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Fight));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetFightExplicit, new OpGetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Fight));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetFlee, new OpGetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Flee));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetFleeExplicit, new OpGetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Flee));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetAlarm, new OpGetAiSetting<ImplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Alarm));
            interpreter.installSegment5 (Compiler::Ai::opcodeGetAlarmExplicit, new OpGetAiSetting<ExplicitRef>(MWMechanics::CreatureStats::AiSetting::AI_Alarm));

            interpreter.installSegment5 (Compiler::Ai::opcodeFace, new OpFace<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Ai::opcodeFaceExplicit, new OpFace<ExplicitRef>);
        }
    }
}
