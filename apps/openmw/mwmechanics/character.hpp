#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include <OgreVector3.h>

#include <components/esm/loadmgef.hpp>

#include "../mwworld/ptr.hpp"

namespace MWWorld
{
    class ContainerStoreIterator;
    class InventoryStore;
}

namespace MWRender
{
    class Animation;
}

namespace MWMechanics
{

class Movement;
class NpcStats;

enum Priority {
    Priority_Default,
    Priority_Jump,
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

    CharState_Death1,
    CharState_Death2,
    CharState_Death3,
    CharState_Death4,
    CharState_Death5,
    CharState_SwimDeath
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
    UpperCharState_FollowStartToFollowStop,
    UpperCharState_CastingSpell
};

enum JumpingState {
    JumpState_None,
    JumpState_Falling,
    JumpState_Landing
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

    JumpingState mJumpState;
    std::string mCurrentJump;

    WeaponType mWeaponType;
    std::string mCurrentWeapon;

    bool mSkipAnim;

    // counted for skill increase
    float mSecondsOfSwimming;
    float mSecondsOfRunning;

    // used for acrobatics progress and fall damages
    float mFallHeight;

    std::string mAttackType; // slash, chop or thrust

    void refreshCurrentAnims(CharacterState idle, CharacterState movement, bool force=false);

    static void getWeaponGroup(WeaponType weaptype, std::string &group);

    static MWWorld::ContainerStoreIterator getActiveWeapon(NpcStats &stats,
                                                           MWWorld::InventoryStore &inv,
                                                           WeaponType *weaptype);

    void clearAnimQueue();

    bool updateNpcState(bool onground, bool inwater, bool isrunning, bool sneak);

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim);
    virtual ~CharacterController();

    void updatePtr(const MWWorld::Ptr &ptr);

    void update(float duration);

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
