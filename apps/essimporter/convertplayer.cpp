#include "convertplayer.hpp"

#include <components/misc/constants.hpp>
#include <components/misc/stringops.hpp>

namespace ESSImport
{

    void convertPCDT(const PCDT& pcdt, ESM::Player& out, std::vector<std::string>& outDialogueTopics, bool& firstPersonCam, bool& teleportingEnabled, bool& levitationEnabled, ESM::ControlsState& controls)
    {
        out.mBirthsign = pcdt.mBirthsign;
        out.mObject.mNpcStats.mBounty = pcdt.mBounty;
        for (const auto & essFaction : pcdt.mFactions)
        {
            ESM::NpcStats::Faction faction;
            faction.mExpelled = (essFaction.mFlags & 0x2) != 0;
            faction.mRank = essFaction.mRank;
            faction.mReputation = essFaction.mReputation;
            out.mObject.mNpcStats.mFactions[Misc::StringUtils::lowerCase(essFaction.mFactionName.toString())] = faction;
        }
        for (int i=0; i<3; ++i)
            out.mObject.mNpcStats.mSpecIncreases[i] = pcdt.mPNAM.mSpecIncreases[i];
        for (int i=0; i<8; ++i)
            out.mObject.mNpcStats.mSkillIncrease[i] = pcdt.mPNAM.mSkillIncreases[i];
        for (int i=0; i<27; ++i)
            out.mObject.mNpcStats.mSkills[i].mProgress = pcdt.mPNAM.mSkillProgress[i];
        out.mObject.mNpcStats.mLevelProgress = pcdt.mPNAM.mLevelProgress;

        if (pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_WeaponDrawn)
            out.mObject.mCreatureStats.mDrawState = 1;
        if (pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_SpellDrawn)
            out.mObject.mCreatureStats.mDrawState = 2;

        firstPersonCam = !(pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_ThirdPerson);
        teleportingEnabled = !(pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_TeleportingDisabled);
        levitationEnabled = !(pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_LevitationDisabled);

        for (const auto & knownDialogueTopic : pcdt.mKnownDialogueTopics)
        {
            outDialogueTopics.push_back(Misc::StringUtils::lowerCase(knownDialogueTopic));
        }

        controls.mViewSwitchDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_ViewSwitchDisabled;
        controls.mControlsDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_ControlsDisabled;
        controls.mJumpingDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_JumpingDisabled;
        controls.mLookingDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_LookingDisabled;
        controls.mVanityModeDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_VanityModeDisabled;
        controls.mWeaponDrawingDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_WeaponDrawingDisabled;
        controls.mSpellDrawingDisabled = pcdt.mPNAM.mPlayerFlags & PCDT::PlayerFlags_SpellDrawingDisabled;

        if (pcdt.mHasMark)
        {
            out.mHasMark = 1;

            const PCDT::PNAM::MarkLocation& mark = pcdt.mPNAM.mMarkLocation;

            ESM::CellId cell;
            cell.mWorldspace = ESM::CellId::sDefaultWorldspace;
            cell.mPaged = true;

            cell.mIndex.mX = mark.mCellX;
            cell.mIndex.mY = mark.mCellY;

            // TODO: Figure out a better way to detect interiors. (0, 0) is a valid exterior cell.
            if (mark.mCellX == 0 && mark.mCellY == 0)
            {
                cell.mWorldspace = pcdt.mMNAM;
                cell.mPaged = false;
            }

            out.mMarkedCell = cell;
            out.mMarkedPosition.pos[0] = mark.mX;
            out.mMarkedPosition.pos[1] = mark.mY;
            out.mMarkedPosition.pos[2] = mark.mZ;
            out.mMarkedPosition.rot[0] = out.mMarkedPosition.rot[1] = 0.0f;
            out.mMarkedPosition.rot[2] = mark.mRotZ;
        }

        if (pcdt.mHasENAM)
        {
            out.mLastKnownExteriorPosition[0] = (pcdt.mENAM.mCellX + 0.5f) * Constants::CellSizeInUnits;
            out.mLastKnownExteriorPosition[1] = (pcdt.mENAM.mCellY + 0.5f) * Constants::CellSizeInUnits;
            out.mLastKnownExteriorPosition[2] = 0.0f;
        }
    }

}
