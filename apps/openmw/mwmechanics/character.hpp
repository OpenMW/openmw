#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include <deque>

#include <components/esm/loadmgef.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwrender/animation.hpp"

#include "weapontype.hpp"

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
    Priority_Persistent,

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
    CharState_SwimTurnLeft,
    CharState_SwimTurnRight,

    CharState_Jump,

    CharState_Death1,
    CharState_Death2,
    CharState_Death3,
    CharState_Death4,
    CharState_Death5,
    CharState_SwimDeath,
    CharState_SwimDeathKnockDown,
    CharState_SwimDeathKnockOut,
    CharState_DeathKnockDown,
    CharState_DeathKnockOut,

    CharState_Hit,
    CharState_SwimHit,
    CharState_KnockDown,
    CharState_KnockOut,
    CharState_SwimKnockDown,
    CharState_SwimKnockOut,
    CharState_Block
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
    MWWorld::Ptr mWeapon;
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

    int mWeaponType;
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
    bool mCastingManualSpell;

    float mTimeUntilWake;

    void setAttackTypeBasedOnMovement();

    void refreshCurrentAnims(CharacterState idle, CharacterState movement, JumpingState jump, bool force=false);
    void refreshHitRecoilAnims(CharacterState& idle);
    void refreshJumpAnims(const std::string& weapShortGroup, JumpingState jump, CharacterState& idle, bool force=false);
    void refreshMovementAnims(const std::string& weapShortGroup, CharacterState movement, CharacterState& idle, bool force=false);
    void refreshIdleAnims(const std::string& weapShortGroup, CharacterState idle, bool force=false);

    void clearAnimQueue(bool clearPersistAnims = false);

    bool updateWeaponState(CharacterState& idle);
    bool updateCreatureState();
    void updateIdleStormState(bool inwater);

    std::string chooseRandomAttackAnimation() const;
    bool isRandomAttackAnimation(const std::string& group) const;

    bool isPersistentAnimPlaying();

    void updateAnimQueue();

    void updateHeadTracking(float duration);

    void updateMagicEffects();

    void playDeath(float startpoint, CharacterState death);
    CharacterState chooseRandomDeathState() const;
    void playRandomDeath(float startpoint = 0.0f);

    /// choose a random animation group with \a prefix and numeric suffix
    /// @param num if non-nullptr, the chosen animation number will be written here
    std::string chooseRandomGroup (const std::string& prefix, int* num = nullptr) const;

    bool updateCarriedLeftVisible(int weaptype) const;

    std::string fallbackShortWeaponGroup(const std::string& baseGroupName, MWRender::Animation::BlendMask* blendMask = nullptr);

    std::string getWeaponAnimation(int weaponType) const;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim);
    virtual ~CharacterController();

    virtual void handleTextKey(const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key,
                       const std::multimap<float, std::string>& map);

    // Be careful when to call this, see comment in Actors
    void updateContinuousVfx();

    void updatePtr(const MWWorld::Ptr &ptr);

    void update(float duration, bool animationOnly=false);

    bool onOpen();
    void onClose();

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
    
    bool isAttackPreparing() const;
    bool isCastingSpell() const;
    bool isReadyToBlock() const;
    bool isKnockedDown() const;
    bool isKnockedOut() const;
    bool isRecovery() const;
    bool isSneaking() const;
    bool isRunning() const;
    bool isTurning() const;
    bool isAttackingOrSpell() const;

    void setVisibility(float visibility);
    void setAttackingOrSpell(bool attackingOrSpell);
    void castSpell(const std::string spellId, bool manualSpell=false);
    void setAIAttackType(const std::string& attackType);
    static void setAttackTypeRandomly(std::string& attackType);

    bool readyToPrepareAttack() const;
    bool readyToStartAttack() const;

    float getAttackStrength() const;

    /// @see Animation::setActive
    void setActive(int active);

    /// Make this character turn its head towards \a target. To turn off head tracking, pass an empty Ptr.
    void setHeadTrackTarget(const MWWorld::ConstPtr& target);

    void playSwishSound(float attackStrength);
};
}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
