#include "actors.hpp"

#include <array>
#include <optional>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/debug/debuglog.hpp>
#include <components/misc/mathutil.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/settings/values.hpp>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadstat.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/scene.hpp"

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/aibreathe.hpp"

#include "../mwrender/vismask.hpp"

#include "../mwsound/constants.hpp"

#include "actor.hpp"
#include "actorutil.hpp"
#include "aicombataction.hpp"
#include "aifollow.hpp"
#include "aipursue.hpp"
#include "aiwander.hpp"
#include "attacktype.hpp"
#include "character.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "npcstats.hpp"
#include "steering.hpp"
#include "summoning.hpp"

namespace
{

    bool isConscious(const MWWorld::Ptr& ptr)
    {
        const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        return !stats.isDead() && !stats.getKnockedDown();
    }

    bool isCommanded(const MWWorld::Ptr& actor)
    {
        const auto& actorClass = actor.getClass();
        const auto& stats = actorClass.getCreatureStats(actor);
        const bool isActorNpc = actorClass.isNpc();
        const auto level = stats.getLevel();
        for (const auto& params : stats.getActiveSpells())
        {
            for (const auto& effect : params.getEffects())
            {
                if (((effect.mEffectId == ESM::MagicEffect::CommandHumanoid && isActorNpc)
                        || (effect.mEffectId == ESM::MagicEffect::CommandCreature && !isActorNpc))
                    && effect.mMagnitude >= level)
                    return true;
            }
        }
        return false;
    }

    // Check for command effects having ended and remove package if necessary
    void adjustCommandedActor(const MWWorld::Ptr& actor)
    {
        if (isCommanded(actor))
            return;

        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        stats.getAiSequence().erasePackageIf([](auto& entry) {
            if (entry->getTypeId() == MWMechanics::AiPackageTypeId::Follow
                && static_cast<const MWMechanics::AiFollow*>(entry.get())->isCommanded())
            {
                return true;
            }
            return false;
        });
    }

    std::pair<float, float> getRestorationPerHourOfSleep(const MWWorld::Ptr& ptr)
    {
        const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        const MWWorld::Store<ESM::GameSetting>& settings
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        const float endurance = stats.getAttribute(ESM::Attribute::Endurance).getModified();
        const float health = 0.1f * endurance;

        static const float fRestMagicMult = settings.find("fRestMagicMult")->mValue.getFloat();
        const float magicka = fRestMagicMult * stats.getAttribute(ESM::Attribute::Intelligence).getModified();

        return { health, magicka };
    }

    template <class T>
    void forEachFollowingPackage(
        const std::list<MWMechanics::Actor>& actors, const MWWorld::Ptr& actorPtr, const MWWorld::Ptr& player, T&& func)
    {
        for (const MWMechanics::Actor& actor : actors)
        {
            const MWWorld::Ptr& iteratedActor = actor.getPtr();
            if (iteratedActor == player || iteratedActor == actorPtr)
                continue;

            const MWMechanics::CreatureStats& stats = iteratedActor.getClass().getCreatureStats(iteratedActor);
            if (stats.isDead())
                continue;

            // An actor counts as following if AiFollow is the current AiPackage,
            // or there are only Combat and Wander packages before the AiFollow package
            for (const auto& package : stats.getAiSequence())
            {
                if (!func(actor, package))
                    break;
            }
        }
    }

    float getStuntedMagickaDuration(const MWWorld::Ptr& actor)
    {
        float remainingTime = 0.f;
        for (const auto& params : actor.getClass().getCreatureStats(actor).getActiveSpells())
        {
            for (const auto& effect : params.getEffects())
            {
                if (effect.mEffectId == ESM::MagicEffect::StuntedMagicka)
                {
                    if (effect.mDuration == -1.f)
                        return -1.f;
                    remainingTime = std::max(remainingTime, effect.mTimeLeft);
                }
            }
        }
        return remainingTime;
    }

    void soulTrap(const MWWorld::Ptr& creature)
    {
        const auto& stats = creature.getClass().getCreatureStats(creature);
        if (!stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Soultrap).getMagnitude())
            return;
        const int creatureSoulValue = creature.get<ESM::Creature>()->mBase->mData.mSoul;
        if (creatureSoulValue == 0)
            return;
        MWBase::World* const world = MWBase::Environment::get().getWorld();
        static const float fSoulgemMult
            = world->getStore().get<ESM::GameSetting>().find("fSoulgemMult")->mValue.getFloat();
        for (const auto& params : stats.getActiveSpells())
        {
            for (const auto& effect : params.getEffects())
            {
                if (effect.mEffectId != ESM::MagicEffect::Soultrap || effect.mMagnitude <= 0.f)
                    continue;
                MWWorld::Ptr caster = world->searchPtrViaActorId(params.getCasterActorId());
                if (caster.isEmpty() || !caster.getClass().isActor())
                    continue;

                // Use the smallest soulgem that is large enough to hold the soul
                MWWorld::ContainerStore& container = caster.getClass().getContainerStore(caster);
                MWWorld::ContainerStoreIterator gem = container.end();
                float gemCapacity = std::numeric_limits<float>::max();
                for (auto it = container.begin(MWWorld::ContainerStore::Type_Miscellaneous); it != container.end();
                     ++it)
                {
                    if (it->getClass().isSoulGem(*it))
                    {
                        float thisGemCapacity = it->get<ESM::Miscellaneous>()->mBase->mData.mValue * fSoulgemMult;
                        if (thisGemCapacity >= creatureSoulValue && thisGemCapacity < gemCapacity
                            && it->getCellRef().getSoul().empty())
                        {
                            gem = it;
                            gemCapacity = thisGemCapacity;
                        }
                    }
                }

                if (gem == container.end())
                    continue;

                // Set the soul on just one of the gems, not the whole stack
                gem->getContainerStore()->unstack(*gem);
                gem->getCellRef().setSoul(creature.getCellRef().getRefId());

                // Restack the gem with other gems with the same soul
                gem->getContainerStore()->restack(*gem);

                if (caster == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sSoultrapSuccess}");

                const ESM::Static* const fx
                    = world->getStore().get<ESM::Static>().search(ESM::RefId::stringRefId("VFX_Soul_Trap"));
                if (fx != nullptr)
                    world->spawnEffect(Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(fx->mModel)), "",
                        creature.getRefData().getPosition().asVec3());

                MWBase::Environment::get().getSoundManager()->playSound3D(
                    creature.getRefData().getPosition().asVec3(), ESM::RefId::stringRefId("conjuration hit"), 1.f, 1.f);
                return; // remove to get vanilla behaviour
            }
        }
    }

    void removeTemporaryEffects(const MWWorld::Ptr& ptr)
    {
        ptr.getClass().getCreatureStats(ptr).getActiveSpells().unloadActor(ptr);
    }

}

namespace MWMechanics
{
    static constexpr int GREETING_SHOULD_START = 4; // how many updates should pass before NPC can greet player
    static constexpr int GREETING_SHOULD_END = 20; // how many updates should pass before NPC stops turning to player
    static constexpr int GREETING_COOLDOWN = 40; // how many updates should pass before NPC can continue movement
    static constexpr float DECELERATE_DISTANCE = 512.f;

    namespace
    {
        std::string_view attackTypeName(AttackType attackType)
        {
            switch (attackType)
            {
                case AttackType::NoAttack:
                case AttackType::Any:
                    return {};
                case AttackType::Chop:
                    return "chop";
                case AttackType::Slash:
                    return "slash";
                case AttackType::Thrust:
                    return "thrust";
            }
            throw std::logic_error("Invalid attack type value: " + std::to_string(static_cast<int>(attackType)));
        }

        float getTimeToDestination(const AiPackage& package, const osg::Vec3f& position, float speed, float duration,
            const osg::Vec3f& halfExtents)
        {
            const auto distanceToNextPathPoint
                = (package.getNextPathPoint(package.getDestination()) - position).length();
            return (distanceToNextPathPoint - package.getNextPathPointTolerance(speed, duration, halfExtents)) / speed;
        }

        void updateHeadTracking(const MWWorld::Ptr& actor, const MWWorld::Ptr& targetActor,
            MWWorld::Ptr& headTrackTarget, float& sqrHeadTrackDistance, bool inCombatOrPursue)
        {
            const auto& actorRefData = actor.getRefData();
            if (!actorRefData.getBaseNode())
                return;

            if (targetActor.getClass().getCreatureStats(targetActor).isDead())
                return;

            if (isTargetMagicallyHidden(targetActor))
                return;

            static const float fMaxHeadTrackDistance = MWBase::Environment::get()
                                                           .getESMStore()
                                                           ->get<ESM::GameSetting>()
                                                           .find("fMaxHeadTrackDistance")
                                                           ->mValue.getFloat();
            static const float fInteriorHeadTrackMult = MWBase::Environment::get()
                                                            .getESMStore()
                                                            ->get<ESM::GameSetting>()
                                                            .find("fInteriorHeadTrackMult")
                                                            ->mValue.getFloat();
            float maxDistance = fMaxHeadTrackDistance;
            auto currentCell = actor.getCell()->getCell();
            if (!currentCell->isExterior() && !(currentCell->isQuasiExterior()))
                maxDistance *= fInteriorHeadTrackMult;

            const osg::Vec3f actor1Pos(actorRefData.getPosition().asVec3());
            const osg::Vec3f actor2Pos(targetActor.getRefData().getPosition().asVec3());
            const float sqrDist = (actor1Pos - actor2Pos).length2();

            if (sqrDist > std::min(maxDistance * maxDistance, sqrHeadTrackDistance) && !inCombatOrPursue)
                return;

            // stop tracking when target is behind the actor
            osg::Vec3f actorDirection = actorRefData.getBaseNode()->getAttitude() * osg::Vec3f(0, 1, 0);
            osg::Vec3f targetDirection(actor2Pos - actor1Pos);
            actorDirection.z() = 0;
            targetDirection.z() = 0;
            if ((actorDirection * targetDirection > 0 || inCombatOrPursue)
                // check LOS and awareness last as it's the most expensive function
                && MWBase::Environment::get().getWorld()->getLOS(actor, targetActor)
                && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(targetActor, actor))
            {
                sqrHeadTrackDistance = sqrDist;
                headTrackTarget = targetActor;
            }
        }

