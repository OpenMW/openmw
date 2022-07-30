#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include <deque>

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

    CharacterState mIdleState{CharState_None};
    std::string mCurrentIdle;

    CharacterState mMovementState{CharState_None};
    std::string mCurrentMovement;
    float mMovementAnimSpeed{0.f};
    bool mAdjustMovementAnimSpeed{false};
    bool mHasMovedInXY{false};
    bool mMovementAnimationControlled{true};

    CharacterState mDeathState{CharState_None};
    std::string mCurrentDeath;
    bool mFloatToSurface{true};

    CharacterState mHitState{CharState_None};
    std::string mCurrentHit;

    UpperBodyCharacterState mUpperBodyState{UpperCharState_Nothing};

    JumpingState mJumpState{JumpState_None};
    std::string mCurrentJump;

    int mWeaponType{ESM::Weapon::None};
    std::string mCurrentWeapon;

    float mAttackStrength{0.f};

    bool mSkipAnim{false};

    // counted for skill increase
    float mSecondsOfSwimming{0.f};
    float mSecondsOfRunning{0.f};

    MWWorld::ConstPtr mHeadTrackTarget;

    float mTurnAnimationThreshold{0.f}; // how long to continue playing turning animation after actor stopped turning

    std::string mAttackType; // slash, chop or thrust

    bool mCanCast{false};

    bool mCastingManualSpell{false};

    bool mIsMovingBackward{false};
    osg::Vec2f mSmoothedSpeed;

    std::string_view getMovementBasedAttackType() const;

    void clearStateAnimation(std::string &anim) const;
    void resetCurrentJumpState();
    void resetCurrentMovementState();
    void resetCurrentIdleState();
    void resetCurrentHitState();
    void resetCurrentWeaponState();
    void resetCurrentDeathState();

    void refreshCurrentAnims(CharacterState idle, CharacterState movement, JumpingState jump, bool force=false);
    void refreshHitRecoilAnims();
    void refreshJumpAnims(JumpingState jump, bool force=false);
    void refreshMovementAnims(CharacterState movement, bool force=false);
    void refreshIdleAnims(CharacterState idle, bool force=false);

    void clearAnimQueue(bool clearPersistAnims = false);

    bool updateWeaponState(CharacterState idle);
    void updateIdleStormState(bool inwater) const;

    std::string chooseRandomAttackAnimation() const;
    static bool isRandomAttackAnimation(std::string_view group);

    bool isPersistentAnimPlaying() const;

    void updateAnimQueue();

    void updateHeadTracking(float duration);

    void updateMagicEffects() const;

    void playDeath(float startpoint, CharacterState death);
    CharacterState chooseRandomDeathState() const;
    void playRandomDeath(float startpoint = 0.0f);

    /// choose a random animation group with \a prefix and numeric suffix
    /// @param num if non-nullptr, the chosen animation number will be written here
    std::string chooseRandomGroup (const std::string& prefix, int* num = nullptr) const;

    bool updateCarriedLeftVisible(int weaptype) const;

    std::string fallbackShortWeaponGroup(const std::string& baseGroupName, MWRender::Animation::BlendMask* blendMask = nullptr) const;

    std::string_view getWeaponAnimation(int weaponType) const;
    std::string_view getWeaponShortGroup(int weaponType) const;

    bool getAttackingOrSpell() const;
    void setAttackingOrSpell(bool attackingOrSpell) const;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim);
    virtual ~CharacterController();

    CharacterController(const CharacterController&) = delete;
    CharacterController(CharacterController&&) = delete;

    const MWWorld::Ptr& getPtr() const { return mPtr; }

    void handleTextKey(std::string_view groupname, SceneUtil::TextKeyMap::ConstIterator key, const SceneUtil::TextKeyMap& map) override;

    // Be careful when to call this, see comment in Actors
    void updateContinuousVfx() const;

    void updatePtr(const MWWorld::Ptr &ptr);

    void update(float duration);

    bool onOpen() const;
    void onClose() const;

    void persistAnimationState() const;
    void unpersistAnimationState();

    bool playGroup(const std::string &groupname, int mode, int count, bool persist=false);
    void skipAnim();
    bool isAnimPlaying(const std::string &groupName) const;

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

    void setVisibility(float visibility) const;
    void castSpell(const std::string& spellId, bool manualSpell=false);
    void setAIAttackType(std::string_view attackType);
    static std::string_view getRandomAttackType();

    bool readyToPrepareAttack() const;
    bool readyToStartAttack() const;

    float getAttackStrength() const;

    /// @see Animation::setActive
    void setActive(int active) const;

    /// Make this character turn its head towards \a target. To turn off head tracking, pass an empty Ptr.
    void setHeadTrackTarget(const MWWorld::ConstPtr& target);

    void playSwishSound(float attackStrength) const;
};
}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
