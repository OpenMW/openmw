#include "convertplayer.hpp"

namespace ESSImport
{

    void convertPCDT(const PCDT& pcdt, ESM::Player& out, std::vector<std::string>& outDialogueTopics, bool& firstPersonCam)
    {
        out.mBirthsign = pcdt.mBirthsign;
        out.mObject.mNpcStats.mBounty = pcdt.mBounty;
        for (std::vector<PCDT::FNAM>::const_iterator it = pcdt.mFactions.begin(); it != pcdt.mFactions.end(); ++it)
        {
            ESM::NpcStats::Faction faction;
            faction.mExpelled = (it->mFlags & 0x2) != 0;
            faction.mRank = it->mRank;
            faction.mReputation = it->mReputation;
            out.mObject.mNpcStats.mFactions[Misc::StringUtils::lowerCase(it->mFactionName.toString())] = faction;
        }
        for (int i=0; i<8; ++i)
            out.mObject.mNpcStats.mSkillIncrease[i] = pcdt.mPNAM.mSkillIncreases[i];
        for (int i=0; i<27; ++i)
            out.mObject.mNpcStats.mSkills[i].mRegular.mProgress = pcdt.mPNAM.mSkillProgress[i];
        out.mObject.mNpcStats.mLevelProgress = pcdt.mPNAM.mLevelProgress;

        if (pcdt.mPNAM.mDrawState & PCDT::DrawState_Weapon)
            out.mObject.mCreatureStats.mDrawState = 1;
        if (pcdt.mPNAM.mDrawState & PCDT::DrawState_Spell)
            out.mObject.mCreatureStats.mDrawState = 2;

        firstPersonCam = (pcdt.mPNAM.mCameraState == PCDT::CameraState_FirstPerson);

        for (std::vector<std::string>::const_iterator it = pcdt.mKnownDialogueTopics.begin();
             it != pcdt.mKnownDialogueTopics.end(); ++it)
        {
            outDialogueTopics.push_back(Misc::StringUtils::lowerCase(*it));
        }
    }

}