        void updateHeadTracking(
            const MWWorld::Ptr& ptr, const std::list<Actor>& actors, bool isPlayer, CharacterController& ctrl)
        {
            float sqrHeadTrackDistance = std::numeric_limits<float>::max();
            MWWorld::Ptr headTrackTarget;

            const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
            const bool firstPersonPlayer = isPlayer && MWBase::Environment::get().getWorld()->isFirstPerson();

            // 1. Unconsious actor can not track target
            // 2. Actors in combat and pursue mode do not bother to headtrack anyone except their target
            // 3. Player character does not use headtracking in the 1st-person view
            if (!stats.getKnockedDown() && !firstPersonPlayer)
            {
                bool inCombatOrPursue = stats.getAiSequence().isInCombat() || stats.getAiSequence().isInPursuit();
                if (inCombatOrPursue)
                {
                    auto activePackageTarget = stats.getAiSequence().getActivePackage().getTarget();
                    if (!activePackageTarget.isEmpty())
                    {
                        // Track the specified target of package.
                        updateHeadTracking(
                            ptr, activePackageTarget, headTrackTarget, sqrHeadTrackDistance, inCombatOrPursue);
                    }
                }
                else
                {
                    // Find something nearby.
                    for (const Actor& otherActor : actors)
                    {
                        if (otherActor.getPtr() == ptr)
                            continue;

                        updateHeadTracking(
                            ptr, otherActor.getPtr(), headTrackTarget, sqrHeadTrackDistance, inCombatOrPursue);
                    }
                }
            }

            ctrl.setHeadTrackTarget(headTrackTarget);
        }

        void updateLuaControls(const MWWorld::Ptr& ptr, bool isPlayer, MWBase::LuaManager::ActorControls& controls)
        {
            Movement& mov = ptr.getClass().getMovementSettings(ptr);
            CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
            const float speedFactor = isPlayer ? 1.f : mov.mSpeedFactor;
            const osg::Vec2f movement = osg::Vec2f(mov.mPosition[0], mov.mPosition[1]) * speedFactor;
            const float rotationX = mov.mRotation[0];
            const float rotationZ = mov.mRotation[2];
            const bool jump = mov.mPosition[2] == 1;
            const bool runFlag = stats.getMovementFlag(MWMechanics::CreatureStats::Flag_Run);
            const bool sneakFlag = stats.getMovementFlag(MWMechanics::CreatureStats::Flag_Sneak);
            const bool attackingOrSpell = stats.getAttackingOrSpell();
            if (controls.mChanged)
            {
                mov.mPosition[0] = controls.mSideMovement;
                mov.mPosition[1] = controls.mMovement;
                if (controls.mJump)
                    mov.mPosition[2] = 1;
                mov.mRotation[0] = controls.mPitchChange;
                mov.mRotation[1] = 0;
                mov.mRotation[2] = controls.mYawChange;
                mov.mSpeedFactor = osg::Vec2(controls.mMovement, controls.mSideMovement).length();
                stats.setMovementFlag(MWMechanics::CreatureStats::Flag_Run, controls.mRun);
                stats.setMovementFlag(MWMechanics::CreatureStats::Flag_Sneak, controls.mSneak);

                AttackType attackType = static_cast<AttackType>(controls.mUse);
                stats.setAttackingOrSpell(attackType != AttackType::NoAttack);
                stats.setAttackType(attackTypeName(attackType));

                controls.mChanged = false;
            }
            // For the player we don't need to copy these values to Lua because mwinput doesn't change them.
            // All handling of these player controls was moved from C++ to a built-in Lua script.
            if (!isPlayer)
            {
                controls.mSideMovement = movement.x();
                controls.mMovement = movement.y();
                controls.mJump = jump;
                controls.mRun = runFlag;
                controls.mSneak = sneakFlag;
                controls.mUse = attackingOrSpell ? controls.mUse | 1 : controls.mUse & ~1;
            }
            // For the player these controls are still handled by mwinput, so we need to update the values.
            controls.mPitchChange = rotationX;
            controls.mYawChange = rotationZ;
        }
    }

    void Actors::updateActor(const MWWorld::Ptr& ptr, float duration) const
    {
        // magic effects
        adjustMagicEffects(ptr, duration);

        // fatigue restoration
        calculateRestoration(ptr, duration);
    }

    void Actors::playIdleDialogue(const MWWorld::Ptr& actor) const
    {
        if (!actor.getClass().isActor() || actor == getPlayer()
            || MWBase::Environment::get().getSoundManager()->sayActive(actor))
            return;

        const CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        if (stats.getAiSetting(AiSetting::Hello).getModified() == 0)
            return;

        const MWMechanics::AiSequence& seq = stats.getAiSequence();
        if (seq.isInCombat() || seq.hasPackage(AiPackageTypeId::Follow) || seq.hasPackage(AiPackageTypeId::Escort))
            return;

        const osg::Vec3f playerPos(getPlayer().getRefData().getPosition().asVec3());
        const osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        MWBase::World* const world = MWBase::Environment::get().getWorld();
        if (world->isSwimming(actor) || (playerPos - actorPos).length2() >= 3000 * 3000)
            return;

        // Our implementation is not FPS-dependent unlike Morrowind's so it needs to be recalibrated.
        // We chose to use the chance MW would have when run at 60 FPS with the default value of the GMST.
        const float delta = MWBase::Environment::get().getFrameDuration() * 6.f;
        static const float fVoiceIdleOdds
            = world->getStore().get<ESM::GameSetting>().find("fVoiceIdleOdds")->mValue.getFloat();
        if (Misc::Rng::rollProbability(world->getPrng()) * 10000.f < fVoiceIdleOdds * delta
            && world->getLOS(getPlayer(), actor))
            MWBase::Environment::get().getDialogueManager()->say(actor, ESM::RefId::stringRefId("idle"));
    }

    void Actors::updateMovementSpeed(const MWWorld::Ptr& actor) const
    {
        if (Settings::game().mSmoothMovement)
            return;

        const auto& actorClass = actor.getClass();
        const CreatureStats& stats = actorClass.getCreatureStats(actor);
        const MWMechanics::AiSequence& seq = stats.getAiSequence();

        if (!seq.isEmpty() && seq.getActivePackage().useVariableSpeed())
        {
            const osg::Vec3f targetPos = seq.getActivePackage().getDestination();
            const osg::Vec3f actorPos = actor.getRefData().getPosition().asVec3();
            const float distance = (targetPos - actorPos).length();

            if (distance < DECELERATE_DISTANCE)
            {
                const float speedCoef = std::max(0.7f, 0.2f + 0.8f * distance / DECELERATE_DISTANCE);
                auto& movement = actorClass.getMovementSettings(actor);
                movement.mPosition[0] *= speedCoef;
                movement.mPosition[1] *= speedCoef;
            }
        }
    }

    void Actors::updateGreetingState(const MWWorld::Ptr& actor, Actor& actorState, bool turnOnly)
    {
        const auto& actorClass = actor.getClass();
        if (!actorClass.isActor() || actor == getPlayer())
            return;

        const CreatureStats& actorStats = actorClass.getCreatureStats(actor);
        const MWMechanics::AiSequence& seq = actorStats.getAiSequence();
        const auto packageId = seq.getTypeId();

        if (seq.isInCombat() || MWBase::Environment::get().getWorld()->isSwimming(actor)
            || (packageId != AiPackageTypeId::Wander && packageId != AiPackageTypeId::Travel
                && packageId != AiPackageTypeId::None))
        {
            actorState.setTurningToPlayer(false);
            actorState.setGreetingTimer(0);
            actorState.setGreetingState(Greet_None);
            return;
        }

        const MWWorld::Ptr player = getPlayer();
        const osg::Vec3f playerPos(player.getRefData().getPosition().asVec3());
        const osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        const osg::Vec3f dir = playerPos - actorPos;

        if (actorState.isTurningToPlayer())
        {
            // Reduce the turning animation glitch by using a *HUGE* value of
            // epsilon...  TODO: a proper fix might be in either the physics or the
            // animation subsystem
            if (zTurn(actor, actorState.getAngleToPlayer(), osg::DegreesToRadians(5.f)))
            {
                actorState.setTurningToPlayer(false);
                // An original engine launches an endless idle2 when an actor greets player.
                playAnimationGroup(actor, "idle2", 0, std::numeric_limits<int>::max(), false);
            }
        }

        if (turnOnly)
            return;

        // Play a random voice greeting if the player gets too close
        static const int iGreetDistanceMultiplier = MWBase::Environment::get()
                                                        .getESMStore()
                                                        ->get<ESM::GameSetting>()
                                                        .find("iGreetDistanceMultiplier")
                                                        ->mValue.getInteger();

        const float helloDistance
            = static_cast<float>(actorStats.getAiSetting(AiSetting::Hello).getModified() * iGreetDistanceMultiplier);
        const auto& playerStats = player.getClass().getCreatureStats(player);

        int greetingTimer = actorState.getGreetingTimer();
        GreetingState greetingState = actorState.getGreetingState();
        if (greetingState == Greet_None)
        {
            if ((playerPos - actorPos).length2() <= helloDistance * helloDistance && !playerStats.isDead()
                && !actorStats.isParalyzed() && !isTargetMagicallyHidden(player)
                && MWBase::Environment::get().getWorld()->getLOS(player, actor)
                && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, actor))
                greetingTimer++;

            if (greetingTimer >= GREETING_SHOULD_START)
            {
                greetingState = Greet_InProgress;
                if (!MWBase::Environment::get().getDialogueManager()->say(actor, ESM::RefId::stringRefId("hello")))
                    greetingState = Greet_Done;
                greetingTimer = 0;
            }
        }

        if (greetingState == Greet_InProgress)
        {
            greetingTimer++;

            if (!actorStats.getMovementFlag(CreatureStats::Flag_ForceJump)
                && !actorStats.getMovementFlag(CreatureStats::Flag_ForceSneak)
                && (greetingTimer <= GREETING_SHOULD_END
                    || MWBase::Environment::get().getSoundManager()->sayActive(actor)))
                turnActorToFacePlayer(actor, actorState, dir);

            if (greetingTimer >= GREETING_COOLDOWN)
            {
                greetingState = Greet_Done;
                greetingTimer = 0;
            }
        }

        if (greetingState == Greet_Done)
        {
            float resetDist = 2 * helloDistance;
            if ((playerPos - actorPos).length2() >= resetDist * resetDist)
                greetingState = Greet_None;
        }

