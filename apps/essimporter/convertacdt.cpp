#include <string>
#include <iostream>
#include <limits>

#include <components/misc/stringops.hpp>

#include "convertacdt.hpp"

namespace ESSImport
{

    int translateDynamicIndex(int mwIndex)
    {
        if (mwIndex == 1)
            return 2;
        else if (mwIndex == 2)
            return 1;
        return mwIndex;
    }

    void convertACDT (const ACDT& acdt, ESM::CreatureStats& cStats)
    {
        for (int i=0; i<3; ++i)
        {
            int writeIndex = translateDynamicIndex(i);
            cStats.mDynamic[writeIndex].mBase = acdt.mDynamic[i][1];
            cStats.mDynamic[writeIndex].mMod = acdt.mDynamic[i][1];
            cStats.mDynamic[writeIndex].mCurrent = acdt.mDynamic[i][0];
        }
        for (int i=0; i<8; ++i)
        {
            cStats.mAttributes[i].mBase = static_cast<int>(acdt.mAttributes[i][1]);
            cStats.mAttributes[i].mMod = static_cast<int>(acdt.mAttributes[i][0]);
            cStats.mAttributes[i].mCurrent = static_cast<int>(acdt.mAttributes[i][0]);
        }
        cStats.mGoldPool = acdt.mGoldPool;
        cStats.mTalkedTo = (acdt.mFlags & TalkedToPlayer) != 0;
        cStats.mAttacked = (acdt.mFlags & Attacked) != 0;
    }

    void convertACSC (const ACSC& acsc, ESM::CreatureStats& cStats)
    {
        cStats.mDead = (acsc.mFlags & Dead) != 0;
    }

    void convertNpcData (const ActorData& actorData, ESM::NpcStats& npcStats)
    {
        for (int i=0; i<ESM::Skill::Length; ++i)
        {
            npcStats.mSkills[i].mMod = actorData.mSkills[i][1];
            npcStats.mSkills[i].mCurrent = actorData.mSkills[i][1];
            npcStats.mSkills[i].mBase = actorData.mSkills[i][0];
        }

        npcStats.mTimeToStartDrowning = actorData.mACDT.mBreathMeter;
    }

    void convertANIS (const ANIS& anis, ESM::AnimationState& state)
    {
        static const char* animGroups[] =
        {
            "Idle", "Idle2", "Idle3", "Idle4", "Idle5", "Idle6", "Idle7", "Idle8", "Idle9", "Idlehh", "Idle1h", "Idle2c",
            "Idle2w", "IdleSwim", "IdleSpell", "IdleCrossbow", "IdleSneak", "IdleStorm", "Torch", "Hit1", "Hit2", "Hit3",
            "Hit4", "Hit5", "SwimHit1", "SwimHit2", "SwimHit3", "Death1", "Death2", "Death3", "Death4", "Death5",
            "DeathKnockDown", "DeathKnockOut", "KnockDown", "KnockOut", "SwimDeath", "SwimDeath2", "SwimDeath3",
            "SwimDeathKnockDown", "SwimDeathKnockOut", "SwimKnockOut", "SwimKnockDown", "SwimWalkForward",
            "SwimWalkBack", "SwimWalkLeft", "SwimWalkRight", "SwimRunForward", "SwimRunBack", "SwimRunLeft",
            "SwimRunRight", "SwimTurnLeft", "SwimTurnRight", "WalkForward", "WalkBack", "WalkLeft", "WalkRight",
            "TurnLeft", "TurnRight", "RunForward", "RunBack", "RunLeft", "RunRight", "SneakForward", "SneakBack",
            "SneakLeft", "SneakRight", "Jump", "WalkForwardhh", "WalkBackhh", "WalkLefthh", "WalkRighthh",
            "TurnLefthh", "TurnRighthh", "RunForwardhh", "RunBackhh", "RunLefthh", "RunRighthh", "SneakForwardhh",
            "SneakBackhh", "SneakLefthh", "SneakRighthh", "Jumphh", "WalkForward1h", "WalkBack1h", "WalkLeft1h",
            "WalkRight1h", "TurnLeft1h", "TurnRight1h", "RunForward1h", "RunBack1h", "RunLeft1h", "RunRight1h",
            "SneakForward1h", "SneakBack1h", "SneakLeft1h", "SneakRight1h", "Jump1h", "WalkForward2c", "WalkBack2c",
            "WalkLeft2c", "WalkRight2c", "TurnLeft2c", "TurnRight2c", "RunForward2c", "RunBack2c", "RunLeft2c",
            "RunRight2c", "SneakForward2c", "SneakBack2c", "SneakLeft2c", "SneakRight2c", "Jump2c", "WalkForward2w",
            "WalkBack2w", "WalkLeft2w", "WalkRight2w", "TurnLeft2w", "TurnRight2w", "RunForward2w", "RunBack2w",
            "RunLeft2w", "RunRight2w", "SneakForward2w", "SneakBack2w", "SneakLeft2w", "SneakRight2w", "Jump2w",
            "SpellCast", "SpellTurnLeft", "SpellTurnRight", "Attack1", "Attack2", "Attack3", "SwimAttack1",
            "SwimAttack2", "SwimAttack3", "HandToHand", "Crossbow", "BowAndArrow", "ThrowWeapon", "WeaponOneHand",
            "WeaponTwoHand", "WeaponTwoWide", "Shield", "PickProbe", "InventoryHandToHand", "InventoryWeaponOneHand",
            "InventoryWeaponTwoHand", "InventoryWeaponTwoWide"
        };

        if (anis.mGroupIndex < (sizeof(animGroups) / sizeof(*animGroups)))
        {
            std::string group(animGroups[anis.mGroupIndex]);
            Misc::StringUtils::lowerCaseInPlace(group);

            ESM::AnimationState::ScriptedAnimation scriptedAnim;
            scriptedAnim.mGroup = group;
            scriptedAnim.mTime = anis.mTime;
            scriptedAnim.mAbsolute = true;
            // Neither loop count nor queueing seems to be supported by the ess format.
            scriptedAnim.mLoopCount = std::numeric_limits<size_t>::max();
            state.mScriptedAnims.push_back(scriptedAnim);
        }
        else
            // TODO: Handle 0xFF index, which seems to be used for finished animations.
            std::cerr << "unknown animation group index: " << static_cast<unsigned int>(anis.mGroupIndex) << std::endl;
    }

}
