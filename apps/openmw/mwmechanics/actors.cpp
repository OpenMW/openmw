#include "actors.hpp"

#include <optional>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/mathutil.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwmechanics/aibreathe.hpp"

#include "../mwrender/vismask.hpp"

#include "spellcasting.hpp"
#include "steering.hpp"
#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "character.hpp"
#include "aicombat.hpp"
#include "aicombataction.hpp"
#include "aifollow.hpp"
#include "aipursue.hpp"
#include "aiwander.hpp"
#include "actor.hpp"
#include "summoning.hpp"
#include "actorutil.hpp"
#include "tickableeffects.hpp"

namespace
{

bool isConscious(const MWWorld::Ptr& ptr)
{
    const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
    return !stats.isDead() && !stats.getKnockedDown();
}

int getBoundItemSlot (const std::string& itemId)
{
    static std::map<std::string, int> boundItemsMap;
    if (boundItemsMap.empty())
    {
        std::string boundId = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sMagicBoundBootsID")->mValue.getString();
        boundItemsMap[boundId] = MWWorld::InventoryStore::Slot_Boots;

        boundId = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sMagicBoundCuirassID")->mValue.getString();
        boundItemsMap[boundId] = MWWorld::InventoryStore::Slot_Cuirass;

        boundId = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sMagicBoundLeftGauntletID")->mValue.getString();
        boundItemsMap[boundId] = MWWorld::InventoryStore::Slot_LeftGauntlet;

        boundId = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sMagicBoundRightGauntletID")->mValue.getString();
        boundItemsMap[boundId] = MWWorld::InventoryStore::Slot_RightGauntlet;

        boundId = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sMagicBoundHelmID")->mValue.getString();
        boundItemsMap[boundId] = MWWorld::InventoryStore::Slot_Helmet;

        boundId = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sMagicBoundShieldID")->mValue.getString();
        boundItemsMap[boundId] = MWWorld::InventoryStore::Slot_CarriedLeft;
    }

    int slot = MWWorld::InventoryStore::Slot_CarriedRight;
    std::map<std::string, int>::iterator it = boundItemsMap.find(itemId);
    if (it != boundItemsMap.end())
        slot = it->second;

    return slot;
}

class CheckActorCommanded : public MWMechanics::EffectSourceVisitor
{
    MWWorld::Ptr mActor;
public:
    bool mCommanded;

    CheckActorCommanded(const MWWorld::Ptr& actor)
        : mActor(actor)
        , mCommanded(false){}

    void visit (MWMechanics::EffectKey key, int effectIndex,
                        const std::string& sourceName, const std::string& sourceId, int casterActorId,
                        float magnitude, float remainingTime = -1, float totalTime = -1) override
    {
        if (((key.mId == ESM::MagicEffect::CommandHumanoid && mActor.getClass().isNpc())
            || (key.mId == ESM::MagicEffect::CommandCreature && mActor.getTypeName() == typeid(ESM::Creature).name()))
            && magnitude >= mActor.getClass().getCreatureStats(mActor).getLevel())
                mCommanded = true;
    }
};

// Check for command effects having ended and remove package if necessary
void adjustCommandedActor (const MWWorld::Ptr& actor)
{
    CheckActorCommanded check(actor);
    MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
    stats.getActiveSpells().visitEffectSources(check);

    bool hasCommandPackage = false;

    auto it = stats.getAiSequence().begin();
    for (; it != stats.getAiSequence().end(); ++it)
    {
        if ((*it)->getTypeId() == MWMechanics::AiPackageTypeId::Follow &&
                static_cast<const MWMechanics::AiFollow*>(it->get())->isCommanded())
        {
            hasCommandPackage = true;
            break;
        }
    }

    if (!check.mCommanded && hasCommandPackage)
        stats.getAiSequence().erase(it);
}

void getRestorationPerHourOfSleep (const MWWorld::Ptr& ptr, float& health, float& magicka)
{
    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
    const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    float endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();
    health = 0.1f * endurance;

    float fRestMagicMult = settings.find("fRestMagicMult")->mValue.getFloat ();
    magicka = fRestMagicMult * stats.getAttribute(ESM::Attribute::Intelligence).getModified();
}

template<class T>
void forEachFollowingPackage(MWMechanics::Actors::PtrActorMap& actors, const MWWorld::Ptr& actor, const MWWorld::Ptr& player, T&& func)
{
    for(auto& iter : actors)
    {
        const MWWorld::Ptr &iteratedActor = iter.first;
        if (iteratedActor == player || iteratedActor == actor)
            continue;

        const MWMechanics::CreatureStats &stats = iteratedActor.getClass().getCreatureStats(iteratedActor);
        if (stats.isDead())
            continue;

        // An actor counts as following if AiFollow is the current AiPackage,
        // or there are only Combat and Wander packages before the AiFollow package
        for (const auto& package : stats.getAiSequence())
        {
            if(!func(iter, package))
                break;
        }
    }
}

}

namespace MWMechanics
{
    static const int GREETING_SHOULD_START = 4; // how many updates should pass before NPC can greet player
    static const int GREETING_SHOULD_END = 20;  // how many updates should pass before NPC stops turning to player
    static const int GREETING_COOLDOWN = 40;    // how many updates should pass before NPC can continue movement
    static const float DECELERATE_DISTANCE = 512.f;

    namespace
    {
        float getTimeToDestination(const AiPackage& package, const osg::Vec3f& position, float speed, float duration, const osg::Vec3f& halfExtents)
        {
            const auto distanceToNextPathPoint = (package.getNextPathPoint(package.getDestination()) - position).length();
            return (distanceToNextPathPoint - package.getNextPathPointTolerance(speed, duration, halfExtents)) / speed;
        }
    }

    class GetStuntedMagickaDuration : public MWMechanics::EffectSourceVisitor
    {
    public:
        float mRemainingTime;

        GetStuntedMagickaDuration(const MWWorld::Ptr& actor)
            : mRemainingTime(0.f){}

        void visit (MWMechanics::EffectKey key, int effectIndex,
                            const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1) override
        {
            if (mRemainingTime == -1) return;

            if (key.mId == ESM::MagicEffect::StuntedMagicka)
            {
                if (totalTime == -1)
                {
                    mRemainingTime = -1;
                    return;
                }

                if (remainingTime > mRemainingTime)
                    mRemainingTime = remainingTime;
            }
        }
    };

    class GetCurrentMagnitudes : public MWMechanics::EffectSourceVisitor
    {
        std::string mSpellId;

    public:
        GetCurrentMagnitudes(const std::string& spellId)
            : mSpellId(spellId)
        {
        }

        void visit (MWMechanics::EffectKey key, int effectIndex,
                            const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1) override
        {
            if (magnitude <= 0)
                return;

            if (sourceId != mSpellId)
                return;

            mMagnitudes.emplace_back(key, magnitude);
        }

        std::vector<std::pair<MWMechanics::EffectKey, float>> mMagnitudes;
    };

    class GetCorprusSpells : public MWMechanics::EffectSourceVisitor
    {

    public:
        void visit (MWMechanics::EffectKey key, int effectIndex,
                            const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1) override
        {
            if (key.mId != ESM::MagicEffect::Corprus)
                return;

            mSpells.push_back(sourceId);
        }

        std::vector<std::string> mSpells;
    };

    class SoulTrap : public MWMechanics::EffectSourceVisitor
    {
        MWWorld::Ptr mCreature;
        MWWorld::Ptr mActor;
        bool mTrapped;
    public:
        SoulTrap(const MWWorld::Ptr& trappedCreature)
            : mCreature(trappedCreature)
            , mTrapped(false)
        {
        }

        void visit (MWMechanics::EffectKey key, int effectIndex,
                            const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1) override
        {
            if (mTrapped)
                return;
            if (key.mId != ESM::MagicEffect::Soultrap)
                return;
            if (magnitude <= 0)
                return;

            MWBase::World* world = MWBase::Environment::get().getWorld();

            MWWorld::Ptr caster = world->searchPtrViaActorId(casterActorId);
            if (caster.isEmpty() || !caster.getClass().isActor())
                return;

            static const float fSoulgemMult = world->getStore().get<ESM::GameSetting>().find("fSoulgemMult")->mValue.getFloat();

            int creatureSoulValue = mCreature.get<ESM::Creature>()->mBase->mData.mSoul;
            if (creatureSoulValue == 0)
                return;

            // Use the smallest soulgem that is large enough to hold the soul
            MWWorld::ContainerStore& container = caster.getClass().getContainerStore(caster);
            MWWorld::ContainerStoreIterator gem = container.end();
            float gemCapacity = std::numeric_limits<float>::max();
            std::string soulgemFilter = "misc_soulgem"; // no other way to check for soulgems? :/
            for (MWWorld::ContainerStoreIterator it = container.begin(MWWorld::ContainerStore::Type_Miscellaneous);
                 it != container.end(); ++it)
            {
                const std::string& id = it->getCellRef().getRefId();
                if (id.size() >= soulgemFilter.size()
                        && id.substr(0,soulgemFilter.size()) == soulgemFilter)
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
                return;

            // Set the soul on just one of the gems, not the whole stack
            gem->getContainerStore()->unstack(*gem, caster);
            gem->getCellRef().setSoul(mCreature.getCellRef().getRefId());

            // Restack the gem with other gems with the same soul
            gem->getContainerStore()->restack(*gem);

            mTrapped = true;

            if (caster == getPlayer())
                MWBase::Environment::get().getWindowManager()->messageBox("#{sSoultrapSuccess}");

            const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                    .search("VFX_Soul_Trap");
            if (fx)
                MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + fx->mModel,
                    "", mCreature.getRefData().getPosition().asVec3());

            MWBase::Environment::get().getSoundManager()->playSound3D(
                mCreature.getRefData().getPosition().asVec3(), "conjuration hit", 1.f, 1.f
            );
        }
    };

    void Actors::addBoundItem (const std::string& itemId, const MWWorld::Ptr& actor)
    {
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        int slot = getBoundItemSlot(itemId);

        if (actor.getClass().getContainerStore(actor).count(itemId) != 0)
            return;

        MWWorld::ContainerStoreIterator prevItem = store.getSlot(slot);

        MWWorld::Ptr boundPtr = *store.MWWorld::ContainerStore::add(itemId, 1, actor);
        MWWorld::ActionEquip action(boundPtr);
        action.execute(actor);

        if (actor != MWMechanics::getPlayer())
            return;

        MWWorld::Ptr newItem;
        auto it = store.getSlot(slot);
        // Equip can fail because beast races cannot equip boots/helmets
        if(it != store.end())
            newItem = *it;

        if (newItem.isEmpty() || boundPtr != newItem)
            return;

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();

        // change draw state only if the item is in player's right hand
        if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
            player.setDrawState(MWMechanics::DrawState_Weapon);

        if (prevItem != store.end())
            player.setPreviousItem(itemId, prevItem->getCellRef().getRefId());
    }

    void Actors::removeBoundItem (const std::string& itemId, const MWWorld::Ptr& actor)
    {
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        int slot = getBoundItemSlot(itemId);

        MWWorld::ContainerStoreIterator currentItem = store.getSlot(slot);

        bool wasEquipped = currentItem != store.end() && Misc::StringUtils::ciEqual(currentItem->getCellRef().getRefId(), itemId);

        if (actor != MWMechanics::getPlayer())
        {
            store.remove(itemId, 1, actor);

            // Equip a replacement
            if (!wasEquipped)
                return;

            std::string type = currentItem->getTypeName();
            if (type != typeid(ESM::Weapon).name() && type != typeid(ESM::Armor).name() && type != typeid(ESM::Clothing).name())
                return;

            if (actor.getClass().getCreatureStats(actor).isDead())
                return;

            if (!actor.getClass().hasInventoryStore(actor))
                return;

            if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
                return;

            actor.getClass().getInventoryStore(actor).autoEquip(actor);

            return;
        }

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        std::string prevItemId = player.getPreviousItem(itemId);
        player.erasePreviousItem(itemId);

        if (!prevItemId.empty())
        {
            // Find previous item (or its replacement) by id.
            // we should equip previous item only if expired bound item was equipped.
            MWWorld::Ptr item = store.findReplacement(prevItemId);
            if (!item.isEmpty() && wasEquipped)
            {
                MWWorld::ActionEquip action(item);
                action.execute(actor);
            }
        }

        store.remove(itemId, 1, actor);
    }

    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {
        // magic effects
        adjustMagicEffects (ptr);
        if (ptr.getClass().getCreatureStats(ptr).needToRecalcDynamicStats())
            calculateDynamicStats (ptr);

        calculateCreatureStatModifiers (ptr, duration);
        // fatigue restoration
        calculateRestoration(ptr, duration);
    }