        actorState.setGreetingTimer(greetingTimer);
        actorState.setGreetingState(greetingState);
    }

    void Actors::turnActorToFacePlayer(const MWWorld::Ptr& actor, Actor& actorState, const osg::Vec3f& dir) const
    {
        auto& movementSettings = actor.getClass().getMovementSettings(actor);
        movementSettings.mPosition[1] = 0;
        movementSettings.mPosition[0] = 0;

        if (!actorState.isTurningToPlayer())
        {
            float from = dir.x();
            float to = dir.y();
            float angle = std::atan2(from, to);
            actorState.setAngleToPlayer(angle);
            float deltaAngle = Misc::normalizeAngle(angle - actor.getRefData().getPosition().rot[2]);
            if (!Settings::game().mSmoothMovement || std::abs(deltaAngle) > osg::DegreesToRadians(60.f))
                actorState.setTurningToPlayer(true);
        }
    }

    void Actors::stopCombat(const MWWorld::Ptr& ptr) const
    {
        auto& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
        std::vector<MWWorld::Ptr> targets;
        if (ai.getCombatTargets(targets))
        {
            std::set<MWWorld::Ptr> allySet;
            getActorsSidingWith(ptr, allySet);
            std::vector<MWWorld::Ptr> allies(allySet.begin(), allySet.end());
            for (const auto& ally : allies)
                ally.getClass().getCreatureStats(ally).getAiSequence().stopCombat(targets);
            for (const auto& target : targets)
                target.getClass().getCreatureStats(target).getAiSequence().stopCombat(allies);
        }
    }

    void Actors::engageCombat(
        const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, SidingCache& cachedAllies, bool againstPlayer) const
    {
        CreatureStats& creatureStats1 = actor1.getClass().getCreatureStats(actor1);
        if (creatureStats1.isDead() || creatureStats1.getAiSequence().isInCombat(actor2))
            return;

        const CreatureStats& creatureStats2 = actor2.getClass().getCreatureStats(actor2);
        if (creatureStats2.isDead())
            return;

        const osg::Vec3f actor1Pos(actor1.getRefData().getPosition().asVec3());
        const osg::Vec3f actor2Pos(actor2.getRefData().getPosition().asVec3());
        const float sqrDist = (actor1Pos - actor2Pos).length2();
        const int actorsProcessingRange = Settings::game().mActorsProcessingRange;

        if (sqrDist > actorsProcessingRange * actorsProcessingRange)
            return;

        // If this is set to true, actor1 will start combat with actor2 if the awareness check at the end of the method
        // returns true
        bool aggressive = false;

        // Get actors allied with actor1. Includes those following or escorting actor1, actors following or escorting
        // those actors, (recursive) and any actor currently being followed or escorted by actor1
        const std::set<MWWorld::Ptr>& allies1 = cachedAllies.getActorsSidingWith(actor1);

        const auto mechanicsManager = MWBase::Environment::get().getMechanicsManager();
        // If an ally of actor1 has been attacked by actor2 or has attacked actor2, start combat between actor1 and
        // actor2
        for (const MWWorld::Ptr& ally : allies1)
        {
            if (creatureStats1.getAiSequence().isInCombat(ally))
                continue;

            if (creatureStats2.matchesActorId(ally.getClass().getCreatureStats(ally).getHitAttemptActorId()))
            {
                mechanicsManager->startCombat(actor1, actor2, &cachedAllies.getActorsSidingWith(actor2));
                // Also set the same hit attempt actor. Otherwise, if fighting the player, they may stop combat
                // if the player gets out of reach, while the ally would continue combat with the player
                creatureStats1.setHitAttemptActorId(ally.getClass().getCreatureStats(ally).getHitAttemptActorId());
                return;
            }

            // If there's been no attack attempt yet but an ally of actor1 is in combat with actor2, become aggressive
            // to actor2
            if (ally.getClass().getCreatureStats(ally).getAiSequence().isInCombat(actor2))
                aggressive = true;
        }

        MWWorld::Ptr player = MWMechanics::getPlayer();
        const std::set<MWWorld::Ptr>& playerAllies = cachedAllies.getActorsSidingWith(player);

        bool isPlayerFollowerOrEscorter = playerAllies.find(actor1) != playerAllies.end();

        // If actor2 and at least one actor2 are in combat with actor1, actor1 and its allies start combat with them
        // Doesn't apply for player followers/escorters
        if (!aggressive && !isPlayerFollowerOrEscorter)
        {
            // Check that actor2 is in combat with actor1
            if (creatureStats2.getAiSequence().isInCombat(actor1))
            {
                const std::set<MWWorld::Ptr>& allies2 = cachedAllies.getActorsSidingWith(actor2);
                // Check that an ally of actor2 is also in combat with actor1
                for (const MWWorld::Ptr& ally2 : allies2)
                {
                    if (ally2 != actor2 && ally2.getClass().getCreatureStats(ally2).getAiSequence().isInCombat(actor1))
                    {
                        mechanicsManager->startCombat(actor1, actor2, &allies2);
                        // Also have actor1's allies start combat
                        for (const MWWorld::Ptr& ally1 : allies1)
                            if (ally1 != player)
                                mechanicsManager->startCombat(ally1, actor2, &allies2);
                        return;
                    }
                }
            }
        }

        if (creatureStats2.getMagicEffects().getOrDefault(ESM::MagicEffect::Invisibility).getMagnitude() > 0)
            return;

        // Stop here if target is unreachable
        if (!canFight(actor1, actor2))
            return;

        // If set in the settings file, player followers and escorters will become aggressive toward enemies in combat
        // with them or the player
        if (!aggressive && isPlayerFollowerOrEscorter && Settings::game().mFollowersAttackOnSight)
        {
            if (creatureStats2.getAiSequence().isInCombat(actor1))
                aggressive = true;
            else
            {
                for (const MWWorld::Ptr& ally : allies1)
                {
                    if (ally != actor1 && creatureStats2.getAiSequence().isInCombat(ally))
                    {
                        aggressive = true;
                        break;
                    }
                }
            }
        }

        // Do aggression check if actor2 is the player or a player follower or escorter
        if (!aggressive)
        {
            if (againstPlayer || playerAllies.find(actor2) != playerAllies.end())
            {
                // Player followers and escorters with high fight should not initiate combat with the player or with
                // other player followers or escorters
                if (!isPlayerFollowerOrEscorter)
                    aggressive = mechanicsManager->isAggressive(actor1, actor2);
            }
        }

        // Make guards go aggressive with creatures and werewolves that are in combat
        const auto world = MWBase::Environment::get().getWorld();
        if (!aggressive && actor1.getClass().isClass(actor1, "Guard") && creatureStats2.getAiSequence().isInCombat())
        {
            // Check if the creature is too far
            static const float fAlarmRadius
                = world->getStore().get<ESM::GameSetting>().find("fAlarmRadius")->mValue.getFloat();
            if (sqrDist > fAlarmRadius * fAlarmRadius)
                return;

            bool targetIsCreature = !actor2.getClass().isNpc();
            if (targetIsCreature || actor2.getClass().getNpcStats(actor2).isWerewolf())
            {
                bool followerOrEscorter = false;
                // ...unless the creature has allies
                if (targetIsCreature)
                {
                    for (const auto& package : creatureStats2.getAiSequence())
                    {
                        // The follow package must be first or have nothing but combat before it
                        if (package->sideWithTarget())
                        {
                            followerOrEscorter = true;
                            break;
                        }
                        else if (package->getTypeId() != MWMechanics::AiPackageTypeId::Combat)
                            break;
                    }
                }
                // Morrowind also checks "known werewolf" flag, but the player is never in combat
                // so this code is unreachable for the player
                if (!followerOrEscorter)
                    aggressive = true;
            }
        }

        // If any of the above conditions turned actor1 aggressive towards actor2, do an awareness check. If it passes,
        // start combat with actor2.
        if (aggressive)
        {
            bool LOS = world->getLOS(actor1, actor2) && mechanicsManager->awarenessCheck(actor2, actor1);

            if (LOS)
                mechanicsManager->startCombat(actor1, actor2, &cachedAllies.getActorsSidingWith(actor2));
        }
    }

    void Actors::adjustMagicEffects(const MWWorld::Ptr& creature, float duration) const
    {
        CreatureStats& creatureStats = creature.getClass().getCreatureStats(creature);
        const bool wasDead = creatureStats.isDead();

        creatureStats.getActiveSpells().update(creature, duration);

        if (!wasDead && creatureStats.isDead())
        {
            // The actor was killed by a magic effect. Figure out if the player was responsible for it.
            const ActiveSpells& spells = creatureStats.getActiveSpells();
            const MWWorld::Ptr player = getPlayer();
            std::set<MWWorld::Ptr> playerFollowers;
            getActorsSidingWith(player, playerFollowers);

            for (const ActiveSpells::ActiveSpellParams& spell : spells)
            {
                bool actorKilled = false;

                MWWorld::Ptr caster
                    = MWBase::Environment::get().getWorld()->searchPtrViaActorId(spell.getCasterActorId());
                if (caster.isEmpty())
                    continue;
                for (const auto& effect : spell.getEffects())
                {
                    static const std::array<int, 7> damageEffects{
                        ESM::MagicEffect::FireDamage,
                        ESM::MagicEffect::ShockDamage,
                        ESM::MagicEffect::FrostDamage,
                        ESM::MagicEffect::Poison,
                        ESM::MagicEffect::SunDamage,
                        ESM::MagicEffect::DamageHealth,
                        ESM::MagicEffect::AbsorbHealth,
                    };

                    const bool isDamageEffect = std::find(damageEffects.begin(), damageEffects.end(), effect.mEffectId)
                        != damageEffects.end();

                    if (isDamageEffect)
                    {
                        if (caster.getClass().isNpc() && caster.getClass().getNpcStats(caster).isWerewolf())
                            caster.getClass().getNpcStats(caster).addWerewolfKill();
                        if (caster == player || playerFollowers.find(caster) != playerFollowers.end())
                        {
                            MWBase::Environment::get().getMechanicsManager()->actorKilled(creature, player);
                            actorKilled = true;
                            break;
                        }
                    }
                }

                if (actorKilled)
                    break;
            }
        }

        // updateSummons assumes the actor belongs to a cell.
        // This assumption isn't always valid for the player character.
        if (!creature.isInCell())
            return;

        if (!creatureStats.getSummonedCreatureMap().empty() || !creatureStats.getSummonedCreatureGraveyard().empty())
            updateSummons(creature, mTimerDisposeSummonsCorpses == 0.f);
    }

    void Actors::restoreDynamicStats(const MWWorld::Ptr& ptr, double hours, bool sleep) const
    {
        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        if (stats.isDead())
            return;

        const MWWorld::Store<ESM::GameSetting>& settings
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        if (sleep)
        {
            const auto [health, magicka] = getRestorationPerHourOfSleep(ptr);

            DynamicStat<float> stat = stats.getHealth();
            stat.setCurrent(stat.getCurrent() + health * hours);
            stats.setHealth(stat);

            double restoreHours = hours;
            const bool stunted
                = stats.getMagicEffects().getOrDefault(ESM::MagicEffect::StuntedMagicka).getMagnitude() > 0;
            if (stunted)
            {
                // Stunted Magicka effect should be taken into account.
                float remainingTime = getStuntedMagickaDuration(ptr);

                // Take a maximum remaining duration of Stunted Magicka effects (-1 is a constant one) in game hours.
                if (remainingTime > 0)
                {
                    double timeScale = MWBase::Environment::get().getWorld()->getTimeManager()->getGameTimeScale();
                    if (timeScale == 0.0)
                        timeScale = 1;

                    restoreHours = std::max(0.0, hours - remainingTime * timeScale / 3600.f);
                }
                else if (remainingTime == -1)
                    restoreHours = 0;
            }

            if (restoreHours > 0)
            {
                stat = stats.getMagicka();
                stat.setCurrent(stat.getCurrent() + magicka * restoreHours);
                stats.setMagicka(stat);
            }
        }

        // Current fatigue can be above base value due to a fortify effect.
        // In that case stop here and don't try to restore.
        DynamicStat<float> fatigue = stats.getFatigue();
        if (fatigue.getCurrent() >= fatigue.getBase())
            return;

        // Restore fatigue
        static const float fFatigueReturnBase = settings.find("fFatigueReturnBase")->mValue.getFloat();
        static const float fFatigueReturnMult = settings.find("fFatigueReturnMult")->mValue.getFloat();
        static const float fEndFatigueMult = settings.find("fEndFatigueMult")->mValue.getFloat();

        const float endurance = stats.getAttribute(ESM::Attribute::Endurance).getModified();

        float normalizedEncumbrance = ptr.getClass().getNormalizedEncumbrance(ptr);
        if (normalizedEncumbrance > 1)
            normalizedEncumbrance = 1;

        const float x
            = (fFatigueReturnBase + fFatigueReturnMult * (1 - normalizedEncumbrance)) * (fEndFatigueMult * endurance);

        fatigue.setCurrent(fatigue.getCurrent() + 3600 * x * hours);
        stats.setFatigue(fatigue);
    }

    void Actors::calculateRestoration(const MWWorld::Ptr& ptr, float duration) const
    {
        if (ptr.getClass().getCreatureStats(ptr).isDead())
            return;

        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

        // Current fatigue can be above base value due to a fortify effect.
        // In that case stop here and don't try to restore.
        DynamicStat<float> fatigue = stats.getFatigue();
        if (fatigue.getCurrent() >= fatigue.getBase())
            return;

        // Restore fatigue
        const float endurance = stats.getAttribute(ESM::Attribute::Endurance).getModified();
        const MWWorld::Store<ESM::GameSetting>& settings
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fFatigueReturnBase = settings.find("fFatigueReturnBase")->mValue.getFloat();
        static const float fFatigueReturnMult = settings.find("fFatigueReturnMult")->mValue.getFloat();

        const float x = fFatigueReturnBase + fFatigueReturnMult * endurance;

        fatigue.setCurrent(fatigue.getCurrent() + duration * x);
        stats.setFatigue(fatigue);
    }

    bool Actors::isAttackPreparing(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;
        return it->second->getCharacterController().isAttackPreparing();
    }

    bool Actors::isRunning(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;
        return it->second->getCharacterController().isRunning();
    }

    bool Actors::isSneaking(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;
        return it->second->getCharacterController().isSneaking();
    }

    static void updateDrowning(const MWWorld::Ptr& ptr, float duration, bool isKnockedOut, bool isPlayer)
    {
        const auto& actorClass = ptr.getClass();
        NpcStats& stats = actorClass.getNpcStats(ptr);

        // When npc stats are just initialized, mTimeToStartDrowning == -1 and we should get value from GMST
        static const float fHoldBreathTime = MWBase::Environment::get()
                                                 .getESMStore()
                                                 ->get<ESM::GameSetting>()
                                                 .find("fHoldBreathTime")
                                                 ->mValue.getFloat();
        if (stats.getTimeToStartDrowning() == -1.f)
            stats.setTimeToStartDrowning(fHoldBreathTime);

        if (!isPlayer && stats.getTimeToStartDrowning() < fHoldBreathTime / 2)
        {
            AiSequence& seq = actorClass.getCreatureStats(ptr).getAiSequence();
            if (seq.getTypeId() != AiPackageTypeId::Breathe) // Only add it once
                seq.stack(AiBreathe(), ptr);
        }

        const MWBase::World* const world = MWBase::Environment::get().getWorld();
        const bool knockedOutUnderwater
            = (isKnockedOut && world->isUnderwater(ptr.getCell(), osg::Vec3f(ptr.getRefData().getPosition().asVec3())));
        if ((world->isSubmerged(ptr) || knockedOutUnderwater)
            && stats.getMagicEffects().getOrDefault(ESM::MagicEffect::WaterBreathing).getMagnitude() == 0)
        {
            float timeLeft = 0.0f;
            if (knockedOutUnderwater)
                stats.setTimeToStartDrowning(0);
            else
            {
                timeLeft = stats.getTimeToStartDrowning() - duration;
                if (timeLeft < 0.0f)
                    timeLeft = 0.0f;
                stats.setTimeToStartDrowning(timeLeft);
            }

            const bool godmode = isPlayer && world->getGodModeState();

            if (timeLeft == 0.0f && !godmode)
            {
                // If drowning, apply 3 points of damage per second
                static const float fSuffocationDamage
                    = world->getStore().get<ESM::GameSetting>().find("fSuffocationDamage")->mValue.getFloat();
                DynamicStat<float> health = stats.getHealth();
                health.setCurrent(health.getCurrent() - fSuffocationDamage * duration);
                stats.setHealth(health);

                // Play a drowning sound
                MWBase::SoundManager* sndmgr = MWBase::Environment::get().getSoundManager();
                auto soundDrown = ESM::RefId::stringRefId("drown");
                if (!sndmgr->getSoundPlaying(ptr, soundDrown))
                    sndmgr->playSound3D(ptr, soundDrown, 1.0f, 1.0f);

                if (isPlayer)
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);
            }
        }
        else
            stats.setTimeToStartDrowning(fHoldBreathTime);
    }

    static void updateEquippedLight(const MWWorld::Ptr& ptr, float duration, bool mayEquip)
    {
        const bool isPlayer = (ptr == getPlayer());

        const auto& actorClass = ptr.getClass();
        auto& inventoryStore = actorClass.getInventoryStore(ptr);

        auto heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

        /**
         * Automatically equip NPCs torches at night and unequip them at day
         */
        if (!isPlayer)
        {
            auto torchIter = std::find_if(std::begin(inventoryStore), std::end(inventoryStore), [&](auto entry) {
                return entry.getType() == ESM::Light::sRecordId && entry.getClass().canBeEquipped(entry, ptr).first;
            });
            if (mayEquip)
            {
                if (torchIter != inventoryStore.end())
                {
                    if (!actorClass.getCreatureStats(ptr).getAiSequence().isInCombat())
                    {
                        // For non-hostile NPCs, unequip whatever is in the left slot in favor of a light.
                        if (heldIter != inventoryStore.end() && heldIter->getType() != ESM::Light::sRecordId)
                            inventoryStore.unequipItem(*heldIter);
                    }
                    else if (heldIter == inventoryStore.end() || heldIter->getType() == ESM::Light::sRecordId)
                    {
                        // For hostile NPCs, see if they have anything better to equip first
                        auto shield = inventoryStore.getPreferredShield();
                        if (shield != inventoryStore.end())
                            inventoryStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, shield);
                    }

                    heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

                    // If we have a torch and can equip it, then equip it now.
                    if (heldIter == inventoryStore.end())
                    {
                        inventoryStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, torchIter);
                    }
                }
            }
            else
            {
                if (heldIter != inventoryStore.end() && heldIter->getType() == ESM::Light::sRecordId)
                {
                    // At day, unequip lights and auto equip shields or other suitable items
                    // (Note: autoEquip will ignore lights)
                    inventoryStore.autoEquip();
                }
            }
        }

        heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

        // If holding a light...
        const auto world = MWBase::Environment::get().getWorld();
        MWRender::Animation* anim = world->getAnimation(ptr);
        if (heldIter.getType() == MWWorld::ContainerStore::Type_Light && anim && anim->getCarriedLeftShown())
        {
            // Use time from the player's light
            if (isPlayer)
            {
                float timeRemaining = heldIter->getClass().getRemainingUsageTime(*heldIter);

                // -1 is infinite light source. Other negative values are treated as 0.
                if (timeRemaining != -1.0f)
                {
                    timeRemaining -= duration;
                    if (timeRemaining <= 0.f)
                    {
                        inventoryStore.remove(*heldIter, 1); // remove it
                        return;
                    }

                    heldIter->getClass().setRemainingUsageTime(*heldIter, timeRemaining);
                }
            }

            // Both NPC and player lights extinguish in water.
            if (world->isSwimming(ptr))
            {
                inventoryStore.remove(*heldIter, 1); // remove it

                // ...But, only the player makes a sound.
                if (isPlayer)
                    MWBase::Environment::get().getSoundManager()->playSound(
                        ESM::RefId::stringRefId("torch out"), 1.0, 1.0, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
            }
        }
    }

    void Actors::updateCrimePursuit(const MWWorld::Ptr& ptr, float duration, SidingCache& cachedAllies) const
    {
        const MWWorld::Ptr player = getPlayer();
        if (ptr == player)
            return;

        const auto& actorClass = ptr.getClass();
        if (!actorClass.isNpc())
            return;

        // get stats of witness
        CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
        NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);

        const auto& playerClass = player.getClass();
        const auto& playerStats = playerClass.getNpcStats(player);
        if (playerStats.isWerewolf())
            return;

        const auto mechanicsManager = MWBase::Environment::get().getMechanicsManager();
        const auto world = MWBase::Environment::get().getWorld();

        if (actorClass.isClass(ptr, "Guard") && !creatureStats.getAiSequence().isInPursuit()
            && !creatureStats.getAiSequence().isInCombat()
            && creatureStats.getMagicEffects().getOrDefault(ESM::MagicEffect::CalmHumanoid).getMagnitude() == 0)
        {
            const MWWorld::ESMStore& esmStore = world->getStore();
            static const int cutoff = esmStore.get<ESM::GameSetting>().find("iCrimeThreshold")->mValue.getInteger();
            // Force dialogue on sight if bounty is greater than the cutoff
            // In vanilla morrowind, the greeting dialogue is scripted to either arrest the player (< 5000 bounty) or
            // attack (>= 5000 bounty)
            if (playerStats.getBounty() >= cutoff
                // TODO: do not run these two every frame. keep an Aware state for each actor and update it every 0.2 s
                // or so?
                && world->getLOS(ptr, player) && mechanicsManager->awarenessCheck(player, ptr))
            {
                static const int iCrimeThresholdMultiplier
                    = esmStore.get<ESM::GameSetting>().find("iCrimeThresholdMultiplier")->mValue.getInteger();
                if (playerStats.getBounty() >= cutoff * iCrimeThresholdMultiplier)
                {
                    mechanicsManager->startCombat(ptr, player, &cachedAllies.getActorsSidingWith(player));
                    creatureStats.setHitAttemptActorId(
                        playerClass.getCreatureStats(player)
                            .getActorId()); // Stops the guard from quitting combat if player is unreachable
                }
                else
                    creatureStats.getAiSequence().stack(AiPursue(player), ptr);
                creatureStats.setAlarmed(true);
                npcStats.setCrimeId(world->getPlayer().getNewCrimeId());
            }
        }

        // if I was a witness to a crime
        if (npcStats.getCrimeId() != -1)
        {
            // if you've paid for your crimes and I haven't noticed
            if (npcStats.getCrimeId() <= world->getPlayer().getCrimeId())
            {
                // Calm witness down
                if (ptr.getClass().isClass(ptr, "Guard"))
                    creatureStats.getAiSequence().stopPursuit();
                stopCombat(ptr);

                // Reset factors to attack
                creatureStats.setAttacked(false);
                creatureStats.setAlarmed(false);
                creatureStats.setAiSetting(AiSetting::Fight, ptr.getClass().getBaseFightRating(ptr));

                // Restore original disposition
                npcStats.setCrimeDispositionModifier(0);

                // Update witness crime id
                npcStats.setCrimeId(-1);
            }
        }
    }

    void Actors::addActor(const MWWorld::Ptr& ptr, bool updateImmediately)
    {
        removeActor(ptr, true);

        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (!anim)
            return;
        const auto it = mActors.emplace(mActors.end(), ptr, anim);
        mIndex.emplace(ptr.mRef, it);

        if (updateImmediately)
            it->getCharacterController().update(0);

        // We should initially hide actors outside of processing range.
        // Note: since we update player after other actors, distance will be incorrect during teleportation.
        // Do not update visibility if player was teleported, so actors will be visible during teleportation frame.
        if (MWBase::Environment::get().getWorld()->getPlayer().wasTeleported())
            return;

        updateVisibility(ptr, it->getCharacterController());
    }

    void Actors::updateVisibility(const MWWorld::Ptr& ptr, CharacterController& ctrl) const
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (ptr == player)
            return;

        const float dist
            = (player.getRefData().getPosition().asVec3() - ptr.getRefData().getPosition().asVec3()).length();
        const int actorsProcessingRange = Settings::game().mActorsProcessingRange;
        if (dist > actorsProcessingRange)
        {
            ptr.getRefData().getBaseNode()->setNodeMask(0);
            return;
        }
        else
            ptr.getRefData().getBaseNode()->setNodeMask(MWRender::Mask_Actor);

        // Fade away actors on large distance (>90% of actor's processing distance)
        float visibilityRatio = 1.0;
        const float fadeStartDistance = actorsProcessingRange * 0.9f;
        const float fadeEndDistance = actorsProcessingRange;
        const float fadeRatio = (dist - fadeStartDistance) / (fadeEndDistance - fadeStartDistance);
        if (fadeRatio > 0)
            visibilityRatio -= std::max(0.f, fadeRatio);

        visibilityRatio = std::min(1.f, visibilityRatio);

        ctrl.setVisibility(visibilityRatio);
    }

    void Actors::removeActor(const MWWorld::Ptr& ptr, bool keepActive)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
        {
            if (!keepActive)
                removeTemporaryEffects(iter->second->getPtr());
            mActors.erase(iter->second);
            mIndex.erase(iter);
        }
    }

    void Actors::castSpell(const MWWorld::Ptr& ptr, const ESM::RefId& spellId, bool scriptedSpell) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->getCharacterController().castSpell(spellId, scriptedSpell);
    }

    bool Actors::isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer) const
    {
        if (!actor.getClass().isActor())
            return false;

        // If an observer is NPC, check if he detected an actor
        if (!observer.isEmpty() && observer.getClass().isNpc())
        {
            return MWBase::Environment::get().getWorld()->getLOS(observer, actor)
                && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor, observer);
        }

        // Otherwise check if any actor in AI processing range sees the target actor
        std::vector<MWWorld::Ptr> neighbors;
        osg::Vec3f position(actor.getRefData().getPosition().asVec3());
        getObjectsInRange(position, Settings::game().mActorsProcessingRange, neighbors);
        for (const MWWorld::Ptr& neighbor : neighbors)
        {
            if (neighbor == actor)
                continue;

            const bool result = MWBase::Environment::get().getWorld()->getLOS(neighbor, actor)
                && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor, neighbor);

            if (result)
                return true;
        }

        return false;
    }

    void Actors::updateActor(const MWWorld::Ptr& old, const MWWorld::Ptr& ptr) const
    {
        const auto iter = mIndex.find(old.mRef);
        if (iter != mIndex.end())
            iter->second->updatePtr(ptr);
    }

    void Actors::dropActors(const MWWorld::CellStore* cellStore, const MWWorld::Ptr& ignore)
    {
        for (auto iter = mActors.begin(); iter != mActors.end();)
        {
            if ((iter->getPtr().isInCell() && iter->getPtr().getCell() == cellStore) && iter->getPtr() != ignore)
            {
                removeTemporaryEffects(iter->getPtr());
                mIndex.erase(iter->getPtr().mRef);
                iter = mActors.erase(iter);
            }
            else
                ++iter;
        }
    }

    void Actors::predictAndAvoidCollisions(float duration) const
    {
        if (!MWBase::Environment::get().getMechanicsManager()->isAIActive())
            return;

        const float minGap = 10.f;
        const float maxDistForPartialAvoiding = 200.f;
        const float maxDistForStrictAvoiding = 100.f;
        const float maxTimeToCheck = 2.0f;
        const bool giveWayWhenIdle = Settings::game().mNPCsGiveWay;

        const MWWorld::Ptr player = getPlayer();
        const MWBase::World* const world = MWBase::Environment::get().getWorld();
        for (const Actor& actor : mActors)
        {
            const MWWorld::Ptr& ptr = actor.getPtr();
            if (ptr == player)
                continue; // Don't interfere with player controls.

            const float maxSpeed = ptr.getClass().getMaxSpeed(ptr);
            if (maxSpeed == 0.0)
                continue; // Can't move, so there is no sense to predict collisions.

            Movement& movement = ptr.getClass().getMovementSettings(ptr);
            const osg::Vec2f origMovement(movement.mPosition[0], movement.mPosition[1]);
            const bool isMoving = origMovement.length2() > 0.01;
            if (movement.mPosition[1] < 0)
                continue; // Actors can not see others when move backward.

            // Moving NPCs always should avoid collisions.
            // Standing NPCs give way to moving ones if they are not in combat (or pursue) mode and either
            // follow player or have a AIWander package with non-empty wander area.
            bool shouldAvoidCollision = isMoving;
            bool shouldGiveWay = false;
            bool shouldTurnToApproachingActor = !isMoving;
            MWWorld::Ptr currentTarget; // Combat or pursue target (NPCs should not avoid collision with their targets).
            const auto& aiSequence = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            if (!aiSequence.isEmpty())
            {
                const auto& package = aiSequence.getActivePackage();
                if (package.getTypeId() == AiPackageTypeId::Follow)
                {
                    shouldAvoidCollision = true;
                }
                else if (package.getTypeId() == AiPackageTypeId::Wander && giveWayWhenIdle)
                {
                    if (!static_cast<const AiWander&>(package).isStationary())
                        shouldGiveWay = true;
                }
                else if (package.getTypeId() == AiPackageTypeId::Combat
                    || package.getTypeId() == AiPackageTypeId::Pursue)
                {
                    currentTarget = package.getTarget();
                    shouldAvoidCollision = isMoving;
                    shouldTurnToApproachingActor = false;
                }
            }

            if (!shouldAvoidCollision && !shouldGiveWay)
                continue;

            const osg::Vec2f baseSpeed = origMovement * maxSpeed;
            const osg::Vec3f basePos = ptr.getRefData().getPosition().asVec3();
            const float baseRotZ = ptr.getRefData().getPosition().rot[2];
            const osg::Vec3f halfExtents = world->getHalfExtents(ptr);
            const float maxDistToCheck = isMoving ? maxDistForPartialAvoiding : maxDistForStrictAvoiding;

            float timeToCheck = maxTimeToCheck;
            if (!shouldGiveWay && !aiSequence.isEmpty())
                timeToCheck = std::min(
                    timeToCheck, getTimeToDestination(**aiSequence.begin(), basePos, maxSpeed, duration, halfExtents));

            float timeToCollision = timeToCheck;
            osg::Vec2f movementCorrection(0, 0);
            float angleToApproachingActor = 0;

            // Iterate through all other actors and predict collisions.
            for (const Actor& otherActor : mActors)
            {
                const MWWorld::Ptr& otherPtr = otherActor.getPtr();
                if (otherPtr == ptr || otherPtr == currentTarget)
                    continue;

                const osg::Vec3f otherHalfExtents = world->getHalfExtents(otherPtr);
                const osg::Vec3f deltaPos = otherPtr.getRefData().getPosition().asVec3() - basePos;
                const osg::Vec2f relPos = Misc::rotateVec2f(osg::Vec2f(deltaPos.x(), deltaPos.y()), baseRotZ);
                const float dist = deltaPos.length();

                // Ignore actors which are not close enough or come from behind.
                if (dist > maxDistToCheck || relPos.y() < 0)
                    continue;

                // Don't check for a collision if vertical distance is greater then the actor's height.
                if (deltaPos.z() > halfExtents.z() * 2 || deltaPos.z() < -otherHalfExtents.z() * 2)
                    continue;

                const osg::Vec3f speed = otherPtr.getClass().getMovementSettings(otherPtr).asVec3()
                    * otherPtr.getClass().getMaxSpeed(otherPtr);
                const float rotZ = otherPtr.getRefData().getPosition().rot[2];
                const osg::Vec2f relSpeed
                    = Misc::rotateVec2f(osg::Vec2f(speed.x(), speed.y()), baseRotZ - rotZ) - baseSpeed;

                float collisionDist = minGap + halfExtents.x() + otherHalfExtents.x();
                collisionDist = std::min(collisionDist, relPos.length());

                // Find the earliest `t` when |relPos + relSpeed * t| == collisionDist.
                const float vr = relPos.x() * relSpeed.x() + relPos.y() * relSpeed.y();
                const float v2 = relSpeed.length2();
                const float Dh = vr * vr - v2 * (relPos.length2() - collisionDist * collisionDist);
                if (Dh <= 0 || v2 == 0)
                    continue; // No solution; distance is always >= collisionDist.
                const float t = (-vr - std::sqrt(Dh)) / v2;

                if (t < 0 || t > timeToCollision)
                    continue;

                // Check visibility and awareness last as it's expensive.
                if (!MWBase::Environment::get().getWorld()->getLOS(otherPtr, ptr))
                    continue;
                if (!MWBase::Environment::get().getMechanicsManager()->awarenessCheck(otherPtr, ptr))
                    continue;

                timeToCollision = t;
                angleToApproachingActor = std::atan2(deltaPos.x(), deltaPos.y());
                const osg::Vec2f posAtT = relPos + relSpeed * t;
                const float coef = (posAtT.x() * relSpeed.x() + posAtT.y() * relSpeed.y())
                    / (collisionDist * collisionDist * maxSpeed)
                    * std::clamp(
                        (maxDistForPartialAvoiding - dist) / (maxDistForPartialAvoiding - maxDistForStrictAvoiding),
                        0.f, 1.f);
                movementCorrection = posAtT * coef;
                if (otherPtr.getClass().getCreatureStats(otherPtr).isDead())
                    // In case of dead body still try to go around (it looks natural), but reduce the correction twice.
                    movementCorrection.y() *= 0.5f;
            }

            if (timeToCollision < timeToCheck)
            {
                // Try to evade the nearest collision.
                osg::Vec2f newMovement = origMovement + movementCorrection;
                // Step to the side rather than backward. Otherwise player will be able to push the NPC far away from
                // it's original location.
                newMovement.y() = std::max(newMovement.y(), 0.f);
                newMovement.normalize();
                if (isMoving)
                    newMovement *= origMovement.length(); // Keep the original speed.
                movement.mPosition[0] = newMovement.x();
                movement.mPosition[1] = newMovement.y();
                if (shouldTurnToApproachingActor)
                    zTurn(ptr, angleToApproachingActor);
            }
        }
    }

    void Actors::update(float duration, bool paused)
    {
        if (!paused)
        {
            const float updateEquippedLightInterval = 1.0f;

            if (mTimerUpdateHeadTrack >= 0.3f)
                mTimerUpdateHeadTrack = 0;

            if (mTimerUpdateHello >= 0.25f)
                mTimerUpdateHello = 0;

            if (mTimerDisposeSummonsCorpses >= 0.2f)
                mTimerDisposeSummonsCorpses = 0;

            if (mTimerUpdateEquippedLight >= updateEquippedLightInterval)
                mTimerUpdateEquippedLight = 0;

            // show torches only when there are darkness and no precipitations
            MWBase::World* const world = MWBase::Environment::get().getWorld();
            const bool showTorches = world->useTorches();

            const MWWorld::Ptr player = getPlayer();
            const osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();

            /// \todo move update logic to Actor class where appropriate

            SidingCache cachedAllies{ *this, true }; // will be filled as engageCombat iterates

            const bool aiActive = MWBase::Environment::get().getMechanicsManager()->isAIActive();
            const int attackedByPlayerId = player.getClass().getCreatureStats(player).getHitAttemptActorId();
            if (attackedByPlayerId != -1)
            {
                const MWWorld::Ptr playerHitAttemptActor = world->searchPtrViaActorId(attackedByPlayerId);

                if (!playerHitAttemptActor.isInCell())
                    player.getClass().getCreatureStats(player).setHitAttemptActorId(-1);
            }
            const int actorsProcessingRange = Settings::game().mActorsProcessingRange;

            // AI and magic effects update
            for (Actor& actor : mActors)
            {
                const bool isPlayer = actor.getPtr() == player;
                CharacterController& ctrl = actor.getCharacterController();
                MWBase::LuaManager::ActorControls* luaControls
                    = MWBase::Environment::get().getLuaManager()->getActorControls(actor.getPtr());

                const float distSqr = (playerPos - actor.getPtr().getRefData().getPosition().asVec3()).length2();
                // AI processing is only done within given distance to the player.
                const bool inProcessingRange = distSqr <= actorsProcessingRange * actorsProcessingRange;

                // If dead or no longer in combat, no longer store any actors who attempted to hit us. Also remove for
                // the player.
                if (!isPlayer
                    && (actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isDead()
                        || !actor.getPtr().getClass().getCreatureStats(actor.getPtr()).getAiSequence().isInCombat()
                        || !inProcessingRange))
                {
                    actor.getPtr().getClass().getCreatureStats(actor.getPtr()).setHitAttemptActorId(-1);
                    if (player.getClass().getCreatureStats(player).getHitAttemptActorId()
                        == actor.getPtr().getClass().getCreatureStats(actor.getPtr()).getActorId())
                        player.getClass().getCreatureStats(player).setHitAttemptActorId(-1);
                }

                const Misc::TimerStatus engageCombatTimerStatus = actor.updateEngageCombatTimer(duration);

                // For dead actors we need to update looping spell particles
                if (actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isDead())
                {
                    // They can be added during the death animation
                    if (!actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isDeathAnimationFinished())
                        adjustMagicEffects(actor.getPtr(), duration);
                    ctrl.updateContinuousVfx();
                }
                else
                {
                    MWWorld::Scene* worldScene = MWBase::Environment::get().getWorldScene();
                    const bool cellChanged = worldScene->hasCellChanged();
                    const MWWorld::Ptr actorPtr = actor.getPtr(); // make a copy of the map key to avoid it being
                                                                  // invalidated when the player teleports
                    updateActor(actorPtr, duration);

                    // Looping magic VFX update
                    // Note: we need to do this before any of the animations are updated.
                    // Reaching the text keys may trigger Hit / Spellcast (and as such, particles),
                    // so updating VFX immediately after that would just remove the particle effects instantly.
                    // There needs to be a magic effect update in between.
                    ctrl.updateContinuousVfx();

                    if (!cellChanged && worldScene->hasCellChanged())
                    {
                        return; // for now abort update of the old cell when cell changes by teleportation magic effect
                                // a better solution might be to apply cell changes at the end of the frame
                    }
                    if (aiActive && inProcessingRange)
                    {
                        if (engageCombatTimerStatus == Misc::TimerStatus::Elapsed)
                        {
                            if (!isPlayer)
                                adjustCommandedActor(actor.getPtr());

                            for (const Actor& otherActor : mActors)
                            {
                                if (otherActor.getPtr() == actor.getPtr() || isPlayer) // player is not AI-controlled
                                    continue;
                                engageCombat(
                                    actor.getPtr(), otherActor.getPtr(), cachedAllies, otherActor.getPtr() == player);
                            }
                        }
                        if (mTimerUpdateHeadTrack == 0)
                            updateHeadTracking(actor.getPtr(), mActors, isPlayer, ctrl);

                        if (actor.getPtr().getClass().isNpc() && !isPlayer)
                            updateCrimePursuit(actor.getPtr(), duration, cachedAllies);

                        if (!isPlayer)
                        {
                            CreatureStats& stats = actor.getPtr().getClass().getCreatureStats(actor.getPtr());
                            if (isConscious(actor.getPtr()) && !(luaControls && luaControls->mDisableAI))
                            {
                                stats.getAiSequence().execute(actor.getPtr(), ctrl, duration);
                                updateGreetingState(actor.getPtr(), actor, mTimerUpdateHello > 0);
                                playIdleDialogue(actor.getPtr());
                                updateMovementSpeed(actor.getPtr());
                            }
                        }
                    }
                    else if (aiActive && !isPlayer && isConscious(actor.getPtr())
                        && !(luaControls && luaControls->mDisableAI))
                    {
                        CreatureStats& stats = actor.getPtr().getClass().getCreatureStats(actor.getPtr());
                        stats.getAiSequence().execute(actor.getPtr(), ctrl, duration, /*outOfRange*/ true);
                    }

                    if (inProcessingRange && actor.getPtr().getClass().isNpc())
                    {
                        // We can not update drowning state for actors outside of AI distance - they can not resurface
                        // to breathe
                        updateDrowning(actor.getPtr(), duration, ctrl.isKnockedOut(), isPlayer);
                    }
                    if (mTimerUpdateEquippedLight == 0 && actor.getPtr().getClass().hasInventoryStore(actor.getPtr()))
                        updateEquippedLight(actor.getPtr(), updateEquippedLightInterval, showTorches);

                    if (luaControls != nullptr && isConscious(actor.getPtr()))
                        updateLuaControls(actor.getPtr(), isPlayer, *luaControls);
                }
            }

            if (Settings::game().mNPCsAvoidCollisions)
                predictAndAvoidCollisions(duration);

            mTimerUpdateHeadTrack += duration;
            mTimerUpdateEquippedLight += duration;
            mTimerUpdateHello += duration;
            mTimerDisposeSummonsCorpses += duration;

            // Animation/movement update
            CharacterController* playerCharacter = nullptr;
            for (Actor& actor : mActors)
            {
                const float dist = (playerPos - actor.getPtr().getRefData().getPosition().asVec3()).length();
                const bool isPlayer = actor.getPtr() == player;
                CreatureStats& stats = actor.getPtr().getClass().getCreatureStats(actor.getPtr());
                // Actors with active AI should be able to move.
                bool alwaysActive = false;
                if (!isPlayer && isConscious(actor.getPtr()) && !stats.isParalyzed())
                {
                    MWMechanics::AiSequence& seq = stats.getAiSequence();
                    alwaysActive = !seq.isEmpty() && seq.getActivePackage().alwaysActive();
                }
                const bool inRange = isPlayer || dist <= actorsProcessingRange || alwaysActive;
                const int activeFlag = isPlayer ? 2 : 1; // Can be changed back to '2' to keep updating bounding boxes
                                                         // off screen (more accurate, but slower)
                const int active = inRange ? activeFlag : 0;

                CharacterController& ctrl = actor.getCharacterController();
                ctrl.setActive(active);

                if (!inRange)
                {
                    actor.getPtr().getRefData().getBaseNode()->setNodeMask(0);
                    world->setActorActive(actor.getPtr(), false);
                    continue;
                }

                world->setActorActive(actor.getPtr(), true);

                const bool isDead = actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isDead();
                if (!isDead && actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isParalyzed())
                    ctrl.skipAnim();

                // Handle player last, in case a cell transition occurs by casting a teleportation spell
                // (would invalidate the iterator)
                if (isPlayer)
                {
                    playerCharacter = &ctrl;
                    continue;
                }

                actor.getPtr().getRefData().getBaseNode()->setNodeMask(MWRender::Mask_Actor);
                world->setActorCollisionMode(actor.getPtr(), true,
                    !actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isDeathAnimationFinished());

                if (!actor.getPositionAdjusted())
                {
                    actor.getPtr().getClass().adjustPosition(actor.getPtr(), false);
                    actor.setPositionAdjusted(true);
                }

                ctrl.update(duration);

                updateVisibility(actor.getPtr(), ctrl);
            }

            if (playerCharacter)
            {
                MWBase::Environment::get().getWorld()->applyDeferredPreviewRotationToPlayer(duration);
                playerCharacter->update(duration);
                playerCharacter->setVisibility(1.f);
                MWBase::LuaManager::ActorControls* luaControls
                    = MWBase::Environment::get().getLuaManager()->getActorControls(player);
                if (luaControls && player.getClass().getMovementSettings(player).mPosition[2] < 1)
                    luaControls->mJump = false;
            }

            for (const Actor& actor : mActors)
            {
                const MWWorld::Class& cls = actor.getPtr().getClass();
                CreatureStats& stats = cls.getCreatureStats(actor.getPtr());

                // KnockedOutOneFrameLogic
                // Used for "OnKnockedOut" command
                // Put here to ensure that it's run for PRECISELY one frame.
                if (stats.getKnockedDown() && !stats.getKnockedDownOneFrame() && !stats.getKnockedDownOverOneFrame())
                { // Start it for one frame if necessary
                    stats.setKnockedDownOneFrame(true);
                }
                else if (stats.getKnockedDownOneFrame() && !stats.getKnockedDownOverOneFrame())
                { // Turn off KnockedOutOneframe
                    stats.setKnockedDownOneFrame(false);
                    stats.setKnockedDownOverOneFrame(true);
                }
            }

            killDeadActors();
            updateSneaking(playerCharacter, duration);
        }
    }

    void Actors::notifyDied(const MWWorld::Ptr& actor)
    {
        actor.getClass().getCreatureStats(actor).notifyDied();

        ++mDeathCount[actor.getCellRef().getRefId()];

        MWBase::Environment::get().getLuaManager()->actorDied(actor);
    }

    void Actors::resurrect(const MWWorld::Ptr& ptr) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
        {
            if (iter->second->getCharacterController().isDead())
            {
                // Actor has been resurrected. Notify the CharacterController and re-enable collision.
                MWBase::Environment::get().getWorld()->enableActorCollision(iter->second->getPtr(), true);
                iter->second->getCharacterController().resurrect();
            }
        }
    }

    void Actors::killDeadActors()
    {
        for (Actor& actor : mActors)
        {
            const MWWorld::Class& cls = actor.getPtr().getClass();
            CreatureStats& stats = cls.getCreatureStats(actor.getPtr());

            if (!stats.isDead())
                continue;

            MWBase::Environment::get().getWorld()->removeActorPath(actor.getPtr());
            CharacterController::KillResult killResult = actor.getCharacterController().kill();
            if (killResult == CharacterController::Result_DeathAnimStarted)
            {
                // Play dying words
                // Note: It's not known whether the soundgen tags scream, roar, and moan are reliable
                // for NPCs since some of the npc death animation files are missing them.
                MWBase::Environment::get().getDialogueManager()->say(actor.getPtr(), ESM::RefId::stringRefId("hit"));

                // Apply soultrap
                if (actor.getPtr().getType() == ESM::Creature::sRecordId)
                    soulTrap(actor.getPtr());

                if (cls.isEssential(actor.getPtr()))
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sKilledEssential}");
            }
            else if (killResult == CharacterController::Result_DeathAnimJustFinished)
            {
                const bool isPlayer = actor.getPtr() == getPlayer();
                notifyDied(actor.getPtr());

                // Reset magic effects and recalculate derived effects
                // One case where we need this is to make sure bound items are removed upon death
                const float vampirism
                    = stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Vampirism).getMagnitude();
                stats.getActiveSpells().clear(actor.getPtr());
                // Make sure spell effects are removed
                purgeSpellEffects(stats.getActorId());

                stats.getMagicEffects().add(ESM::MagicEffect::Vampirism, vampirism);

                if (isPlayer)
                {
                    // player's death animation is over
                    MWBase::Environment::get().getStateManager()->askLoadRecent();
                }
                else
                {
                    // NPC death animation is over, disable actor collision
                    MWBase::Environment::get().getWorld()->enableActorCollision(actor.getPtr(), false);
                }
            }
        }
    }

    void Actors::cleanupSummonedCreature(MWMechanics::CreatureStats& casterStats, int creatureActorId) const
    {
        const MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(creatureActorId);
        if (!ptr.isEmpty())
        {
            MWBase::Environment::get().getWorld()->deleteObject(ptr);

            const ESM::Static* fx = MWBase::Environment::get().getESMStore()->get<ESM::Static>().search(
                ESM::RefId::stringRefId("VFX_Summon_End"));
            if (fx)
                MWBase::Environment::get().getWorld()->spawnEffect(
                    Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(fx->mModel)), "",
                    ptr.getRefData().getPosition().asVec3());

            // Remove the summoned creature's summoned creatures as well
            MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
            auto& creatureMap = stats.getSummonedCreatureMap();
            for (const auto& creature : creatureMap)
                cleanupSummonedCreature(stats, creature.second);
            creatureMap.clear();
        }
        else if (creatureActorId != -1)
        {
            // We didn't find the creature. It's probably in an inactive cell.
            // Add to graveyard so we can delete it when the cell becomes active.
            std::vector<int>& graveyard = casterStats.getSummonedCreatureGraveyard();
            graveyard.push_back(creatureActorId);
        }

        purgeSpellEffects(creatureActorId);
    }

    void Actors::purgeSpellEffects(int casterActorId) const
    {
        for (const Actor& actor : mActors)
        {
            MWMechanics::ActiveSpells& spells
                = actor.getPtr().getClass().getCreatureStats(actor.getPtr()).getActiveSpells();
            spells.purge(actor.getPtr(), casterActorId);
        }
    }

    void Actors::rest(double hours, bool sleep) const
    {
        float duration = hours * 3600.f;
        const float timeScale = MWBase::Environment::get().getWorld()->getTimeManager()->getGameTimeScale();
        if (timeScale != 0.f)
            duration /= timeScale;

        const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();
        const int actorsProcessingRange = Settings::game().mActorsProcessingRange;

        for (const Actor& actor : mActors)
        {
            if (actor.getPtr().getClass().getCreatureStats(actor.getPtr()).isDead())
            {
                adjustMagicEffects(actor.getPtr(), duration);
                continue;
            }

            if (!sleep || actor.getPtr() == player)
                restoreDynamicStats(actor.getPtr(), hours, sleep);

            if ((!actor.getPtr().getRefData().getBaseNode())
                || (playerPos - actor.getPtr().getRefData().getPosition().asVec3()).length2()
                    > actorsProcessingRange * actorsProcessingRange)
                continue;

            // Get rid of effects pending removal so they are not applied when resting
            updateMagicEffects(actor.getPtr());

            adjustMagicEffects(actor.getPtr(), duration);

            MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(actor.getPtr());
            if (animation)
            {
                animation->removeEffects();
                MWBase::Environment::get().getWorld()->applyLoopingParticles(actor.getPtr());
            }
        }

        fastForwardAi();
    }

    void Actors::updateSneaking(CharacterController* ctrl, float duration)
    {
        if (!ctrl)
        {
            MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
            return;
        }

        const MWWorld::Ptr player = getPlayer();

        if (!MWBase::Environment::get().getMechanicsManager()->isSneaking(player))
        {
            MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
            return;
        }

        MWBase::World* const world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();
        static const float fSneakUseDist = gmst.find("fSneakUseDist")->mValue.getFloat();
        static const float fSneakUseDelay = gmst.find("fSneakUseDelay")->mValue.getFloat();

        if (mSneakTimer >= fSneakUseDelay)
            mSneakTimer = 0.f;

        if (mSneakTimer == 0.f)
        {
            // Set when an NPC is within line of sight and distance, but is still unaware. Used for skill progress.
            bool avoidedNotice = false;
            bool detected = false;

            std::vector<MWWorld::Ptr> observers;
            const osg::Vec3f position(player.getRefData().getPosition().asVec3());
            const float radius = std::min<float>(fSneakUseDist, Settings::game().mActorsProcessingRange);
            getObjectsInRange(position, radius, observers);

            std::set<MWWorld::Ptr> sidingActors;
            getActorsSidingWith(player, sidingActors);

            for (const MWWorld::Ptr& observer : observers)
            {
                if (observer == player || observer.getClass().getCreatureStats(observer).isDead())
                    continue;

                if (sidingActors.find(observer) != sidingActors.cend())
                    continue;

                if (world->getLOS(player, observer))
                {
                    if (MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, observer))
                    {
                        detected = true;
                        avoidedNotice = false;
                        MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
                        break;
                    }
                    else
                    {
                        avoidedNotice = true;
                    }
                }
            }

            if (mSneakSkillTimer >= fSneakUseDelay)
                mSneakSkillTimer = 0.f;

            if (avoidedNotice && mSneakSkillTimer == 0.f)
                player.getClass().skillUsageSucceeded(player, ESM::Skill::Sneak, ESM::Skill::Sneak_AvoidNotice);

            if (!detected)
                MWBase::Environment::get().getWindowManager()->setSneakVisibility(true);
        }

        mSneakTimer += duration;
        mSneakSkillTimer += duration;
    }

    int Actors::getHoursToRest(const MWWorld::Ptr& ptr) const
    {
        const auto [healthPerHour, magickaPerHour] = getRestorationPerHourOfSleep(ptr);

        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        const bool stunted = stats.getMagicEffects().getOrDefault(ESM::MagicEffect::StuntedMagicka).getMagnitude() > 0;

        const float healthHours = healthPerHour > 0
            ? (stats.getHealth().getModified() - stats.getHealth().getCurrent()) / healthPerHour
            : 1.0f;
        const float magickaHours = magickaPerHour > 0 && !stunted
            ? (stats.getMagicka().getModified() - stats.getMagicka().getCurrent()) / magickaPerHour
            : 1.0f;

        return static_cast<int>(std::ceil(std::max(1.f, std::max(healthHours, magickaHours))));
    }

    int Actors::countDeaths(const ESM::RefId& id) const
    {
        const auto iter = mDeathCount.find(id);
        if (iter != mDeathCount.end())
            return iter->second;
        return 0;
    }

    void Actors::forceStateUpdate(const MWWorld::Ptr& ptr) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->getCharacterController().forceStateUpdate();
    }

    bool Actors::playAnimationGroup(
        const MWWorld::Ptr& ptr, std::string_view groupName, int mode, uint32_t number, bool scripted) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
        {
            return iter->second->getCharacterController().playGroup(groupName, mode, number, scripted);
        }
        else
        {
            Log(Debug::Warning) << "Warning: Actors::playAnimationGroup: Unable to find "
                                << ptr.getCellRef().getRefId();
            return false;
        }
    }

    bool Actors::playAnimationGroupLua(const MWWorld::Ptr& ptr, std::string_view groupName, uint32_t loops, float speed,
        std::string_view startKey, std::string_view stopKey, bool forceLoop)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            return iter->second->getCharacterController().playGroupLua(
                groupName, speed, startKey, stopKey, loops, forceLoop);
        return false;
    }

    void Actors::enableLuaAnimations(const MWWorld::Ptr& ptr, bool enable)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->getCharacterController().enableLuaAnimations(enable);
    }

    void Actors::skipAnimation(const MWWorld::Ptr& ptr) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->getCharacterController().skipAnim();
    }

    bool Actors::checkAnimationPlaying(const MWWorld::Ptr& ptr, std::string_view groupName) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            return iter->second->getCharacterController().isAnimPlaying(groupName);
        return false;
    }

    bool Actors::checkScriptedAnimationPlaying(const MWWorld::Ptr& ptr) const
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            return iter->second->getCharacterController().isScriptedAnimPlaying();
        return false;
    }

    void Actors::persistAnimationStates() const
    {
        for (const Actor& actor : mActors)
            actor.getCharacterController().persistAnimationState();
    }

    void Actors::clearAnimationQueue(const MWWorld::Ptr& ptr, bool clearScripted)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->getCharacterController().clearAnimQueue(clearScripted);
    }

    void Actors::getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out) const
    {
        for (const Actor& actor : mActors)
        {
            if ((actor.getPtr().getRefData().getPosition().asVec3() - position).length2() <= radius * radius)
                out.push_back(actor.getPtr());
        }
    }

    bool Actors::isAnyObjectInRange(const osg::Vec3f& position, float radius) const
    {
        for (const Actor& actor : mActors)
        {
            if ((actor.getPtr().getRefData().getPosition().asVec3() - position).length2() <= radius * radius)
                return true;
        }

        return false;
    }

    std::vector<MWWorld::Ptr> Actors::getActorsSidingWith(const MWWorld::Ptr& actorPtr, bool excludeInfighting) const
    {
        std::vector<MWWorld::Ptr> list;
        list.push_back(actorPtr);
        for (const Actor& actor : mActors)
        {
            const MWWorld::Ptr& iteratedActor = actor.getPtr();
            if (iteratedActor == getPlayer())
                continue;

            const bool sameActor = (iteratedActor == actorPtr);

            const CreatureStats& stats = iteratedActor.getClass().getCreatureStats(iteratedActor);
            if (stats.isDead())
                continue;

            // An actor counts as siding with this actor if Follow or Escort is the current AI package, or there are
            // only Wander packages before the Follow/Escort package Actors that are targeted by this actor's Follow or
            // Escort packages also side with them
            for (const auto& package : stats.getAiSequence())
            {
                if (excludeInfighting && !sameActor && package->getTypeId() == AiPackageTypeId::Combat
                    && package->targetIs(actorPtr))
                    break;
                if (package->sideWithTarget() && !package->getTarget().isEmpty())
                {
                    if (sameActor)
                    {
                        if (excludeInfighting)
                        {
                            MWWorld::Ptr ally = package->getTarget();
                            std::vector<MWWorld::Ptr> enemies;
                            if (ally.getClass().getCreatureStats(ally).getAiSequence().getCombatTargets(enemies)
                                && std::find(enemies.begin(), enemies.end(), actorPtr) != enemies.end())
                                break;
                            enemies.clear();
                            if (actorPtr.getClass().getCreatureStats(actorPtr).getAiSequence().getCombatTargets(enemies)
                                && std::find(enemies.begin(), enemies.end(), ally) != enemies.end())
                                break;
                        }
                        list.push_back(package->getTarget());
                    }
                    else if (package->targetIs(actorPtr))
                    {
                        list.push_back(iteratedActor);
                    }
                    break;
                }
                else if (package->getTypeId() > AiPackageTypeId::Wander
                    && package->getTypeId() <= AiPackageTypeId::Activate) // Don't count "fake" package types
                    break;
            }
        }
        return list;
    }

    std::vector<MWWorld::Ptr> Actors::getActorsFollowing(const MWWorld::Ptr& actorPtr) const
    {
        std::vector<MWWorld::Ptr> list;
        forEachFollowingPackage(
            mActors, actorPtr, getPlayer(), [&](const Actor& actor, const std::shared_ptr<AiPackage>& package) {
                if (package->followTargetThroughDoors() && package->targetIs(actorPtr))
                    list.push_back(actor.getPtr());
                else if (package->getTypeId() != AiPackageTypeId::Combat
                    && package->getTypeId() != AiPackageTypeId::Wander)
                    return false;
                return true;
            });
        return list;
    }

    void Actors::getActorsFollowing(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out) const
    {
        auto followers = getActorsFollowing(actor);
        for (const MWWorld::Ptr& follower : followers)
            if (out.insert(follower).second)
                getActorsFollowing(follower, out);
    }

    void Actors::getActorsSidingWith(
        const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out, bool excludeInfighting) const
    {
        auto followers = getActorsSidingWith(actor, excludeInfighting);
        for (const MWWorld::Ptr& follower : followers)
            if (out.insert(follower).second && follower != actor)
                getActorsSidingWith(follower, out, excludeInfighting);
    }

    std::vector<int> Actors::getActorsFollowingIndices(const MWWorld::Ptr& actor) const
    {
        std::vector<int> list;
        forEachFollowingPackage(
            mActors, actor, getPlayer(), [&](const Actor&, const std::shared_ptr<AiPackage>& package) {
                if (package->followTargetThroughDoors() && package->targetIs(actor))
                {
                    list.push_back(static_cast<const AiFollow*>(package.get())->getFollowIndex());
                    return false;
                }
                else if (package->getTypeId() != AiPackageTypeId::Combat
                    && package->getTypeId() != AiPackageTypeId::Wander)
                    return false;
                return true;
            });
        return list;
    }

    std::map<int, MWWorld::Ptr> Actors::getActorsFollowingByIndex(const MWWorld::Ptr& actor) const
    {
        std::map<int, MWWorld::Ptr> map;
        forEachFollowingPackage(
            mActors, actor, getPlayer(), [&](const Actor& otherActor, const std::shared_ptr<AiPackage>& package) {
                if (package->followTargetThroughDoors() && package->targetIs(actor))
                {
                    const int index = static_cast<const AiFollow*>(package.get())->getFollowIndex();
                    map[index] = otherActor.getPtr();
                    return false;
                }
                else if (package->getTypeId() != AiPackageTypeId::Combat
                    && package->getTypeId() != AiPackageTypeId::Wander)
                    return false;
                return true;
            });
        return map;
    }

    std::vector<MWWorld::Ptr> Actors::getActorsFighting(const MWWorld::Ptr& actor) const
    {
        std::vector<MWWorld::Ptr> list;
        std::vector<MWWorld::Ptr> neighbors;
        const osg::Vec3f position(actor.getRefData().getPosition().asVec3());
        getObjectsInRange(position, Settings::game().mActorsProcessingRange, neighbors);
        for (const MWWorld::Ptr& neighbor : neighbors)
        {
            if (neighbor == actor)
                continue;

            const CreatureStats& stats = neighbor.getClass().getCreatureStats(neighbor);
            if (stats.isDead())
                continue;

            if (stats.getAiSequence().isInCombat(actor))
                list.push_back(neighbor);
        }
        return list;
    }

    std::vector<MWWorld::Ptr> Actors::getEnemiesNearby(const MWWorld::Ptr& actor) const
    {
        std::vector<MWWorld::Ptr> list;
        std::vector<MWWorld::Ptr> neighbors;
        osg::Vec3f position(actor.getRefData().getPosition().asVec3());
        getObjectsInRange(position, Settings::game().mActorsProcessingRange, neighbors);

        std::set<MWWorld::Ptr> followers;
        getActorsFollowing(actor, followers);
        for (const MWWorld::Ptr& neighbor : neighbors)
        {
            const CreatureStats& stats = neighbor.getClass().getCreatureStats(neighbor);
            if (stats.isDead() || neighbor == actor || neighbor.getClass().isPureWaterCreature(neighbor))
                continue;

            const bool isFollower = followers.find(neighbor) != followers.end();

            if (stats.getAiSequence().isInCombat(actor)
                || (MWBase::Environment::get().getMechanicsManager()->isAggressive(neighbor, actor) && !isFollower))
                list.push_back(neighbor);
        }
        return list;
    }

    void Actors::write(ESM::ESMWriter& writer, Loading::Listener& listener) const
    {
        writer.startRecord(ESM::REC_DCOU);
        for (const auto& [id, count] : mDeathCount)
        {
            writer.writeHNRefId("ID__", id);
            writer.writeHNT("COUN", count);
        }
        writer.endRecord(ESM::REC_DCOU);
    }

    void Actors::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_DCOU)
        {
            while (reader.isNextSub("ID__"))
            {
                ESM::RefId id = reader.getRefId();
                int count;
                reader.getHNT(count, "COUN");
                if (MWBase::Environment::get().getESMStore()->find(id))
                    mDeathCount[id] = count;
            }
        }
    }

    void Actors::clear()
    {
        mIndex.clear();
        mActors.clear();
        mDeathCount.clear();
    }

    void Actors::updateMagicEffects(const MWWorld::Ptr& ptr) const
    {
        adjustMagicEffects(ptr, 0.f);
    }

    bool Actors::isReadyToBlock(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;

        return it->second->getCharacterController().isReadyToBlock();
    }

    bool Actors::isCastingSpell(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;

        return it->second->getCharacterController().isCastingSpell();
    }

    bool Actors::isAttackingOrSpell(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;

        return it->second->getCharacterController().isAttackingOrSpell();
    }

    int Actors::getGreetingTimer(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return 0;

        return it->second->getGreetingTimer();
    }

    float Actors::getAngleToPlayer(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return 0.f;

        return it->second->getAngleToPlayer();
    }

    GreetingState Actors::getGreetingState(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return Greet_None;

        return it->second->getGreetingState();
    }

    bool Actors::isTurningToPlayer(const MWWorld::Ptr& ptr) const
    {
        const auto it = mIndex.find(ptr.mRef);
        if (it == mIndex.end())
            return false;

        return it->second->isTurningToPlayer();
    }

    void Actors::fastForwardAi() const
    {
        if (!MWBase::Environment::get().getMechanicsManager()->isAIActive())
            return;

        for (auto it = mActors.begin(); it != mActors.end();)
        {
            const MWWorld::Ptr ptr = it->getPtr();
            ++it;
            if (ptr == getPlayer() || !isConscious(ptr) || ptr.getClass().getCreatureStats(ptr).isParalyzed())
                continue;
            MWMechanics::AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            seq.fastForward(ptr);
        }
    }

    const std::set<MWWorld::Ptr>& SidingCache::getActorsSidingWith(const MWWorld::Ptr& actor)
    {
        // If we have already found actor's allies, use the cache
        auto search = mCache.find(actor);
        if (search != mCache.end())
            return search->second;
        std::set<MWWorld::Ptr>& out = mCache[actor];
        for (const MWWorld::Ptr& follower : mActors.getActorsSidingWith(actor, mExcludeInfighting))
        {
            if (out.insert(follower).second && follower != actor)
            {
                const auto& allies = getActorsSidingWith(follower);
                out.insert(allies.begin(), allies.end());
            }
        }

        // Cache ptrs and their sets of allies
        for (const MWWorld::Ptr& iter : out)
        {
            if (iter == actor)
                continue;
            search = mCache.find(iter);
            if (search == mCache.end())
                mCache.emplace(iter, out);
        }
        return out;
    }
}
