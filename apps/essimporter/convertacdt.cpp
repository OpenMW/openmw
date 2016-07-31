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

}