    void Actors::updateHeadTracking(const MWWorld::Ptr& actor, const MWWorld::Ptr& targetActor,
                                    MWWorld::Ptr& headTrackTarget, float& sqrHeadTrackDistance,
                                    bool inCombatOrPursue)
    {
        if (!actor.getRefData().getBaseNode())
            return;

        if (targetActor.getClass().getCreatureStats(targetActor).isDead())
            return;

        static const float fMaxHeadTrackDistance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("fMaxHeadTrackDistance")->mValue.getFloat();
        static const float fInteriorHeadTrackMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("fInteriorHeadTrackMult")->mValue.getFloat();
        float maxDistance = fMaxHeadTrackDistance;
        const ESM::Cell* currentCell = actor.getCell()->getCell();
        if (!currentCell->isExterior() && !(currentCell->mData.mFlags & ESM::Cell::QuasiEx))
            maxDistance *= fInteriorHeadTrackMult;

        const osg::Vec3f actor1Pos(actor.getRefData().getPosition().asVec3());
        const osg::Vec3f actor2Pos(targetActor.getRefData().getPosition().asVec3());
        float sqrDist = (actor1Pos - actor2Pos).length2();

        if (sqrDist > std::min(maxDistance * maxDistance, sqrHeadTrackDistance) && !inCombatOrPursue)
            return;

        // stop tracking when target is behind the actor
        osg::Vec3f actorDirection = actor.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,1,0);
        osg::Vec3f targetDirection(actor2Pos - actor1Pos);
        actorDirection.z() = 0;
        targetDirection.z() = 0;
        if ((actorDirection * targetDirection > 0 || inCombatOrPursue)
            && MWBase::Environment::get().getWorld()->getLOS(actor, targetActor) // check LOS and awareness last as it's the most expensive function
            && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(targetActor, actor))
        {
            sqrHeadTrackDistance = sqrDist;
            headTrackTarget = targetActor;
        }
    }

    void Actors::playIdleDialogue(const MWWorld::Ptr& actor)
    {
        if (!actor.getClass().isActor() || actor == getPlayer() || MWBase::Environment::get().getSoundManager()->sayActive(actor))
            return;

        const CreatureStats &stats = actor.getClass().getCreatureStats(actor);
        if (stats.getAiSetting(CreatureStats::AI_Hello).getModified() == 0)
            return;

        const MWMechanics::AiSequence& seq = stats.getAiSequence();
        if (seq.isInCombat() || seq.hasPackage(AiPackageTypeId::Follow) || seq.hasPackage(AiPackageTypeId::Escort))
            return;

        const osg::Vec3f playerPos(getPlayer().getRefData().getPosition().asVec3());
        const osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        MWBase::World* world = MWBase::Environment::get().getWorld();
        if (world->isSwimming(actor) || (playerPos - actorPos).length2() >= 3000 * 3000)
            return;

        // Our implementation is not FPS-dependent unlike Morrowind's so it needs to be recalibrated.
        // We chose to use the chance MW would have when run at 60 FPS with the default value of the GMST.
        const float delta = MWBase::Environment::get().getFrameDuration() * 6.f;
        static const float fVoiceIdleOdds = world->getStore().get<ESM::GameSetting>().find("fVoiceIdleOdds")->mValue.getFloat();
        if (Misc::Rng::rollProbability() * 10000.f < fVoiceIdleOdds * delta && world->getLOS(getPlayer(), actor))
            MWBase::Environment::get().getDialogueManager()->say(actor, "idle");
    }

    void Actors::updateMovementSpeed(const MWWorld::Ptr& actor)
    {
        if (mSmoothMovement)
            return;

        CreatureStats &stats = actor.getClass().getCreatureStats(actor);
        MWMechanics::AiSequence& seq = stats.getAiSequence();

        if (!seq.isEmpty() && seq.getActivePackage().useVariableSpeed())
        {
            osg::Vec3f targetPos = seq.getActivePackage().getDestination();
            osg::Vec3f actorPos = actor.getRefData().getPosition().asVec3();
            float distance = (targetPos - actorPos).length();

            if (distance < DECELERATE_DISTANCE)
            {
                float speedCoef = std::max(0.7f, 0.2f + 0.8f * distance / DECELERATE_DISTANCE);
                auto& movement = actor.getClass().getMovementSettings(actor);
                movement.mPosition[0] *= speedCoef;
                movement.mPosition[1] *= speedCoef;
            }
        }
    }

    void Actors::updateGreetingState(const MWWorld::Ptr& actor, Actor& actorState, bool turnOnly)
    {
        if (!actor.getClass().isActor() || actor == getPlayer())
            return;

        CreatureStats &stats = actor.getClass().getCreatureStats(actor);
        const MWMechanics::AiSequence& seq = stats.getAiSequence();
        const auto packageId = seq.getTypeId();

        if (seq.isInCombat() ||
            MWBase::Environment::get().getWorld()->isSwimming(actor) ||
            (packageId != AiPackageTypeId::Wander && packageId != AiPackageTypeId::Travel && packageId != AiPackageTypeId::None))
        {
            actorState.setTurningToPlayer(false);
            actorState.setGreetingTimer(0);
            actorState.setGreetingState(Greet_None);
            return;
        }

        MWWorld::Ptr player = getPlayer();
        osg::Vec3f playerPos(player.getRefData().getPosition().asVec3());
        osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        osg::Vec3f dir = playerPos - actorPos;

        if (actorState.isTurningToPlayer())
        {
            // Reduce the turning animation glitch by using a *HUGE* value of
            // epsilon...  TODO: a proper fix might be in either the physics or the
            // animation subsystem
            if (zTurn(actor, actorState.getAngleToPlayer(), osg::DegreesToRadians(5.f)))
            {
                actorState.setTurningToPlayer(false);
                // An original engine launches an endless idle2 when an actor greets player.
                playAnimationGroup (actor, "idle2", 0, std::numeric_limits<int>::max(), false);
            }
        }

        if (turnOnly)
            return;

        // Play a random voice greeting if the player gets too close
        static int iGreetDistanceMultiplier = MWBase::Environment::get().getWorld()->getStore()
            .get<ESM::GameSetting>().find("iGreetDistanceMultiplier")->mValue.getInteger();

        float helloDistance = static_cast<float>(stats.getAiSetting(CreatureStats::AI_Hello).getModified() * iGreetDistanceMultiplier);

        int greetingTimer = actorState.getGreetingTimer();
        GreetingState greetingState = actorState.getGreetingState();
        if (greetingState == Greet_None)
        {
            if ((playerPos - actorPos).length2() <= helloDistance*helloDistance &&
                !player.getClass().getCreatureStats(player).isDead() && !actor.getClass().getCreatureStats(actor).isParalyzed()
                && MWBase::Environment::get().getWorld()->getLOS(player, actor)
                && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, actor))
                greetingTimer++;

            if (greetingTimer >= GREETING_SHOULD_START)
            {
                greetingState = Greet_InProgress;
                MWBase::Environment::get().getDialogueManager()->say(actor, "hello");
                greetingTimer = 0;
            }
        }

        if (greetingState == Greet_InProgress)
        {
            greetingTimer++;

            if (!stats.getMovementFlag(CreatureStats::Flag_ForceJump) && !stats.getMovementFlag(CreatureStats::Flag_ForceSneak)
                && (greetingTimer <= GREETING_SHOULD_END || MWBase::Environment::get().getSoundManager()->sayActive(actor)))
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
            if ((playerPos - actorPos).length2() >= resetDist*resetDist)
                greetingState = Greet_None;
        }

        actorState.setGreetingTimer(greetingTimer);
        actorState.setGreetingState(greetingState);
    }

    void Actors::turnActorToFacePlayer(const MWWorld::Ptr& actor, Actor& actorState, const osg::Vec3f& dir)
    {
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
        actor.getClass().getMovementSettings(actor).mPosition[0] = 0;

        if (!actorState.isTurningToPlayer())
        {
            float from = dir.x();
            float to = dir.y();
            float angle = std::atan2(from, to);
            actorState.setAngleToPlayer(angle);
            float deltaAngle = Misc::normalizeAngle(angle - actor.getRefData().getPosition().rot[2]);
            if (!mSmoothMovement || std::abs(deltaAngle) > osg::DegreesToRadians(60.f))
                actorState.setTurningToPlayer(true);
        }
    }

    void Actors::engageCombat (const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, std::map<const MWWorld::Ptr, const std::set<MWWorld::Ptr> >& cachedAllies, bool againstPlayer)
    {
        // No combat for totally static creatures
        if (!actor1.getClass().isMobile(actor1))
            return;

        CreatureStats& creatureStats1 = actor1.getClass().getCreatureStats(actor1);
        if (creatureStats1.isDead() || creatureStats1.getAiSequence().isInCombat(actor2))
            return;

        const CreatureStats& creatureStats2 = actor2.getClass().getCreatureStats(actor2);
        if (creatureStats2.isDead())
            return;

        const osg::Vec3f actor1Pos(actor1.getRefData().getPosition().asVec3());
        const osg::Vec3f actor2Pos(actor2.getRefData().getPosition().asVec3());
        float sqrDist = (actor1Pos - actor2Pos).length2();

        if (sqrDist > mActorsProcessingRange*mActorsProcessingRange)
            return;

        // If this is set to true, actor1 will start combat with actor2 if the awareness check at the end of the method returns true
        bool aggressive = false;

        // Get actors allied with actor1. Includes those following or escorting actor1, actors following or escorting those actors, (recursive)
        // and any actor currently being followed or escorted by actor1
        std::set<MWWorld::Ptr> allies1;

        getActorsSidingWith(actor1, allies1, cachedAllies);

        // If an ally of actor1 has been attacked by actor2 or has attacked actor2, start combat between actor1 and actor2
        for (const MWWorld::Ptr &ally : allies1)
        {
            if (creatureStats1.getAiSequence().isInCombat(ally))
                continue;

            if (creatureStats2.matchesActorId(ally.getClass().getCreatureStats(ally).getHitAttemptActorId()))
            {
                MWBase::Environment::get().getMechanicsManager()->startCombat(actor1, actor2);
                // Also set the same hit attempt actor. Otherwise, if fighting the player, they may stop combat
                // if the player gets out of reach, while the ally would continue combat with the player
                creatureStats1.setHitAttemptActorId(ally.getClass().getCreatureStats(ally).getHitAttemptActorId());
                return;
            }

            // If there's been no attack attempt yet but an ally of actor1 is in combat with actor2, become aggressive to actor2
            if (ally.getClass().getCreatureStats(ally).getAiSequence().isInCombat(actor2))
                aggressive = true;
        }

        std::set<MWWorld::Ptr> playerAllies;
        getActorsSidingWith(MWMechanics::getPlayer(), playerAllies, cachedAllies);

        bool isPlayerFollowerOrEscorter = playerAllies.find(actor1) != playerAllies.end();

        // If actor2 and at least one actor2 are in combat with actor1, actor1 and its allies start combat with them
        // Doesn't apply for player followers/escorters
        if (!aggressive && !isPlayerFollowerOrEscorter)
        {
            // Check that actor2 is in combat with actor1
            if (actor2.getClass().getCreatureStats(actor2).getAiSequence().isInCombat(actor1))
            {
                std::set<MWWorld::Ptr> allies2;

                getActorsSidingWith(actor2, allies2, cachedAllies);

                // Check that an ally of actor2 is also in combat with actor1
                for (const MWWorld::Ptr &ally2 : allies2)
                {
                    if (ally2.getClass().getCreatureStats(ally2).getAiSequence().isInCombat(actor1))
                    {
                        MWBase::Environment::get().getMechanicsManager()->startCombat(actor1, actor2);
                        // Also have actor1's allies start combat
                        for (const MWWorld::Ptr& ally1 : allies1)
                            MWBase::Environment::get().getMechanicsManager()->startCombat(ally1, actor2);
                        return;
                    }
                }
            }
        }

        // Stop here if target is unreachable
        if (!canFight(actor1, actor2))
            return;

        // If set in the settings file, player followers and escorters will become aggressive toward enemies in combat with them or the player
        static const bool followersAttackOnSight = Settings::Manager::getBool("followers attack on sight", "Game");
        if (!aggressive && isPlayerFollowerOrEscorter && followersAttackOnSight)
        {
            if (actor2.getClass().getCreatureStats(actor2).getAiSequence().isInCombat(actor1))
                aggressive = true;
            else
            {
                for (const MWWorld::Ptr &ally : allies1)
                {
                    if (actor2.getClass().getCreatureStats(actor2).getAiSequence().isInCombat(ally))
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
                    aggressive = MWBase::Environment::get().getMechanicsManager()->isAggressive(actor1, actor2);
            }
        }

        // Make guards go aggressive with creatures that are in combat, unless the creature is a follower or escorter
        if (!aggressive && actor1.getClass().isClass(actor1, "Guard") && !actor2.getClass().isNpc() && creatureStats2.getAiSequence().isInCombat())
        {
            // Check if the creature is too far
            static const float fAlarmRadius = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fAlarmRadius")->mValue.getFloat();
            if (sqrDist > fAlarmRadius * fAlarmRadius)
                return;

            bool followerOrEscorter = false;
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
            if (!followerOrEscorter)
                aggressive = true;
        }

        // If any of the above conditions turned actor1 aggressive towards actor2, do an awareness check. If it passes, start combat with actor2.
        if (aggressive)
        {
            bool LOS = MWBase::Environment::get().getWorld()->getLOS(actor1, actor2)
                    && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor2, actor1);

            if (LOS)
                MWBase::Environment::get().getMechanicsManager()->startCombat(actor1, actor2);
        }
    }

    void Actors::adjustMagicEffects (const MWWorld::Ptr& creature)
    {
        CreatureStats& creatureStats =  creature.getClass().getCreatureStats (creature);
        if (creatureStats.isDeathAnimationFinished())
            return;

        MagicEffects now = creatureStats.getSpells().getMagicEffects();

        if (creature.getClass().hasInventoryStore(creature))
        {
            MWWorld::InventoryStore& store = creature.getClass().getInventoryStore (creature);
            now += store.getMagicEffects();
        }

        now += creatureStats.getActiveSpells().getMagicEffects();

        creatureStats.modifyMagicEffects(now);
    }

    void Actors::calculateDynamicStats (const MWWorld::Ptr& ptr)
    {
        CreatureStats& creatureStats = ptr.getClass().getCreatureStats (ptr);

        float intelligence = creatureStats.getAttribute(ESM::Attribute::Intelligence).getModified();

        float base = 1.f;
        if (ptr == getPlayer())
            base = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fPCbaseMagickaMult")->mValue.getFloat();
        else
            base = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fNPCbaseMagickaMult")->mValue.getFloat();

        double magickaFactor = base +
            creatureStats.getMagicEffects().get (EffectKey (ESM::MagicEffect::FortifyMaximumMagicka)).getMagnitude() * 0.1;

        DynamicStat<float> magicka = creatureStats.getMagicka();
        float diff = (static_cast<int>(magickaFactor*intelligence)) - magicka.getBase();
        float currentToBaseRatio = (magicka.getCurrent() / magicka.getBase());
        magicka.setModified(magicka.getModified() + diff, 0);
        magicka.setCurrent(magicka.getBase() * currentToBaseRatio, false, true);
        creatureStats.setMagicka(magicka);
    }

    void Actors::restoreDynamicStats (const MWWorld::Ptr& ptr, double hours, bool sleep)
    {
        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);
        if (stats.isDead())
            return;

        const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (sleep)
        {
            float health, magicka;
            getRestorationPerHourOfSleep(ptr, health, magicka);

            DynamicStat<float> stat = stats.getHealth();
            stat.setCurrent(stat.getCurrent() + health * hours);
            stats.setHealth(stat);

            double restoreHours = hours;
            bool stunted = stats.getMagicEffects ().get(ESM::MagicEffect::StuntedMagicka).getMagnitude() > 0;
            if (stunted)
            {
                // Stunted Magicka effect should be taken into account.
                GetStuntedMagickaDuration visitor(ptr);
                stats.getActiveSpells().visitEffectSources(visitor);
                stats.getSpells().visitEffectSources(visitor);
                if (ptr.getClass().hasInventoryStore(ptr))
                    ptr.getClass().getInventoryStore(ptr).visitEffectSources(visitor);

                // Take a maximum remaining duration of Stunted Magicka effects (-1 is a constant one) in game hours.
                if (visitor.mRemainingTime > 0)
                {
                    double timeScale = MWBase::Environment::get().getWorld()->getTimeScaleFactor();
                    if(timeScale == 0.0)
                        timeScale = 1;

                    restoreHours = std::max(0.0, hours - visitor.mRemainingTime * timeScale / 3600.f);
                }
                else if (visitor.mRemainingTime == -1)
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
        float fFatigueReturnBase = settings.find("fFatigueReturnBase")->mValue.getFloat ();
        float fFatigueReturnMult = settings.find("fFatigueReturnMult")->mValue.getFloat ();
        float fEndFatigueMult = settings.find("fEndFatigueMult")->mValue.getFloat ();

        float endurance = stats.getAttribute (ESM::Attribute::Endurance).getModified ();

        float normalizedEncumbrance = ptr.getClass().getNormalizedEncumbrance(ptr);
        if (normalizedEncumbrance > 1)
            normalizedEncumbrance = 1;

        float x = fFatigueReturnBase + fFatigueReturnMult * (1 - normalizedEncumbrance);
        x *= fEndFatigueMult * endurance;

        fatigue.setCurrent (fatigue.getCurrent() + 3600 * x * hours);
        stats.setFatigue (fatigue);
    }

    void Actors::calculateRestoration (const MWWorld::Ptr& ptr, float duration)
    {
        if (ptr.getClass().getCreatureStats(ptr).isDead())
            return;

        MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);

        // Current fatigue can be above base value due to a fortify effect.
        // In that case stop here and don't try to restore.
        DynamicStat<float> fatigue = stats.getFatigue();
        if (fatigue.getCurrent() >= fatigue.getBase())
            return;

        // Restore fatigue
        float endurance = stats.getAttribute(ESM::Attribute::Endurance).getModified();
        const MWWorld::Store<ESM::GameSetting>& settings = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fFatigueReturnBase = settings.find("fFatigueReturnBase")->mValue.getFloat ();
        static const float fFatigueReturnMult = settings.find("fFatigueReturnMult")->mValue.getFloat ();

        float x = fFatigueReturnBase + fFatigueReturnMult * endurance;

        fatigue.setCurrent (fatigue.getCurrent() + duration * x);
        stats.setFatigue (fatigue);
    }

    class ExpiryVisitor : public EffectSourceVisitor
    {
        private:
            MWWorld::Ptr mActor;
            float mDuration;

        public:
            ExpiryVisitor(const MWWorld::Ptr& actor, float duration)
                : mActor(actor), mDuration(duration)
            {
            }

            void visit (MWMechanics::EffectKey key, int /*effectIndex*/,
                                const std::string& /*sourceName*/, const std::string& /*sourceId*/, int /*casterActorId*/,
                                float magnitude, float remainingTime = -1, float /*totalTime*/ = -1) override
            {
                if (magnitude > 0 && remainingTime > 0 && remainingTime < mDuration)
                {
                    CreatureStats& creatureStats = mActor.getClass().getCreatureStats(mActor);
                    if (effectTick(creatureStats, mActor, key, magnitude * remainingTime))
                        creatureStats.getMagicEffects().add(key, -magnitude);
                }
            }
    };

    void Actors::applyCureEffects(const MWWorld::Ptr& actor)
    {
        CreatureStats &creatureStats = actor.getClass().getCreatureStats(actor);
        const MagicEffects &effects = creatureStats.getMagicEffects();

        if (effects.get(ESM::MagicEffect::CurePoison).getModifier() > 0)
        {
            creatureStats.getActiveSpells().purgeEffect(ESM::MagicEffect::Poison);
            creatureStats.getSpells().purgeEffect(ESM::MagicEffect::Poison);
            if (actor.getClass().hasInventoryStore(actor))
                actor.getClass().getInventoryStore(actor).purgeEffect(ESM::MagicEffect::Poison);
        }
        if (effects.get(ESM::MagicEffect::CureParalyzation).getModifier() > 0)
        {
            creatureStats.getActiveSpells().purgeEffect(ESM::MagicEffect::Paralyze);
            creatureStats.getSpells().purgeEffect(ESM::MagicEffect::Paralyze);
            if (actor.getClass().hasInventoryStore(actor))
                actor.getClass().getInventoryStore(actor).purgeEffect(ESM::MagicEffect::Paralyze);
        }
        if (effects.get(ESM::MagicEffect::CureCommonDisease).getModifier() > 0)
        {
            creatureStats.getSpells().purgeCommonDisease();
        }
        if (effects.get(ESM::MagicEffect::CureBlightDisease).getModifier() > 0)
        {
            creatureStats.getSpells().purgeBlightDisease();
        }
        if (effects.get(ESM::MagicEffect::CureCorprusDisease).getModifier() > 0)
        {
            creatureStats.getActiveSpells().purgeCorprusDisease();
            creatureStats.getSpells().purgeCorprusDisease();
            if (actor.getClass().hasInventoryStore(actor))
                actor.getClass().getInventoryStore(actor).purgeEffect(ESM::MagicEffect::Corprus, true);
        }
        if (effects.get(ESM::MagicEffect::RemoveCurse).getModifier() > 0)
        {
            creatureStats.getSpells().purgeCurses();
        }
    }

    void Actors::calculateCreatureStatModifiers (const MWWorld::Ptr& ptr, float duration)
    {
        CreatureStats &creatureStats = ptr.getClass().getCreatureStats(ptr);
        const MagicEffects &effects = creatureStats.getMagicEffects();
        bool godmode = ptr == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        applyCureEffects(ptr);

        bool wasDead = creatureStats.isDead();

        if (duration > 0)
        {
            // Apply correct magnitude for tickable effects that have just expired,
            // in case duration > remaining time of effect.
            // One case where this will happen is when the player uses the rest/wait command
            // while there is a tickable effect active that should expire before the end of the rest/wait.
            ExpiryVisitor visitor(ptr, duration);
            creatureStats.getActiveSpells().visitEffectSources(visitor);

            for (MagicEffects::Collection::const_iterator it = effects.begin(); it != effects.end(); ++it)
            {
                // tickable effects (i.e. effects having a lasting impact after expiry)
                effectTick(creatureStats, ptr, it->first, it->second.getMagnitude() * duration);

                // instant effects are already applied on spell impact in spellcasting.cpp, but may also come from permanent abilities
                if (it->second.getMagnitude() > 0)
                {
                    CastSpell cast(ptr, ptr);
                    if (cast.applyInstantEffect(ptr, ptr, it->first, it->second.getMagnitude()))
                    {
                        creatureStats.getSpells().purgeEffect(it->first.mId);
                        creatureStats.getActiveSpells().purgeEffect(it->first.mId);
                        if (ptr.getClass().hasInventoryStore(ptr))
                            ptr.getClass().getInventoryStore(ptr).purgeEffect(it->first.mId);
                    }
                }
            }
        }

        // purge levitate effect if levitation is disabled
        // check only modifier, because base value can be setted from SetFlying console command.
        if (MWBase::Environment::get().getWorld()->isLevitationEnabled() == false && effects.get(ESM::MagicEffect::Levitate).getModifier() > 0)
        {
            creatureStats.getSpells().purgeEffect(ESM::MagicEffect::Levitate);
            creatureStats.getActiveSpells().purgeEffect(ESM::MagicEffect::Levitate);
            if (ptr.getClass().hasInventoryStore(ptr))
                ptr.getClass().getInventoryStore(ptr).purgeEffect(ESM::MagicEffect::Levitate);

            if (ptr == getPlayer())
            {
                MWBase::Environment::get().getWindowManager()->messageBox ("#{sLevitateDisabled}");
            }
        }

        // dynamic stats
        for (int i = 0; i < 3; ++i)
        {
            DynamicStat<float> stat = creatureStats.getDynamic(i);
            float fortify = effects.get(ESM::MagicEffect::FortifyHealth + i).getMagnitude();
            float drain = 0.f;
            if (!godmode)
                drain = effects.get(ESM::MagicEffect::DrainHealth + i).getMagnitude();
            stat.setCurrentModifier(fortify - drain,
                // Magicka can be decreased below zero due to a fortify effect wearing off
                // Fatigue can be decreased below zero meaning the actor will be knocked out
                i == 1 || i == 2);

            creatureStats.setDynamic(i, stat);
        }

        // attributes
        for(int i = 0;i < ESM::Attribute::Length;++i)
        {
            AttributeValue stat = creatureStats.getAttribute(i);
            float fortify = effects.get(EffectKey(ESM::MagicEffect::FortifyAttribute, i)).getMagnitude();
            float drain = 0.f, absorb = 0.f;
            if (!godmode)
            {
                drain = effects.get(EffectKey(ESM::MagicEffect::DrainAttribute, i)).getMagnitude();
                absorb = effects.get(EffectKey(ESM::MagicEffect::AbsorbAttribute, i)).getMagnitude();
            }
            stat.setModifier(static_cast<int>(fortify - drain - absorb));

            creatureStats.setAttribute(i, stat);
        }

        if (creatureStats.needToRecalcDynamicStats())
            calculateDynamicStats(ptr);

        if (ptr == getPlayer())
        {
            GetCorprusSpells getCorprusSpellsVisitor;
            creatureStats.getSpells().visitEffectSources(getCorprusSpellsVisitor);
            creatureStats.getActiveSpells().visitEffectSources(getCorprusSpellsVisitor);
            ptr.getClass().getInventoryStore(ptr).visitEffectSources(getCorprusSpellsVisitor);
            std::vector<std::string> corprusSpells = getCorprusSpellsVisitor.mSpells;
            std::vector<std::string> corprusSpellsToRemove;

            for (auto it = creatureStats.getCorprusSpells().begin(); it != creatureStats.getCorprusSpells().end(); ++it)
            {
                if(std::find(corprusSpells.begin(), corprusSpells.end(), it->first) == corprusSpells.end())
                {
                    // Corprus effect expired, remove entry and restore stats.
                    MWBase::Environment::get().getMechanicsManager()->restoreStatsAfterCorprus(ptr, it->first);
                    corprusSpellsToRemove.push_back(it->first);
                    corprusSpells.erase(std::remove(corprusSpells.begin(), corprusSpells.end(), it->first), corprusSpells.end());
                    continue;
                }

                corprusSpells.erase(std::remove(corprusSpells.begin(), corprusSpells.end(), it->first), corprusSpells.end());

                if (MWBase::Environment::get().getWorld()->getTimeStamp() >= it->second.mNextWorsening)
                {
                    it->second.mNextWorsening += CorprusStats::sWorseningPeriod;
                    GetCurrentMagnitudes getMagnitudesVisitor (it->first);
                    creatureStats.getSpells().visitEffectSources(getMagnitudesVisitor);
                    creatureStats.getActiveSpells().visitEffectSources(getMagnitudesVisitor);
                    ptr.getClass().getInventoryStore(ptr).visitEffectSources(getMagnitudesVisitor);
                    for (auto& effectMagnitude : getMagnitudesVisitor.mMagnitudes)
                    {
                        if (effectMagnitude.first.mId == ESM::MagicEffect::FortifyAttribute)
                        {
                            AttributeValue attr = creatureStats.getAttribute(effectMagnitude.first.mArg);
                            attr.damage(-effectMagnitude.second);
                            creatureStats.setAttribute(effectMagnitude.first.mArg, attr);
                            it->second.mWorsenings[effectMagnitude.first.mArg] = 0;
                        }
                        else if (effectMagnitude.first.mId == ESM::MagicEffect::DrainAttribute)
                        {
                            AttributeValue attr = creatureStats.getAttribute(effectMagnitude.first.mArg);
                            int currentDamage = attr.getDamage();
                            if (currentDamage >= 0)
                                it->second.mWorsenings[effectMagnitude.first.mArg] = std::min(it->second.mWorsenings[effectMagnitude.first.mArg], currentDamage);

                            it->second.mWorsenings[effectMagnitude.first.mArg] += effectMagnitude.second;
                            attr.damage(effectMagnitude.second);
                            creatureStats.setAttribute(effectMagnitude.first.mArg, attr);
                        }
                    }

                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicCorprusWorsens}");
                }
            }

            for (std::string& oldCorprusSpell : corprusSpellsToRemove)
            {
                 creatureStats.removeCorprusSpell(oldCorprusSpell);
            }

            for (std::string& newCorprusSpell : corprusSpells)
            {
                CorprusStats corprus;
                for (int i=0; i<ESM::Attribute::Length; ++i)
                    corprus.mWorsenings[i] = 0;
                corprus.mNextWorsening = MWBase::Environment::get().getWorld()->getTimeStamp() + CorprusStats::sWorseningPeriod;

                creatureStats.addCorprusSpell(newCorprusSpell, corprus);
            }
        }

        // AI setting modifiers
        int creature = !ptr.getClass().isNpc();
        if (creature && ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Humanoid)
            creature = false;
        // Note: the Creature variants only work on normal creatures, not on daedra or undead creatures.
        if (!creature || ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Creatures)
        {
            Stat<int> stat = creatureStats.getAiSetting(CreatureStats::AI_Fight);
            stat.setModifier(static_cast<int>(effects.get(ESM::MagicEffect::FrenzyHumanoid + creature).getMagnitude()
                - effects.get(ESM::MagicEffect::CalmHumanoid+creature).getMagnitude()));
            creatureStats.setAiSetting(CreatureStats::AI_Fight, stat);

            stat = creatureStats.getAiSetting(CreatureStats::AI_Flee);
            stat.setModifier(static_cast<int>(effects.get(ESM::MagicEffect::DemoralizeHumanoid + creature).getMagnitude()
                - effects.get(ESM::MagicEffect::RallyHumanoid+creature).getMagnitude()));
            creatureStats.setAiSetting(CreatureStats::AI_Flee, stat);
        }
        if (creature && ptr.get<ESM::Creature>()->mBase->mData.mType == ESM::Creature::Undead)
        {
            Stat<int> stat = creatureStats.getAiSetting(CreatureStats::AI_Flee);
            stat.setModifier(static_cast<int>(effects.get(ESM::MagicEffect::TurnUndead).getMagnitude()));
            creatureStats.setAiSetting(CreatureStats::AI_Flee, stat);
        }

        if (!wasDead && creatureStats.isDead())
        {
            // The actor was killed by a magic effect. Figure out if the player was responsible for it.
            const ActiveSpells& spells = creatureStats.getActiveSpells();
            MWWorld::Ptr player = getPlayer();
            std::set<MWWorld::Ptr> playerFollowers;
            getActorsSidingWith(player, playerFollowers);

            for (ActiveSpells::TIterator it = spells.begin(); it != spells.end(); ++it)
            {
                bool actorKilled = false;

                const ActiveSpells::ActiveSpellParams& spell = it->second;
                MWWorld::Ptr caster = MWBase::Environment::get().getWorld()->searchPtrViaActorId(spell.mCasterActorId);
                for (std::vector<ActiveSpells::ActiveEffect>::const_iterator effectIt = spell.mEffects.begin();
                     effectIt != spell.mEffects.end(); ++effectIt)
                {
                    int effectId = effectIt->mEffectId;
                    bool isDamageEffect = false;

                    int damageEffects[] = {
                        ESM::MagicEffect::FireDamage, ESM::MagicEffect::ShockDamage, ESM::MagicEffect::FrostDamage, ESM::MagicEffect::Poison,
                        ESM::MagicEffect::SunDamage, ESM::MagicEffect::DamageHealth, ESM::MagicEffect::AbsorbHealth
                    };

                    for (unsigned int i=0; i<sizeof(damageEffects)/sizeof(int); ++i)
                    {
                        if (damageEffects[i] == effectId)
                            isDamageEffect = true;
                    }

                    if (isDamageEffect)
                    {
                        if (caster == player || playerFollowers.find(caster) != playerFollowers.end())
                        {
                            if (caster.getClass().isNpc() && caster.getClass().getNpcStats(caster).isWerewolf())
                                caster.getClass().getNpcStats(caster).addWerewolfKill();

                            MWBase::Environment::get().getMechanicsManager()->actorKilled(ptr, player);
                            actorKilled = true;
                            break;
                        }
                    }
                }

                if (actorKilled)
                    break;
            }
        }

        // TODO: dirty flag for magic effects to avoid some unnecessary work below?

        // any value of calm > 0 will stop the actor from fighting
        if ((effects.get(ESM::MagicEffect::CalmHumanoid).getMagnitude() > 0 && ptr.getClass().isNpc())
            || (effects.get(ESM::MagicEffect::CalmCreature).getMagnitude() > 0 && !ptr.getClass().isNpc()))
            creatureStats.getAiSequence().stopCombat();

        // Update bound effects
        // Note: in vanilla MW multiple bound items of the same type can be created by different spells.
        // As these extra copies are kinda useless this may or may not be important.
        static std::map<ESM::MagicEffect::Effects, std::string> boundItemsMap;
        if (boundItemsMap.empty())
        {
            boundItemsMap[ESM::MagicEffect::BoundBattleAxe] = "sMagicBoundBattleAxeID";
            boundItemsMap[ESM::MagicEffect::BoundBoots] = "sMagicBoundBootsID";
            boundItemsMap[ESM::MagicEffect::BoundCuirass] = "sMagicBoundCuirassID";
            boundItemsMap[ESM::MagicEffect::BoundDagger] = "sMagicBoundDaggerID";
            boundItemsMap[ESM::MagicEffect::BoundGloves] = "sMagicBoundLeftGauntletID"; // Note: needs RightGauntlet variant too (see below)
            boundItemsMap[ESM::MagicEffect::BoundHelm] = "sMagicBoundHelmID";
            boundItemsMap[ESM::MagicEffect::BoundLongbow] = "sMagicBoundLongbowID";
            boundItemsMap[ESM::MagicEffect::BoundLongsword] = "sMagicBoundLongswordID";
            boundItemsMap[ESM::MagicEffect::BoundMace] = "sMagicBoundMaceID";
            boundItemsMap[ESM::MagicEffect::BoundShield] = "sMagicBoundShieldID";
            boundItemsMap[ESM::MagicEffect::BoundSpear] = "sMagicBoundSpearID";
        }

        if(ptr.getClass().hasInventoryStore(ptr))
        {
            for (const auto& [effect, itemGmst] : boundItemsMap)
            {
                bool found = creatureStats.mBoundItems.find(effect) != creatureStats.mBoundItems.end();
                float magnitude = effects.get(effect).getMagnitude();
                if (found != (magnitude > 0))
                {
                    if (magnitude > 0)
                        creatureStats.mBoundItems.insert(effect);
                    else
                        creatureStats.mBoundItems.erase(effect);

                    std::string item = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                                itemGmst)->mValue.getString();

                    magnitude > 0 ? addBoundItem(item, ptr) : removeBoundItem(item, ptr);

                    if (effect == ESM::MagicEffect::BoundGloves)
                    {
                        item = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                                    "sMagicBoundRightGauntletID")->mValue.getString();
                        magnitude > 0 ? addBoundItem(item, ptr) : removeBoundItem(item, ptr);
                    }
                }
            }
        }

        // Summoned creature update visitor assumes the actor belongs to a cell.
        // This assumption isn't always valid for the player character.
        if (!ptr.isInCell())
            return;

        bool hasSummonEffect = false;
        for (MagicEffects::Collection::const_iterator it = effects.begin(); it != effects.end(); ++it)
        {
            if (isSummoningEffect(it->first.mId))
            {
                hasSummonEffect = true;
                break;
            }
        }

        if (!creatureStats.getSummonedCreatureMap().empty() || !creatureStats.getSummonedCreatureGraveyard().empty() || hasSummonEffect)
        {
            UpdateSummonedCreatures updateSummonedCreatures(ptr);
            creatureStats.getActiveSpells().visitEffectSources(updateSummonedCreatures);
            creatureStats.getSpells().visitEffectSources(updateSummonedCreatures);
            if (ptr.getClass().hasInventoryStore(ptr))
                ptr.getClass().getInventoryStore(ptr).visitEffectSources(updateSummonedCreatures);
            updateSummonedCreatures.process(mTimerDisposeSummonsCorpses == 0.f);
        }
    }

    void Actors::calculateNpcStatModifiers (const MWWorld::Ptr& ptr, float duration)
    {
        NpcStats &npcStats = ptr.getClass().getNpcStats(ptr);
        const MagicEffects &effects = npcStats.getMagicEffects();
        bool godmode = ptr == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        // skills
        for(int i = 0;i < ESM::Skill::Length;++i)
        {
            SkillValue& skill = npcStats.getSkill(i);
            float fortify = effects.get(EffectKey(ESM::MagicEffect::FortifySkill, i)).getMagnitude();
            float drain = 0.f, absorb = 0.f;
            if (!godmode)
            {
                drain = effects.get(EffectKey(ESM::MagicEffect::DrainSkill, i)).getMagnitude();
                absorb = effects.get(EffectKey(ESM::MagicEffect::AbsorbSkill, i)).getMagnitude();
            }
            skill.setModifier(static_cast<int>(fortify - drain - absorb));
        }
    }

    bool Actors::isAttackPreparing(const MWWorld::Ptr& ptr)
    {
        PtrActorMap::iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;
        CharacterController* ctrl = it->second->getCharacterController();

        return ctrl->isAttackPreparing();
    }

    bool Actors::isRunning(const MWWorld::Ptr& ptr)
    {
        PtrActorMap::iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;
        CharacterController* ctrl = it->second->getCharacterController();

        return ctrl->isRunning();
    }

    bool Actors::isSneaking(const MWWorld::Ptr& ptr)
    {
        PtrActorMap::iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;
        CharacterController* ctrl = it->second->getCharacterController();

        return ctrl->isSneaking();
    }

    void Actors::updateDrowning(const MWWorld::Ptr& ptr, float duration, bool isKnockedOut, bool isPlayer)
    {
        NpcStats &stats = ptr.getClass().getNpcStats(ptr);

        // When npc stats are just initialized, mTimeToStartDrowning == -1 and we should get value from GMST
        static const float fHoldBreathTime = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fHoldBreathTime")->mValue.getFloat();
        if (stats.getTimeToStartDrowning() == -1.f)
            stats.setTimeToStartDrowning(fHoldBreathTime);

        if (!isPlayer && stats.getTimeToStartDrowning() < fHoldBreathTime / 2)
        {
            AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            if (seq.getTypeId() != AiPackageTypeId::Breathe) //Only add it once
                seq.stack(AiBreathe(), ptr);
        }

        MWBase::World *world = MWBase::Environment::get().getWorld();
        bool knockedOutUnderwater = (isKnockedOut && world->isUnderwater(ptr.getCell(), osg::Vec3f(ptr.getRefData().getPosition().asVec3())));
        if((world->isSubmerged(ptr) || knockedOutUnderwater)
           && stats.getMagicEffects().get(ESM::MagicEffect::WaterBreathing).getMagnitude() == 0)
        {
            float timeLeft = 0.0f;
            if(knockedOutUnderwater)
                stats.setTimeToStartDrowning(0);
            else
            {
                timeLeft = stats.getTimeToStartDrowning() - duration;
                if(timeLeft < 0.0f)
                    timeLeft = 0.0f;
                stats.setTimeToStartDrowning(timeLeft);
            }

            bool godmode = isPlayer && MWBase::Environment::get().getWorld()->getGodModeState();

            if(timeLeft == 0.0f && !godmode)
            {
                // If drowning, apply 3 points of damage per second
                static const float fSuffocationDamage = world->getStore().get<ESM::GameSetting>().find("fSuffocationDamage")->mValue.getFloat();
                DynamicStat<float> health = stats.getHealth();
                health.setCurrent(health.getCurrent() - fSuffocationDamage*duration);
                stats.setHealth(health);

                // Play a drowning sound
                MWBase::SoundManager *sndmgr = MWBase::Environment::get().getSoundManager();
                if(!sndmgr->getSoundPlaying(ptr, "drown"))
                    sndmgr->playSound3D(ptr, "drown", 1.0f, 1.0f);

                if(isPlayer)
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);
            }
        }
        else
            stats.setTimeToStartDrowning(fHoldBreathTime);
    }

    void Actors::updateEquippedLight (const MWWorld::Ptr& ptr, float duration, bool mayEquip)
    {
        bool isPlayer = (ptr == getPlayer());

        MWWorld::InventoryStore &inventoryStore = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator heldIter =
                inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        /**
         * Automatically equip NPCs torches at night and unequip them at day
         */
        if (!isPlayer)
        {
            MWWorld::ContainerStoreIterator torch = inventoryStore.end();
            for (MWWorld::ContainerStoreIterator it = inventoryStore.begin(); it != inventoryStore.end(); ++it)
            {
                if (it->getTypeName() == typeid(ESM::Light).name() &&
                    it->getClass().canBeEquipped(*it, ptr).first)
                {
                    torch = it;
                    break;
                }
            }

            if (mayEquip)
            {
                if (torch != inventoryStore.end())
                {
                    if (!ptr.getClass().getCreatureStats (ptr).getAiSequence().isInCombat())
                    {
                        // For non-hostile NPCs, unequip whatever is in the left slot in favor of a light.
                        if (heldIter != inventoryStore.end() && heldIter->getTypeName() != typeid(ESM::Light).name())
                            inventoryStore.unequipItem(*heldIter, ptr);
                    }
                    else if (heldIter == inventoryStore.end() || heldIter->getTypeName() == typeid(ESM::Light).name())
                    {
                        // For hostile NPCs, see if they have anything better to equip first
                        auto shield = inventoryStore.getPreferredShield(ptr);
                        if(shield != inventoryStore.end())
                            inventoryStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, shield, ptr);
                    }

                    heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

                    // If we have a torch and can equip it, then equip it now.
                    if (heldIter == inventoryStore.end())
                    {
                        inventoryStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, torch, ptr);
                    }
                }
            }
            else
            {
                if (heldIter != inventoryStore.end() && heldIter->getTypeName() == typeid(ESM::Light).name())
                {
                    // At day, unequip lights and auto equip shields or other suitable items
                    // (Note: autoEquip will ignore lights)
                    inventoryStore.autoEquip(ptr);
                }
            }
        }

        heldIter = inventoryStore.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);

        //If holding a light...
        if(heldIter.getType() == MWWorld::ContainerStore::Type_Light)
        {
            // Use time from the player's light
            if(isPlayer)
            {
                // But avoid using it up if the light source is hidden
                MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
                if (anim && anim->getCarriedLeftShown())
                {
                    float timeRemaining = heldIter->getClass().getRemainingUsageTime(*heldIter);

                    // -1 is infinite light source. Other negative values are treated as 0.
                    if (timeRemaining != -1.0f)
                    {
                        timeRemaining -= duration;
                        if (timeRemaining <= 0.f)
                        {
                            inventoryStore.remove(*heldIter, 1, ptr); // remove it
                            return;
                        }

                        heldIter->getClass().setRemainingUsageTime(*heldIter, timeRemaining);
                    }
                }
            }

            // Both NPC and player lights extinguish in water.
            if(MWBase::Environment::get().getWorld()->isSwimming(ptr))
            {
                inventoryStore.remove(*heldIter, 1, ptr); // remove it

                // ...But, only the player makes a sound.
                if(isPlayer)
                    MWBase::Environment::get().getSoundManager()->playSound("torch out",
                            1.0, 1.0, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
            }
        }
    }

    void Actors::updateCrimePursuit(const MWWorld::Ptr& ptr, float duration)
    {
        MWWorld::Ptr player = getPlayer();
        if (ptr != player && ptr.getClass().isNpc())
        {
            // get stats of witness
            CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
            NpcStats& npcStats = ptr.getClass().getNpcStats(ptr);

            if (player.getClass().getNpcStats(player).isWerewolf())
                return;

            if (ptr.getClass().isClass(ptr, "Guard") && creatureStats.getAiSequence().getTypeId() != AiPackageTypeId::Pursue && !creatureStats.getAiSequence().isInCombat()
                && creatureStats.getMagicEffects().get(ESM::MagicEffect::CalmHumanoid).getMagnitude() == 0)
            {
                const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();
                static const int cutoff = esmStore.get<ESM::GameSetting>().find("iCrimeThreshold")->mValue.getInteger();
                // Force dialogue on sight if bounty is greater than the cutoff
                // In vanilla morrowind, the greeting dialogue is scripted to either arrest the player (< 5000 bounty) or attack (>= 5000 bounty)
                if (   player.getClass().getNpcStats(player).getBounty() >= cutoff
                       // TODO: do not run these two every frame. keep an Aware state for each actor and update it every 0.2 s or so?
                    && MWBase::Environment::get().getWorld()->getLOS(ptr, player)
                    && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, ptr))
                {
                    static const int iCrimeThresholdMultiplier = esmStore.get<ESM::GameSetting>().find("iCrimeThresholdMultiplier")->mValue.getInteger();
                    if (player.getClass().getNpcStats(player).getBounty() >= cutoff * iCrimeThresholdMultiplier)
                    {
                        MWBase::Environment::get().getMechanicsManager()->startCombat(ptr, player);
                        creatureStats.setHitAttemptActorId(player.getClass().getCreatureStats(player).getActorId()); // Stops the guard from quitting combat if player is unreachable
                    }
                    else
                        creatureStats.getAiSequence().stack(AiPursue(player), ptr);
                    creatureStats.setAlarmed(true);
                    npcStats.setCrimeId(MWBase::Environment::get().getWorld()->getPlayer().getNewCrimeId());
                }
            }

            // if I was a witness to a crime
            if (npcStats.getCrimeId() != -1)
            {
                // if you've paid for your crimes and I havent noticed
                if( npcStats.getCrimeId() <= MWBase::Environment::get().getWorld()->getPlayer().getCrimeId() )
                {
                    // Calm witness down
                    if (ptr.getClass().isClass(ptr, "Guard"))
                        creatureStats.getAiSequence().stopPursuit();
                    creatureStats.getAiSequence().stopCombat();

                    // Reset factors to attack
                    creatureStats.setAttacked(false);
                    creatureStats.setAlarmed(false);
                    creatureStats.setAiSetting(CreatureStats::AI_Fight, ptr.getClass().getBaseFightRating(ptr));

                    // Update witness crime id
                    npcStats.setCrimeId(-1);
                }
            }
        }
    }

    Actors::Actors() : mSmoothMovement(Settings::Manager::getBool("smooth movement", "Game"))
    {
        mTimerDisposeSummonsCorpses = 0.2f; // We should add a delay between summoned creature death and its corpse despawning

        updateProcessingRange();
    }

    Actors::~Actors()
    {
        clear();
    }

    float Actors::getProcessingRange() const
    {
        return mActorsProcessingRange;
    }

    void Actors::updateProcessingRange()
    {
        // We have to cap it since using high values (larger than 7168) will make some quests harder or impossible to complete (bug #1876)
        static const float maxProcessingRange = 7168.f;
        static const float minProcessingRange = maxProcessingRange / 2.f;

        float actorsProcessingRange = Settings::Manager::getFloat("actors processing range", "Game");
        actorsProcessingRange = std::min(actorsProcessingRange, maxProcessingRange);
        actorsProcessingRange = std::max(actorsProcessingRange, minProcessingRange);
        mActorsProcessingRange = actorsProcessingRange;
    }

    void Actors::addActor (const MWWorld::Ptr& ptr, bool updateImmediately)
    {
        removeActor(ptr);

        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (!anim)
            return;
        mActors.insert(std::make_pair(ptr, new Actor(ptr, anim)));

        CharacterController* ctrl = mActors[ptr]->getCharacterController();
        if (updateImmediately)
            ctrl->update(0);

        // We should initially hide actors outside of processing range.
        // Note: since we update player after other actors, distance will be incorrect during teleportation.
        // Do not update visibility if player was teleported, so actors will be visible during teleportation frame.
        if (MWBase::Environment::get().getWorld()->getPlayer().wasTeleported())
            return;

        updateVisibility(ptr, ctrl);
    }

    void Actors::updateVisibility (const MWWorld::Ptr& ptr, CharacterController* ctrl)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (ptr == player)
            return;

        const float dist = (player.getRefData().getPosition().asVec3() - ptr.getRefData().getPosition().asVec3()).length();
        if (dist > mActorsProcessingRange)
        {
            ptr.getRefData().getBaseNode()->setNodeMask(0);
            return;
        }
        else
            ptr.getRefData().getBaseNode()->setNodeMask(MWRender::Mask_Actor);

        // Fade away actors on large distance (>90% of actor's processing distance)
        float visibilityRatio = 1.0;
        float fadeStartDistance = mActorsProcessingRange*0.9f;
        float fadeEndDistance = mActorsProcessingRange;
        float fadeRatio = (dist - fadeStartDistance)/(fadeEndDistance - fadeStartDistance);
        if (fadeRatio > 0)
            visibilityRatio -= std::max(0.f, fadeRatio);

        visibilityRatio = std::min(1.f, visibilityRatio);

        ctrl->setVisibility(visibilityRatio);
    }

    void Actors::removeActor (const MWWorld::Ptr& ptr)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
        {
            delete iter->second;
            mActors.erase(iter);
        }
    }

    void Actors::castSpell(const MWWorld::Ptr& ptr, const std::string spellId, bool manualSpell)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second->getCharacterController()->castSpell(spellId, manualSpell);
    }

    bool Actors::isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer)
    {
        if (!actor.getClass().isActor())
            return false;

        // If an observer is NPC, check if he detected an actor
        if (!observer.isEmpty() && observer.getClass().isNpc())
        {
            return
                MWBase::Environment::get().getWorld()->getLOS(observer, actor) &&
                MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor, observer);
        }

        // Otherwise check if any actor in AI processing range sees the target actor
        std::vector<MWWorld::Ptr> neighbors;
        osg::Vec3f position (actor.getRefData().getPosition().asVec3());
        getObjectsInRange(position, mActorsProcessingRange, neighbors);
        for (const MWWorld::Ptr &neighbor : neighbors)
        {
            if (neighbor == actor)
                continue;

            bool result = MWBase::Environment::get().getWorld()->getLOS(neighbor, actor)
                       && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(actor, neighbor);

            if (result)
                return true;
        }

        return false;
    }

    void Actors::updateActor(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr)
    {
        PtrActorMap::iterator iter = mActors.find(old);
        if(iter != mActors.end())
        {
            Actor *actor = iter->second;
            mActors.erase(iter);

            actor->updatePtr(ptr);
            mActors.insert(std::make_pair(ptr, actor));
        }
    }

    void Actors::dropActors (const MWWorld::CellStore *cellStore, const MWWorld::Ptr& ignore)
    {
        PtrActorMap::iterator iter = mActors.begin();
        while(iter != mActors.end())
        {
            if((iter->first.isInCell() && iter->first.getCell()==cellStore) && iter->first != ignore)
            {
                delete iter->second;
                mActors.erase(iter++);
            }
            else
                ++iter;
        }
    }

    void Actors::updateCombatMusic ()
    {
        MWWorld::Ptr player = getPlayer();
        const osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();
        bool hasHostiles = false; // need to know this to play Battle music
        bool aiActive = MWBase::Environment::get().getMechanicsManager()->isAIActive();

        if (aiActive)
        {
            for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                if (iter->first == player) continue;

                bool inProcessingRange = (playerPos - iter->first.getRefData().getPosition().asVec3()).length2() <= mActorsProcessingRange*mActorsProcessingRange;
                if (inProcessingRange)
                {
                    MWMechanics::CreatureStats& stats = iter->first.getClass().getCreatureStats(iter->first);
                    if (!stats.isDead() && stats.getAiSequence().isInCombat())
                    {
                        hasHostiles = true;
                        break;
                    }
                }
            }
        }

        // check if we still have any player enemies to switch music
        static int currentMusic = 0;

        if (currentMusic != 1 && !hasHostiles && !(player.getClass().getCreatureStats(player).isDead() &&
        MWBase::Environment::get().getSoundManager()->isMusicPlaying()))
        {
            MWBase::Environment::get().getSoundManager()->playPlaylist(std::string("Explore"));
            currentMusic = 1;
        }
        else if (currentMusic != 2 && hasHostiles)
        {
            MWBase::Environment::get().getSoundManager()->playPlaylist(std::string("Battle"));
            currentMusic = 2;
        }

    }

    void Actors::predictAndAvoidCollisions(float duration)
    {
        if (!MWBase::Environment::get().getMechanicsManager()->isAIActive())
            return;

        const float minGap = 10.f;
        const float maxDistForPartialAvoiding = 200.f;
        const float maxDistForStrictAvoiding = 100.f;
        const float maxTimeToCheck = 2.0f;
        static const bool giveWayWhenIdle = Settings::Manager::getBool("NPCs give way", "Game");

        MWWorld::Ptr player = getPlayer();
        MWBase::World* world = MWBase::Environment::get().getWorld();
        for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
        {
            const MWWorld::Ptr& ptr = iter->first;
            if (ptr == player)
                continue; // Don't interfere with player controls.

            float maxSpeed = ptr.getClass().getMaxSpeed(ptr);
            if (maxSpeed == 0.0)
                continue; // Can't move, so there is no sense to predict collisions.

            Movement& movement = ptr.getClass().getMovementSettings(ptr);
            osg::Vec2f origMovement(movement.mPosition[0], movement.mPosition[1]);
            bool isMoving = origMovement.length2() > 0.01;
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
            for (const auto& package : aiSequence)
            {
                if (package->getTypeId() == AiPackageTypeId::Follow)
                    shouldAvoidCollision = true;
                else if (package->getTypeId() == AiPackageTypeId::Wander && giveWayWhenIdle)
                {
                    if (!static_cast<const AiWander*>(package.get())->isStationary())
                        shouldGiveWay = true;
                }
                else if (package->getTypeId() == AiPackageTypeId::Combat || package->getTypeId() == AiPackageTypeId::Pursue)
                {
                    currentTarget = package->getTarget();
                    shouldAvoidCollision = isMoving;
                    shouldTurnToApproachingActor = false;
                    break;
                }
            }
            if (!shouldAvoidCollision && !shouldGiveWay)
                continue;

            osg::Vec2f baseSpeed = origMovement * maxSpeed;
            osg::Vec3f basePos = ptr.getRefData().getPosition().asVec3();
            float baseRotZ = ptr.getRefData().getPosition().rot[2];
            osg::Vec3f halfExtents = world->getHalfExtents(ptr);
            float maxDistToCheck = isMoving ? maxDistForPartialAvoiding : maxDistForStrictAvoiding;

            float timeToCheck = maxTimeToCheck;
            if (!shouldGiveWay && !aiSequence.isEmpty())
                timeToCheck = std::min(timeToCheck, getTimeToDestination(**aiSequence.begin(), basePos, maxSpeed, duration, halfExtents));

            float timeToCollision = timeToCheck;
            osg::Vec2f movementCorrection(0, 0);
            float angleToApproachingActor = 0;

            // Iterate through all other actors and predict collisions.
            for(PtrActorMap::iterator otherIter(mActors.begin()); otherIter != mActors.end(); ++otherIter)
            {
                const MWWorld::Ptr& otherPtr = otherIter->first;
                if (otherPtr == ptr || otherPtr == currentTarget)
                    continue;

                osg::Vec3f otherHalfExtents = world->getHalfExtents(otherPtr);
                osg::Vec3f deltaPos = otherPtr.getRefData().getPosition().asVec3() - basePos;
                osg::Vec2f relPos = Misc::rotateVec2f(osg::Vec2f(deltaPos.x(), deltaPos.y()), baseRotZ);
                float dist = deltaPos.length();

                // Ignore actors which are not close enough or come from behind.
                if (dist > maxDistToCheck || relPos.y() < 0)
                    continue;

                // Don't check for a collision if vertical distance is greater then the actor's height.
                if (deltaPos.z() > halfExtents.z() * 2 || deltaPos.z() < -otherHalfExtents.z() * 2)
                    continue;

                osg::Vec3f speed = otherPtr.getClass().getMovementSettings(otherPtr).asVec3() *
                                   otherPtr.getClass().getMaxSpeed(otherPtr);
                float rotZ = otherPtr.getRefData().getPosition().rot[2];
                osg::Vec2f relSpeed = Misc::rotateVec2f(osg::Vec2f(speed.x(), speed.y()), baseRotZ - rotZ) - baseSpeed;

                float collisionDist = minGap + world->getHalfExtents(ptr).x() + world->getHalfExtents(otherPtr).x();
                collisionDist = std::min(collisionDist, relPos.length());

                // Find the earliest `t` when |relPos + relSpeed * t| == collisionDist.
                float vr = relPos.x() * relSpeed.x() + relPos.y() * relSpeed.y();
                float v2 = relSpeed.length2();
                float Dh = vr * vr - v2 * (relPos.length2() - collisionDist * collisionDist);
                if (Dh <= 0 || v2 == 0)
                    continue; // No solution; distance is always >= collisionDist.
                float t = (-vr - std::sqrt(Dh)) / v2;

                if (t < 0 || t > timeToCollision)
                    continue;

                // Check visibility and awareness last as it's expensive.
                if (!MWBase::Environment::get().getWorld()->getLOS(otherPtr, ptr))
                    continue;
                if (!MWBase::Environment::get().getMechanicsManager()->awarenessCheck(otherPtr, ptr))
                    continue;

                timeToCollision = t;
                angleToApproachingActor = std::atan2(deltaPos.x(), deltaPos.y());
                osg::Vec2f posAtT = relPos + relSpeed * t;
                float coef = (posAtT.x() * relSpeed.x() + posAtT.y() * relSpeed.y()) / (collisionDist * collisionDist * maxSpeed);
                coef *= osg::clampBetween((maxDistForPartialAvoiding - dist) / (maxDistForPartialAvoiding - maxDistForStrictAvoiding), 0.f, 1.f);
                movementCorrection = posAtT * coef;
                if (otherPtr.getClass().getCreatureStats(otherPtr).isDead())
                    // In case of dead body still try to go around (it looks natural), but reduce the correction twice.
                    movementCorrection.y() *= 0.5f;
            }

            if (timeToCollision < timeToCheck)
            {
                // Try to evade the nearest collision.
                osg::Vec2f newMovement = origMovement + movementCorrection;
                // Step to the side rather than backward. Otherwise player will be able to push the NPC far away from it's original location.
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

    void Actors::update (float duration, bool paused)
    {
        if(!paused)
        {
            static float timerUpdateHeadTrack = 0;
            static float timerUpdateEquippedLight = 0;
            static float timerUpdateHello = 0;
            const float updateEquippedLightInterval = 1.0f;

            if (timerUpdateHeadTrack >= 0.3f) timerUpdateHeadTrack = 0;
            if (timerUpdateHello >= 0.25f) timerUpdateHello = 0;
            if (mTimerDisposeSummonsCorpses >= 0.2f) mTimerDisposeSummonsCorpses = 0;
            if (timerUpdateEquippedLight >= updateEquippedLightInterval) timerUpdateEquippedLight = 0;

            // show torches only when there are darkness and no precipitations
            MWBase::World* world = MWBase::Environment::get().getWorld();
            bool showTorches = world->useTorches();

            MWWorld::Ptr player = getPlayer();
            const osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();

            /// \todo move update logic to Actor class where appropriate

            std::map<const MWWorld::Ptr, const std::set<MWWorld::Ptr> > cachedAllies; // will be filled as engageCombat iterates

            bool aiActive = MWBase::Environment::get().getMechanicsManager()->isAIActive();
            int attackedByPlayerId = player.getClass().getCreatureStats(player).getHitAttemptActorId();
            if (attackedByPlayerId != -1)
            {
                const MWWorld::Ptr playerHitAttemptActor = world->searchPtrViaActorId(attackedByPlayerId);

                if (!playerHitAttemptActor.isInCell())
                    player.getClass().getCreatureStats(player).setHitAttemptActorId(-1);
            }
            bool godmode = MWBase::Environment::get().getWorld()->getGodModeState();

             // AI and magic effects update
            for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                bool isPlayer = iter->first == player;
                CharacterController* ctrl = iter->second->getCharacterController();

                float distSqr = (playerPos - iter->first.getRefData().getPosition().asVec3()).length2();
                // AI processing is only done within given distance to the player.
                bool inProcessingRange = distSqr <= mActorsProcessingRange*mActorsProcessingRange;

                if (isPlayer)
                    ctrl->setAttackingOrSpell(world->getPlayer().getAttackingOrSpell());

                // If dead or no longer in combat, no longer store any actors who attempted to hit us. Also remove for the player.
                if (iter->first != player && (iter->first.getClass().getCreatureStats(iter->first).isDead()
                    || !iter->first.getClass().getCreatureStats(iter->first).getAiSequence().isInCombat()
                    || !inProcessingRange))
                {
                    iter->first.getClass().getCreatureStats(iter->first).setHitAttemptActorId(-1);
                    if (player.getClass().getCreatureStats(player).getHitAttemptActorId() == iter->first.getClass().getCreatureStats(iter->first).getActorId())
                        player.getClass().getCreatureStats(player).setHitAttemptActorId(-1);
                }

                iter->first.getClass().getCreatureStats(iter->first).getActiveSpells().update(duration);

                const Misc::TimerStatus engageCombatTimerStatus = iter->second->updateEngageCombatTimer(duration);

                // For dead actors we need to update looping spell particles
                if (iter->first.getClass().getCreatureStats(iter->first).isDead())
                {
                    // They can be added during the death animation
                    if (!iter->first.getClass().getCreatureStats(iter->first).isDeathAnimationFinished())
                        adjustMagicEffects(iter->first);
                    ctrl->updateContinuousVfx();
                }
                else
                {
                    bool cellChanged = world->hasCellChanged();
                    MWWorld::Ptr actor = iter->first; // make a copy of the map key to avoid it being invalidated when the player teleports
                    updateActor(actor, duration);

                    // Looping magic VFX update
                    // Note: we need to do this before any of the animations are updated.
                    // Reaching the text keys may trigger Hit / Spellcast (and as such, particles),
                    // so updating VFX immediately after that would just remove the particle effects instantly.
                    // There needs to be a magic effect update in between.
                    ctrl->updateContinuousVfx();

                    if (!cellChanged && world->hasCellChanged())
                    {
                        return; // for now abort update of the old cell when cell changes by teleportation magic effect
                                // a better solution might be to apply cell changes at the end of the frame
                    }
                    if (aiActive && inProcessingRange)
                    {
                        if (engageCombatTimerStatus == Misc::TimerStatus::Elapsed)
                        {
                            if (!isPlayer)
                                adjustCommandedActor(iter->first);

                            for(PtrActorMap::iterator it(mActors.begin()); it != mActors.end(); ++it)
                            {
                                if (it->first == iter->first || isPlayer) // player is not AI-controlled
                                    continue;
                                engageCombat(iter->first, it->first, cachedAllies, it->first == player);
                            }
                        }
                        if (timerUpdateHeadTrack == 0)
                        {
                            float sqrHeadTrackDistance = std::numeric_limits<float>::max();
                            MWWorld::Ptr headTrackTarget;

                            MWMechanics::CreatureStats& stats = iter->first.getClass().getCreatureStats(iter->first);
                            bool firstPersonPlayer = isPlayer && world->isFirstPerson();
                            bool inCombatOrPursue = stats.getAiSequence().isInCombat() || stats.getAiSequence().hasPackage(AiPackageTypeId::Pursue);
                            MWWorld::Ptr activePackageTarget;

                            // 1. Unconsious actor can not track target
                            // 2. Actors in combat and pursue mode do not bother to headtrack anyone except their target
                            // 3. Player character does not use headtracking in the 1st-person view
                            if (!stats.getKnockedDown() && !firstPersonPlayer)
                            {
                                if (inCombatOrPursue)
                                    activePackageTarget = stats.getAiSequence().getActivePackage().getTarget();

                                for (PtrActorMap::iterator it(mActors.begin()); it != mActors.end(); ++it)
                                {
                                    if (it->first == iter->first)
                                        continue;

                                    if (inCombatOrPursue && it->first != activePackageTarget)
                                        continue;

                                    updateHeadTracking(iter->first, it->first, headTrackTarget, sqrHeadTrackDistance, inCombatOrPursue);
                                }
                            }

                            ctrl->setHeadTrackTarget(headTrackTarget);
                        }

                        if (iter->first.getClass().isNpc() && iter->first != player)
                            updateCrimePursuit(iter->first, duration);

                        if (iter->first != player)
                        {
                            CreatureStats &stats = iter->first.getClass().getCreatureStats(iter->first);
                            if (isConscious(iter->first))
                            {
                                stats.getAiSequence().execute(iter->first, *ctrl, duration);
                                updateGreetingState(iter->first, *iter->second, timerUpdateHello > 0);
                                playIdleDialogue(iter->first);
                                updateMovementSpeed(iter->first);
                            }
                        }
                    }
                    else if (aiActive && iter->first != player && isConscious(iter->first))
                    {
                        CreatureStats &stats = iter->first.getClass().getCreatureStats(iter->first);
                        stats.getAiSequence().execute(iter->first, *ctrl, duration, /*outOfRange*/true);
                    }

                    if(iter->first.getClass().isNpc())
                    {
                        // We can not update drowning state for actors outside of AI distance - they can not resurface to breathe
                        if (inProcessingRange)
                            updateDrowning(iter->first, duration, ctrl->isKnockedOut(), isPlayer);

                        calculateNpcStatModifiers(iter->first, duration);

                        if (timerUpdateEquippedLight == 0)
                            updateEquippedLight(iter->first, updateEquippedLightInterval, showTorches);
                    }
                }
            }

            static const bool avoidCollisions = Settings::Manager::getBool("NPCs avoid collisions", "Game");
            if (avoidCollisions)
                predictAndAvoidCollisions(duration);

            timerUpdateHeadTrack += duration;
            timerUpdateEquippedLight += duration;
            timerUpdateHello += duration;
            mTimerDisposeSummonsCorpses += duration;

            // Animation/movement update
            CharacterController* playerCharacter = nullptr;
            for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                const float dist = (playerPos - iter->first.getRefData().getPosition().asVec3()).length();
                bool isPlayer = iter->first == player;
                CreatureStats &stats = iter->first.getClass().getCreatureStats(iter->first);
                // Actors with active AI should be able to move.
                bool alwaysActive = false;
                if (!isPlayer && isConscious(iter->first) && !stats.isParalyzed())
                {
                    MWMechanics::AiSequence& seq = stats.getAiSequence();
                    alwaysActive = !seq.isEmpty() && seq.getActivePackage().alwaysActive();
                }
                bool inRange = isPlayer || dist <= mActorsProcessingRange || alwaysActive;
                int activeFlag = 1; // Can be changed back to '2' to keep updating bounding boxes off screen (more accurate, but slower)
                if (isPlayer)
                    activeFlag = 2;
                int active = inRange ? activeFlag : 0;

                CharacterController* ctrl = iter->second->getCharacterController();
                ctrl->setActive(active);

                if (!inRange)
                {
                    iter->first.getRefData().getBaseNode()->setNodeMask(0);
                    world->setActorCollisionMode(iter->first, false, false);
                    continue;
                }
                else if (!isPlayer)
                    iter->first.getRefData().getBaseNode()->setNodeMask(MWRender::Mask_Actor);

                const bool isDead = iter->first.getClass().getCreatureStats(iter->first).isDead();
                if (!isDead && (!godmode || !isPlayer) && iter->first.getClass().getCreatureStats(iter->first).isParalyzed())
                    ctrl->skipAnim();

                // Handle player last, in case a cell transition occurs by casting a teleportation spell
                // (would invalidate the iterator)
                if (iter->first == getPlayer())
                {
                    playerCharacter = ctrl;
                    continue;
                }

                world->setActorCollisionMode(iter->first, true, !iter->first.getClass().getCreatureStats(iter->first).isDeathAnimationFinished());
                ctrl->update(duration);

                updateVisibility(iter->first, ctrl);
            }

            if (playerCharacter)
            {
                playerCharacter->update(duration);
                playerCharacter->setVisibility(1.f);
            }

            for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
            {
                const MWWorld::Class &cls = iter->first.getClass();
                CreatureStats &stats = cls.getCreatureStats(iter->first);

                //KnockedOutOneFrameLogic
                //Used for "OnKnockedOut" command
                //Put here to ensure that it's run for PRECISELY one frame.
                if (stats.getKnockedDown() && !stats.getKnockedDownOneFrame() && !stats.getKnockedDownOverOneFrame())
                { //Start it for one frame if nessesary
                    stats.setKnockedDownOneFrame(true);
                }
                else if (stats.getKnockedDownOneFrame() && !stats.getKnockedDownOverOneFrame())
                { //Turn off KnockedOutOneframe
                    stats.setKnockedDownOneFrame(false);
                    stats.setKnockedDownOverOneFrame(true);
                }
            }

            killDeadActors();
            updateSneaking(playerCharacter, duration);
        }

        updateCombatMusic();
    }

    void Actors::notifyDied(const MWWorld::Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).notifyDied();

        ++mDeathCount[Misc::StringUtils::lowerCase(actor.getCellRef().getRefId())];
    }

    void Actors::resurrect(const MWWorld::Ptr &ptr)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
        {
            if(iter->second->getCharacterController()->isDead())
            {
                // Actor has been resurrected. Notify the CharacterController and re-enable collision.
                MWBase::Environment::get().getWorld()->enableActorCollision(iter->first, true);
                iter->second->getCharacterController()->resurrect();
            }
        }
    }

    void Actors::killDeadActors()
    {
        for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
        {
            const MWWorld::Class &cls = iter->first.getClass();
            CreatureStats &stats = cls.getCreatureStats(iter->first);

            if(!stats.isDead())
                continue;

            MWBase::Environment::get().getWorld()->removeActorPath(iter->first);
            CharacterController::KillResult killResult = iter->second->getCharacterController()->kill();
            if (killResult == CharacterController::Result_DeathAnimStarted)
            {
                // Play dying words
                // Note: It's not known whether the soundgen tags scream, roar, and moan are reliable
                // for NPCs since some of the npc death animation files are missing them.
                MWBase::Environment::get().getDialogueManager()->say(iter->first, "hit");

                // Apply soultrap
                if (iter->first.getTypeName() == typeid(ESM::Creature).name())
                {
                    SoulTrap soulTrap (iter->first);
                    stats.getActiveSpells().visitEffectSources(soulTrap);
                }

                // Magic effects will be reset later, and the magic effect that could kill the actor
                // needs to be determined now
                calculateCreatureStatModifiers(iter->first, 0);

                if (cls.isEssential(iter->first))
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sKilledEssential}");
            }
            else if (killResult == CharacterController::Result_DeathAnimJustFinished)
            {
                bool isPlayer = iter->first == getPlayer();
                notifyDied(iter->first);

                // Reset magic effects and recalculate derived effects
                // One case where we need this is to make sure bound items are removed upon death
                stats.modifyMagicEffects(MWMechanics::MagicEffects());
                stats.getActiveSpells().clear();
                // Make sure spell effects are removed
                purgeSpellEffects(stats.getActorId());

                // Reset dynamic stats, attributes and skills
                calculateCreatureStatModifiers(iter->first, 0);
                if (iter->first.getClass().isNpc())
                    calculateNpcStatModifiers(iter->first, 0);

                if (isPlayer)
                {
                    //player's death animation is over
                    MWBase::Environment::get().getStateManager()->askLoadRecent();
                }
                else
                {
                    // NPC death animation is over, disable actor collision
                    MWBase::Environment::get().getWorld()->enableActorCollision(iter->first, false);
                }

                // Play Death Music if it was the player dying
                if(iter->first == getPlayer())
                    MWBase::Environment::get().getSoundManager()->streamMusic("Special/MW_Death.mp3");
            }
        }
    }

    void Actors::cleanupSummonedCreature (MWMechanics::CreatureStats& casterStats, int creatureActorId)
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaActorId(creatureActorId);
        if (!ptr.isEmpty())
        {
            MWBase::Environment::get().getWorld()->deleteObject(ptr);

            const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                    .search("VFX_Summon_End");
            if (fx)
                MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + fx->mModel,
                    "", ptr.getRefData().getPosition().asVec3());

            // Remove the summoned creature's summoned creatures as well
            MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
            std::map<ESM::SummonKey, int>& creatureMap = stats.getSummonedCreatureMap();
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

    void Actors::purgeSpellEffects(int casterActorId)
    {
        for (PtrActorMap::iterator iter(mActors.begin());iter != mActors.end();++iter)
        {
            MWMechanics::ActiveSpells& spells = iter->first.getClass().getCreatureStats(iter->first).getActiveSpells();
            spells.purge(casterActorId);
        }
    }

    void Actors::rest(double hours, bool sleep)
    {
        float duration = hours * 3600.f;
        float timeScale = MWBase::Environment::get().getWorld()->getTimeScaleFactor();
        if (timeScale != 0.f)
            duration /= timeScale;

        const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();

        for(PtrActorMap::iterator iter(mActors.begin()); iter != mActors.end(); ++iter)
        {
            if (iter->first.getClass().getCreatureStats(iter->first).isDead())
            {
                iter->first.getClass().getCreatureStats(iter->first).getActiveSpells().update(duration);
                continue;
            }

            if (!sleep || iter->first == player)
                restoreDynamicStats(iter->first, hours, sleep);

            if ((!iter->first.getRefData().getBaseNode()) ||
                    (playerPos - iter->first.getRefData().getPosition().asVec3()).length2() > mActorsProcessingRange*mActorsProcessingRange)
                continue;

            adjustMagicEffects (iter->first);
            if (iter->first.getClass().getCreatureStats(iter->first).needToRecalcDynamicStats())
                calculateDynamicStats (iter->first);

            calculateCreatureStatModifiers (iter->first, duration);
            if (iter->first.getClass().isNpc())
                calculateNpcStatModifiers(iter->first, duration);

            iter->first.getClass().getCreatureStats(iter->first).getActiveSpells().update(duration);

            MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(iter->first);
            if (animation)
            {
                animation->removeEffects();
                MWBase::Environment::get().getWorld()->applyLoopingParticles(iter->first);
            }
        }

        fastForwardAi();
    }

    void Actors::updateSneaking(CharacterController* ctrl, float duration)
    {
        static float sneakTimer = 0.f; // Times update of sneak icon

        if (!ctrl)
        {
            MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
            return;
        }

        MWWorld::Ptr player = getPlayer();

        if (!MWBase::Environment::get().getMechanicsManager()->isSneaking(player))
        {
            MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
            return;
        }

        static float sneakSkillTimer = 0.f; // Times sneak skill progress from "avoid notice"

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();
        static const float fSneakUseDist = gmst.find("fSneakUseDist")->mValue.getFloat();
        static const float fSneakUseDelay = gmst.find("fSneakUseDelay")->mValue.getFloat();

        if (sneakTimer >= fSneakUseDelay)
            sneakTimer = 0.f;

        if (sneakTimer == 0.f)
        {
            // Set when an NPC is within line of sight and distance, but is still unaware. Used for skill progress.
            bool avoidedNotice = false;
            bool detected = false;

            std::vector<MWWorld::Ptr> observers;
            osg::Vec3f position(player.getRefData().getPosition().asVec3());
            float radius = std::min(fSneakUseDist, mActorsProcessingRange);
            getObjectsInRange(position, radius, observers);

            for (const MWWorld::Ptr &observer : observers)
            {
                if (observer == player || observer.getClass().getCreatureStats(observer).isDead())
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

            if (sneakSkillTimer >= fSneakUseDelay)
                sneakSkillTimer = 0.f;

            if (avoidedNotice && sneakSkillTimer == 0.f)
                player.getClass().skillUsageSucceeded(player, ESM::Skill::Sneak, 0);

            if (!detected)
                MWBase::Environment::get().getWindowManager()->setSneakVisibility(true);
        }

        sneakTimer += duration;
        sneakSkillTimer += duration;
    }

    int Actors::getHoursToRest(const MWWorld::Ptr &ptr) const
    {
        float healthPerHour, magickaPerHour;
        getRestorationPerHourOfSleep(ptr, healthPerHour, magickaPerHour);

        CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
        bool stunted = stats.getMagicEffects ().get(ESM::MagicEffect::StuntedMagicka).getMagnitude() > 0;

        float healthHours  = healthPerHour > 0
                             ? (stats.getHealth().getModified() - stats.getHealth().getCurrent()) / healthPerHour
                             : 1.0f;
        float magickaHours = magickaPerHour > 0 && !stunted
                              ? (stats.getMagicka().getModified() - stats.getMagicka().getCurrent()) / magickaPerHour
                              : 1.0f;

        int autoHours = static_cast<int>(std::ceil(std::max(1.f, std::max(healthHours, magickaHours))));
        return autoHours;
    }

    int Actors::countDeaths (const std::string& id) const
    {
        std::map<std::string, int>::const_iterator iter = mDeathCount.find(id);
        if(iter != mDeathCount.end())
            return iter->second;
        return 0;
    }

    void Actors::forceStateUpdate(const MWWorld::Ptr & ptr)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second->getCharacterController()->forceStateUpdate();
    }

    bool Actors::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number, bool persist)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
        {
            return iter->second->getCharacterController()->playGroup(groupName, mode, number, persist);
        }
        else
        {
            Log(Debug::Warning) << "Warning: Actors::playAnimationGroup: Unable to find " << ptr.getCellRef().getRefId();
            return false;
        }
    }
    void Actors::skipAnimation(const MWWorld::Ptr& ptr)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            iter->second->getCharacterController()->skipAnim();
    }

    bool Actors::checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string& groupName)
    {
        PtrActorMap::iterator iter = mActors.find(ptr);
        if(iter != mActors.end())
            return iter->second->getCharacterController()->isAnimPlaying(groupName);
        return false;
    }

    void Actors::persistAnimationStates()
    {
        for (PtrActorMap::iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
            iter->second->getCharacterController()->persistAnimationState();
    }

    void Actors::getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out)
    {
        for (PtrActorMap::iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
        {
            if ((iter->first.getRefData().getPosition().asVec3() - position).length2() <= radius*radius)
                out.push_back(iter->first);
        }
    }

    bool Actors::isAnyObjectInRange(const osg::Vec3f& position, float radius)
    {
        for (PtrActorMap::iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
        {
            if ((iter->first.getRefData().getPosition().asVec3() - position).length2() <= radius*radius)
                return true;
        }

        return false;
    }

    std::list<MWWorld::Ptr> Actors::getActorsSidingWith(const MWWorld::Ptr& actor)
    {
        std::list<MWWorld::Ptr> list;
        for(PtrActorMap::iterator iter = mActors.begin(); iter != mActors.end(); ++iter)
        {
            const MWWorld::Ptr &iteratedActor = iter->first;
            if (iteratedActor == getPlayer())
                continue;

            const bool sameActor = (iteratedActor == actor);

            const CreatureStats &stats = iteratedActor.getClass().getCreatureStats(iteratedActor);
            if (stats.isDead())
                continue;

            // An actor counts as siding with this actor if Follow or Escort is the current AI package, or there are only Combat and Wander packages before the Follow/Escort package
            // Actors that are targeted by this actor's Follow or Escort packages also side with them
            for (const auto& package : stats.getAiSequence())
            {
                if (package->sideWithTarget() && !package->getTarget().isEmpty())
                {
                    if (sameActor)
                    {
                        list.push_back(package->getTarget());
                    }
                    else if (package->getTarget() == actor)
                    {
                        list.push_back(iteratedActor);
                    }
                    break;
                }
                else if (package->getTypeId() != AiPackageTypeId::Combat && package->getTypeId() != AiPackageTypeId::Wander)
                    break;
            }
        }
        return list;
    }

    std::list<MWWorld::Ptr> Actors::getActorsFollowing(const MWWorld::Ptr& actor)
    {
        std::list<MWWorld::Ptr> list;
        forEachFollowingPackage(mActors, actor, getPlayer(), [&] (auto& iter, const std::unique_ptr<AiPackage>& package)
        {
            if (package->followTargetThroughDoors() && package->getTarget() == actor)
                list.push_back(iter.first);
            else if (package->getTypeId() != AiPackageTypeId::Combat && package->getTypeId() != AiPackageTypeId::Wander)
                return false;
            return true;
        });
        return list;
    }

    void Actors::getActorsFollowing(const MWWorld::Ptr &actor, std::set<MWWorld::Ptr>& out) {
        std::list<MWWorld::Ptr> followers = getActorsFollowing(actor);
        for(const MWWorld::Ptr &follower : followers)
            if (out.insert(follower).second)
                getActorsFollowing(follower, out);
    }

    void Actors::getActorsSidingWith(const MWWorld::Ptr &actor, std::set<MWWorld::Ptr>& out) {
        std::list<MWWorld::Ptr> followers = getActorsSidingWith(actor);
        for(const MWWorld::Ptr &follower : followers)
            if (out.insert(follower).second)
                getActorsSidingWith(follower, out);
    }

    void Actors::getActorsSidingWith(const MWWorld::Ptr &actor, std::set<MWWorld::Ptr>& out, std::map<const MWWorld::Ptr, const std::set<MWWorld::Ptr> >& cachedAllies) {
        // If we have already found actor's allies, use the cache
        std::map<const MWWorld::Ptr, const std::set<MWWorld::Ptr> >::const_iterator search = cachedAllies.find(actor);
        if (search != cachedAllies.end())
            out.insert(search->second.begin(), search->second.end());
        else
        {
            std::list<MWWorld::Ptr> followers = getActorsSidingWith(actor);
            for (const MWWorld::Ptr &follower : followers)
                if (out.insert(follower).second)
                    getActorsSidingWith(follower, out, cachedAllies);

            // Cache ptrs and their sets of allies
            cachedAllies.insert(std::make_pair(actor, out));
            for (const MWWorld::Ptr &iter : out)
            {
                search = cachedAllies.find(iter);
                if (search == cachedAllies.end())
                    cachedAllies.insert(std::make_pair(iter, out));
            }
        }
    }

    std::list<int> Actors::getActorsFollowingIndices(const MWWorld::Ptr &actor)
    {
        std::list<int> list;
        forEachFollowingPackage(mActors, actor, getPlayer(), [&] (auto& iter, const std::unique_ptr<AiPackage>& package)
        {
            if (package->followTargetThroughDoors() && package->getTarget() == actor)
            {
                list.push_back(static_cast<const AiFollow*>(package.get())->getFollowIndex());
                return false;
            }
            else if (package->getTypeId() != AiPackageTypeId::Combat && package->getTypeId() != AiPackageTypeId::Wander)
                return false;
            return true;
        });
        return list;
    }

    std::map<int, MWWorld::Ptr> Actors::getActorsFollowingByIndex(const MWWorld::Ptr &actor)
    {
        std::map<int, MWWorld::Ptr> map;
        forEachFollowingPackage(mActors, actor, getPlayer(), [&] (auto& iter, const std::unique_ptr<AiPackage>& package)
        {
            if (package->followTargetThroughDoors() && package->getTarget() == actor)
            {
                int index = static_cast<const AiFollow*>(package.get())->getFollowIndex();
                map[index] = iter.first;
                return false;
            }
            else if (package->getTypeId() != AiPackageTypeId::Combat && package->getTypeId() != AiPackageTypeId::Wander)
                return false;
            return true;
        });
        return map;
    }

    std::list<MWWorld::Ptr> Actors::getActorsFighting(const MWWorld::Ptr& actor) {
        std::list<MWWorld::Ptr> list;
        std::vector<MWWorld::Ptr> neighbors;
        osg::Vec3f position (actor.getRefData().getPosition().asVec3());
        getObjectsInRange(position, mActorsProcessingRange, neighbors);
        for(const MWWorld::Ptr& neighbor : neighbors)
        {
            if (neighbor == actor)
                continue;

            const CreatureStats &stats = neighbor.getClass().getCreatureStats(neighbor);
            if (stats.isDead())
                continue;

            if (stats.getAiSequence().isInCombat(actor))
                list.push_front(neighbor);
        }
        return list;
    }

    std::list<MWWorld::Ptr> Actors::getEnemiesNearby(const MWWorld::Ptr& actor)
    {
        std::list<MWWorld::Ptr> list;
        std::vector<MWWorld::Ptr> neighbors;
        osg::Vec3f position (actor.getRefData().getPosition().asVec3());
        getObjectsInRange(position, mActorsProcessingRange, neighbors);

        std::set<MWWorld::Ptr> followers;
        getActorsFollowing(actor, followers);
        for (auto neighbor = neighbors.begin(); neighbor != neighbors.end(); ++neighbor)
        {
            const CreatureStats &stats = neighbor->getClass().getCreatureStats(*neighbor);
            if (stats.isDead() || *neighbor == actor || neighbor->getClass().isPureWaterCreature(*neighbor))
                continue;

            const bool isFollower = followers.find(*neighbor) != followers.end();

            if (stats.getAiSequence().isInCombat(actor) || (MWBase::Environment::get().getMechanicsManager()->isAggressive(*neighbor, actor) && !isFollower))
                list.push_back(*neighbor);
        }
        return list;
    }


    void Actors::write (ESM::ESMWriter& writer, Loading::Listener& listener) const
    {
        writer.startRecord(ESM::REC_DCOU);
        for (std::map<std::string, int>::const_iterator it = mDeathCount.begin(); it != mDeathCount.end(); ++it)
        {
            writer.writeHNString("ID__", it->first);
            writer.writeHNT ("COUN", it->second);
        }
        writer.endRecord(ESM::REC_DCOU);
    }

    void Actors::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_DCOU)
        {
            while (reader.isNextSub("ID__"))
            {
                std::string id = reader.getHString();
                int count;
                reader.getHNT(count, "COUN");
                if (MWBase::Environment::get().getWorld()->getStore().find(id))
                    mDeathCount[id] = count;
            }
        }
    }

    void Actors::clear()
    {
        PtrActorMap::iterator it(mActors.begin());
        for (; it != mActors.end(); ++it)
        {
            delete it->second;
            it->second = nullptr;
        }
        mActors.clear();
        mDeathCount.clear();
    }

    void Actors::updateMagicEffects(const MWWorld::Ptr &ptr)
    {
        adjustMagicEffects(ptr);
        calculateCreatureStatModifiers(ptr, 0.f);
        if (ptr.getClass().isNpc())
            calculateNpcStatModifiers(ptr, 0.f);
    }

    bool Actors::isReadyToBlock(const MWWorld::Ptr &ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;

        return it->second->getCharacterController()->isReadyToBlock();
    }

    bool Actors::isCastingSpell(const MWWorld::Ptr &ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;

        return it->second->getCharacterController()->isCastingSpell();
    }

    bool Actors::isAttackingOrSpell(const MWWorld::Ptr& ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;
        CharacterController* ctrl = it->second->getCharacterController();

        return ctrl->isAttackingOrSpell();
    }

    int Actors::getGreetingTimer(const MWWorld::Ptr& ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return 0;

        return it->second->getGreetingTimer();
    }

    float Actors::getAngleToPlayer(const MWWorld::Ptr& ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return 0.f;

        return it->second->getAngleToPlayer();
    }

    GreetingState Actors::getGreetingState(const MWWorld::Ptr& ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return Greet_None;

        return it->second->getGreetingState();
    }

    bool Actors::isTurningToPlayer(const MWWorld::Ptr& ptr) const
    {
        PtrActorMap::const_iterator it = mActors.find(ptr);
        if (it == mActors.end())
            return false;

        return it->second->isTurningToPlayer();
    }

    void Actors::fastForwardAi()
    {
        if (!MWBase::Environment::get().getMechanicsManager()->isAIActive())
            return;

        // making a copy since fast-forward could move actor to a different cell and invalidate the mActors iterator
        PtrActorMap map = mActors;
        for (PtrActorMap::iterator it = map.begin(); it != map.end(); ++it)
        {
            MWWorld::Ptr ptr = it->first;
            if (ptr == getPlayer()
                    || !isConscious(ptr)
                    || ptr.getClass().getCreatureStats(ptr).isParalyzed())
                continue;
            MWMechanics::AiSequence& seq = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            seq.fastForward(ptr);
        }
    }
}
