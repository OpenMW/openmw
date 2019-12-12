#ifndef GAME_MWMECHANICS_CREATURESTATS_H
#define GAME_MWMECHANICS_CREATURESTATS_H

#include <set>
#include <string>
#include <stdexcept>

#include "stat.hpp"
#include "magiceffects.hpp"
#include "spells.hpp"
#include "activespells.hpp"
#include "aisequence.hpp"
#include "drawstate.hpp"

namespace ESM
{
    struct CreatureStats;
}

namespace MWMechanics
{
    enum GreetingState
    {
        Greet_None,
        Greet_InProgress,
        Greet_Done
    };

    /// \brief Common creature stats
    ///
    ///
    class CreatureStats
    {
        static int sActorId;
        DrawState_ mDrawState;
        AttributeValue mAttributes[8];
        DynamicStat<float> mDynamic[3]; // health, magicka, fatigue
        Spells mSpells;
        ActiveSpells mActiveSpells;
        MagicEffects mMagicEffects;
        Stat<int> mAiSettings[4];
        AiSequence mAiSequence;
        bool mDead;
        bool mDeathAnimationFinished;
        bool mDied; // flag for OnDeath script function
        bool mMurdered;
        int mFriendlyHits;
        bool mTalkedTo;
        bool mAlarmed;
        bool mAttacked;
        bool mKnockdown;
        bool mKnockdownOneFrame;
        bool mKnockdownOverOneFrame;
        bool mHitRecovery;
        bool mBlock;
        unsigned int mMovementFlags;

        float mFallHeight;

        std::string mLastHitObject; // The last object to hit this actor
        std::string mLastHitAttemptObject; // The last object to attempt to hit this actor

        bool mRecalcMagicka;

        // For merchants: the last time items were restocked and gold pool refilled.
        MWWorld::TimeStamp mLastRestock;

        // The pool of merchant gold (not in inventory)
        int mGoldPool;

        int mActorId;
        int mHitAttemptActorId; // Stores an actor that attacked this actor. Only one is stored at a time,
                                // and it is not changed if a different actor attacks. It is cleared when combat ends.

        // The index of the death animation that was played, or -1 if none played
        signed char mDeathAnimation;

        MWWorld::TimeStamp mTimeOfDeath;

        GreetingState mGreetingState;
        int mGreetingTimer;
        float mTargetAngleRadians;
        bool mIsTurningToPlayer;

    public:
        typedef std::pair<int, std::string> SummonKey; // <ESM::MagicEffect index, spell ID>
    private:
        std::map<SummonKey, int> mSummonedCreatures; // <SummonKey, ActorId>

        // Contains ActorIds of summoned creatures with an expired lifetime that have not been deleted yet.
        // This may be necessary when the creature is in an inactive cell.
        std::vector<int> mSummonGraveyard;

    protected:
        int mLevel;

    public:
        CreatureStats();

        int getGreetingTimer() const;
        void setGreetingTimer(int timer);

        float getAngleToPlayer() const;
        void setAngleToPlayer(float angle);

        GreetingState getGreetingState() const;
        void setGreetingState(GreetingState state);

        bool isTurningToPlayer() const;
        void setTurningToPlayer(bool turning);

        DrawState_ getDrawState() const;
        void setDrawState(DrawState_ state);

        bool needToRecalcDynamicStats();
        void setNeedRecalcDynamicStats(bool val);

        float getFallHeight() const;
        void addToFallHeight(float height);

        /// Reset the fall height
        /// @return total fall height
        float land(bool isPlayer=false);

        const AttributeValue & getAttribute(int index) const;

        const DynamicStat<float> & getHealth() const;

        const DynamicStat<float> & getMagicka() const;

        const DynamicStat<float> & getFatigue() const;

        const DynamicStat<float> & getDynamic (int index) const;

        const Spells & getSpells() const;

        const ActiveSpells & getActiveSpells() const;

        const MagicEffects & getMagicEffects() const;

        bool getAttackingOrSpell() const;

        int getLevel() const;

        Spells & getSpells();

        ActiveSpells & getActiveSpells();

        MagicEffects & getMagicEffects();

        void setAttribute(int index, const AttributeValue &value);
        // Shortcut to set only the base
        void setAttribute(int index, int base);

        void setHealth(const DynamicStat<float> &value);

        void setMagicka(const DynamicStat<float> &value);

        void setFatigue(const DynamicStat<float> &value);

        void setDynamic (int index, const DynamicStat<float> &value);

