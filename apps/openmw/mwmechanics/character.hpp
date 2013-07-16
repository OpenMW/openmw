#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include <OgreVector3.h>

#include "../mwworld/ptr.hpp"

namespace MWRender
{
    class Animation;
}

namespace MWMechanics
{

class Movement;

enum Priority {
    Priority_Default,
    Priority_Movement,
    Priority_Weapon,
    Priority_Torch,

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

    /* Death states must be last! */
    CharState_Death1,
    CharState_Death2,
    CharState_Death3,
    CharState_Death4,
    CharState_Death5
};

enum WeaponType {
    WeapType_None,

    WeapType_HandToHand,
    WeapType_OneHand,
    WeapType_TwoHand,
    WeapType_TwoWide,
    WeapType_BowAndArrow,
    WeapType_Crossbow,
    WeapType_ThowWeapon,
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
    UpperCharState_LargeFollowStartToLargeFollowStop,
    UpperCharState_MediumFollowStartToMediumFollowStop,
    UpperCharState_SmallFollowStartToSmallFollowStop,
    UpperCharState_EquipingSpell,
    UpperCharState_UnEquipingSpell
};

class CharacterController
{
    MWWorld::Ptr mPtr;
    MWRender::Animation *mAnimation;

    typedef std::deque<std::pair<std::string,size_t> > AnimationQueue;
    AnimationQueue mAnimQueue;

    CharacterState mIdleState;
    std::string mCurrentIdle;

    CharacterState mMovementState;
    std::string mCurrentMovement;
    float mMovementSpeed;

    CharacterState mDeathState;
    std::string mCurrentDeath;

    UpperBodyCharacterState mUpperBodyState;

    WeaponType mWeaponType;
    bool mSkipAnim;

    // Workaround for playing weapon draw animation and sound when going to new cell
    bool mUpdateWeapon;

    // counted for skill increase
    float mSecondsOfSwimming;
    float mSecondsOfRunning;

    std::string mAttackType; // slash, chop or thrust

    void refreshCurrentAnims(CharacterState idle, CharacterState movement, bool force=false);

    static void getWeaponGroup(WeaponType weaptype, std::string &group);

    void clearAnimQueue();

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim);
    virtual ~CharacterController();

    void updatePtr(const MWWorld::Ptr &ptr);

    void update(float duration, Movement &movement);

    void playGroup(const std::string &groupname, int mode, int count);
    void skipAnim();
    bool isAnimPlaying(const std::string &groupName);

    void kill();
    void resurrect();
    bool isDead() const
    { return mDeathState != CharState_None; }

    void forceStateUpdate();
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
