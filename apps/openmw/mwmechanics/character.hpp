#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include <deque>

#include <components/esm/loadmgef.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwrender/animation.hpp"

namespace MWWorld
{
    class InventoryStore;
}

namespace MWRender
{
    class Animation;
}

namespace MWMechanics
{

struct Movement;
class CreatureStats;

enum Priority {
    Priority_Default,
    Priority_WeaponLowerBody,
    Priority_SneakIdleLowerBody,
    Priority_SwimIdle,
    Priority_Jump,
    Priority_Movement,
    Priority_Hit,
    Priority_Weapon,
    Priority_Block,
    Priority_Knockdown,
    Priority_Torch,
    Priority_Storm,

    Priority_Death,

    Num_Priorities
};

enum CharacterState {
    CharState_None,

    CharState_SpecialIdle,
    CharState_Idle,
    CharState_Idle2,
    CharState_Idle3,
    CharState_Idle4,
    CharState_Idle5,
    CharState_Idle6,
    CharState_Idle7,
    CharState_Idle8,
    CharState_Idle9,
    CharState_IdleSwim,
    CharState_IdleSneak,

    CharState_WalkForward,
    CharState_WalkBack,
    CharState_WalkLeft,
    CharState_WalkRight,

    CharState_SwimWalkForward,
    CharState_SwimWalkBack,
    CharState_SwimWalkLeft,
    CharState_SwimWalkRight,

    CharState_RunForward,
    CharState_RunBack,
    CharState_RunLeft,
    CharState_RunRight,

    CharState_SwimRunForward,
    CharState_SwimRunBack,
    CharState_SwimRunLeft,
    CharState_SwimRunRight,

    CharState_SneakForward,
    CharState_SneakBack,
    CharState_SneakLeft,
    CharState_SneakRight,

    CharState_TurnLeft,
    CharState_TurnRight,

    CharState_Jump,

    CharState_Death1,
    CharState_Death2,
    CharState_Death3,
    CharState_Death4,
    CharState_Death5,
    CharState_SwimDeath,
    CharState_DeathKnockDown,
    CharState_DeathKnockOut,

    CharState_Hit,
    CharState_KnockDown,
    CharState_KnockOut,
    CharState_Block
};

enum WeaponType {
    WeapType_None,

    WeapType_HandToHand,
    WeapType_OneHand,
    WeapType_TwoHand,
    WeapType_TwoWide,
    WeapType_BowAndArrow,
    WeapType_Crossbow,
    WeapType_Thrown,
    WeapType_PickProbe,

    WeapType_Spell
};

enum UpperBodyCharacterState {
    UpperCharState_Nothing,
    UpperCharState_EquipingWeap,
    UpperCharState_UnEquipingWeap,
    UpperCharState_WeapEquiped,
    UpperCharState_StartToMinAttack,
    UpperCharState_MinAttackToMaxAttack,
    UpperCharState_MaxAttackToMinHit,
    UpperCharState_MinHitToHit,
    UpperCharState_FollowStartToFollowStop,
    UpperCharState_CastingSpell
};

enum JumpingState {
    JumpState_None,
    JumpState_InAir,
    JumpState_Landing
};

struct WeaponInfo;

class CharacterController : public MWRender::Animation::TextKeyListener
{
    MWWorld::Ptr mPtr;
    MWRender::Animation *mAnimation;
    
    struct AnimationQueueEntry
    {
        std::string mGroup;
        size_t mLoopCount;
        bool mPersist;
    };
    typedef std::deque<AnimationQueueEntry> AnimationQueue;
    AnimationQueue mAnimQueue;

    CharacterState mIdleState;
    std::string mCurrentIdle;

    CharacterState mMovementState;
    std::string mCurrentMovement;
    float mMovementAnimSpeed;
    bool mAdjustMovementAnimSpeed;
    bool mHasMovedInXY;
    bool mMovementAnimationControlled;

    CharacterState mDeathState;
    std::string mCurrentDeath;
    bool mFloatToSurface;

    CharacterState mHitState;
    std::string mCurrentHit;

    UpperBodyCharacterState mUpperBodyState;

    JumpingState mJumpState;
    std::string mCurrentJump;

    WeaponType mWeaponType;
    std::string mCurrentWeapon;

    float mAttackStrength;

    bool mSkipAnim;

    // counted for skill increase
    float mSecondsOfSwimming;
    float mSecondsOfRunning;

    MWWorld::ConstPtr mHeadTrackTarget;

    float mTurnAnimationThreshold; // how long to continue playing turning animation after actor stopped turning

    std::string mAttackType; // slash, chop or thrust

    bool mAttackingOrSpell;

    void setAttackTypeBasedOnMovement();

    void refreshCurrentAnims(CharacterState idle, CharacterState movement, JumpingState jump, bool force=false);
    void refreshHitRecoilAnims();
    void refreshJumpAnims(const WeaponInfo* weap, JumpingState jump, bool force=false);
    void refreshMovementAnims(const WeaponInfo* weap, CharacterState movement, bool force=false);
    void refreshIdleAnims(const WeaponInfo* weap, CharacterState idle, bool force=false);

    void clearAnimQueue();

    bool updateWeaponState();
    bool updateCreatureState();
    void updateIdleStormState(bool inwater);

    void updateAnimQueue();

    void updateHeadTracking(float duration);

    void updateMagicEffects();

    void playDeath(float startpoint, CharacterState death);
    CharacterState chooseRandomDeathState() const;
    void playRandomDeath(float startpoint = 0.0f);

    /// choose a random animation group with \a prefix and numeric suffix
    /// @param num if non-NULL, the chosen animation number will be written here
    std::string chooseRandomGroup (const std::string& prefix, int* num = NULL) const;

    bool updateCarriedLeftVisible(WeaponType weaptype) const;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim);
    virtual ~CharacterController();

    virtual void handleTextKey(const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key,
                       const std::multimap<float, std::string>& map);

    // Be careful when to call this, see comment in Actors
    void updateContinuousVfx();

    void updatePtr(const MWWorld::Ptr &ptr);

    void update(float duration);

    void persistAnimationState();
    void unpersistAnimationState();

    bool playGroup(const std::string &groupname, int mode, int count, bool persist=false);
    void skipAnim();
    bool isAnimPlaying(const std::string &groupName);

    enum KillResult
    {
        Result_DeathAnimStarted,
        Result_DeathAnimPlaying,
        Result_DeathAnimJustFinished,
        Result_DeathAnimFinished
    };
    KillResult kill();

    void resurrect();
    bool isDead() const
    { return mDeathState != CharState_None; }

    void forceStateUpdate();
    
    bool isReadyToBlock() const;
    bool isKnockedOut() const;
    bool isSneaking() const;

    void setAttackingOrSpell(bool attackingOrSpell);
    void setAIAttackType(const std::string& attackType);
    static void setAttackTypeRandomly(std::string& attackType);

    bool readyToPrepareAttack() const;
    bool readyToStartAttack() const;

    float getAttackStrength() const;

    /// @see Animation::setActive
    void setActive(bool active);

    /// Make this character turn its head towards \a target. To turn off head tracking, pass an empty Ptr.
    void setHeadTrackTarget(const MWWorld::ConstPtr& target);

    void playSwishSound(float attackStrength);
};

    MWWorld::ContainerStoreIterator getActiveWeapon(CreatureStats &stats, MWWorld::InventoryStore &inv, WeaponType *weaptype);
}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