        /// Set Modifier for each magic effect according to \a effects. Does not touch Base values.
        void modifyMagicEffects(const MagicEffects &effects);

        void setAttackingOrSpell(bool attackingOrSpell);

        void setLevel(int level);

        enum AiSetting
        {
            AI_Hello = 0,
            AI_Fight = 1,
            AI_Flee = 2,
            AI_Alarm = 3
        };
        void setAiSetting (AiSetting index, Stat<int> value);
        void setAiSetting (AiSetting index, int base);
        Stat<int> getAiSetting (AiSetting index) const;

        const AiSequence& getAiSequence() const;

        AiSequence& getAiSequence();

        float getFatigueTerm() const;
        ///< Return effective fatigue

        bool isParalyzed() const;

        bool isDead() const;

        bool isDeathAnimationFinished() const;
        void setDeathAnimationFinished(bool finished);

        void notifyDied();

        bool hasDied() const;

        void clearHasDied();

        bool hasBeenMurdered() const;

        void clearHasBeenMurdered();

        void notifyMurder();

        void resurrect();

        bool hasCommonDisease() const;

        bool hasBlightDisease() const;

        int getFriendlyHits() const;
        ///< Number of friendly hits received.

        void friendlyHit();
        ///< Increase number of friendly hits by one.

        bool hasTalkedToPlayer() const;
        ///< Has this creature talked with the player before?

        void talkedToPlayer();

        bool isAlarmed() const;
        void setAlarmed (bool alarmed);

        bool getAttacked() const;
        void setAttacked (bool attacked);

        float getEvasion() const;

        void setKnockedDown(bool value);
        /// Returns true for the entire duration of the actor being knocked down or knocked out,
        /// including transition animations (falling down & standing up)
        bool getKnockedDown() const;
        void setKnockedDownOneFrame(bool value);
        ///Returns true only for the first frame of the actor being knocked out; used for "onKnockedOut" command
        bool getKnockedDownOneFrame() const;
        void setKnockedDownOverOneFrame(bool value);
        ///Returns true for all but the first frame of being knocked out; used to know to not reset mKnockedDownOneFrame
        bool getKnockedDownOverOneFrame() const;
        void setHitRecovery(bool value);
        bool getHitRecovery() const;
        void setBlock(bool value);
        bool getBlock() const;

        std::map<SummonKey, int>& getSummonedCreatureMap(); // <SummonKey, ActorId of summoned creature>
        std::vector<int>& getSummonedCreatureGraveyard(); // ActorIds

        enum Flag
        {
            Flag_ForceRun = 1,
            Flag_ForceSneak = 2,
            Flag_Run = 4,
            Flag_Sneak = 8,
            Flag_ForceJump = 16,
            Flag_ForceMoveJump = 32
        };
        enum Stance
        {
            Stance_Run,
            Stance_Sneak
        };

        bool getMovementFlag (Flag flag) const;
        void setMovementFlag (Flag flag, bool state);
        /// Like getMovementFlag, but also takes into account if the flag is Forced
        bool getStance (Stance flag) const;

        void setLastHitObject(const std::string &objectid);
        const std::string &getLastHitObject() const;
        void setLastHitAttemptObject(const std::string &objectid);
        const std::string &getLastHitAttemptObject() const;
        void setHitAttemptActorId(const int actorId);
        int getHitAttemptActorId() const;

        // Note, this is just a cache to avoid checking the whole container store every frame. We don't need to store it in saves.
        // TODO: Put it somewhere else?
        std::set<int> mBoundItems;

        void writeState (ESM::CreatureStats& state) const;

        void readState (const ESM::CreatureStats& state);

        static void writeActorIdCounter (ESM::ESMWriter& esm);
        static void readActorIdCounter (ESM::ESMReader& esm);

        void setLastRestockTime(MWWorld::TimeStamp tradeTime);
        MWWorld::TimeStamp getLastRestockTime() const;

        void setGoldPool(int pool);
        int getGoldPool() const;

        signed char getDeathAnimation() const; // -1 means not decided
        void setDeathAnimation(signed char index);

        MWWorld::TimeStamp getTimeOfDeath() const;

        int getActorId();
        ///< Will generate an actor ID, if the actor does not have one yet.

        bool matchesActorId (int id) const;
        ///< Check if \a id matches the actor ID of *this (if the actor does not have an ID
        /// assigned this function will return false).

        static void cleanup();
    };
}

#endif
