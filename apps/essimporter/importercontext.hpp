#ifndef OPENMW_ESSIMPORT_CONTEXT_H
#define OPENMW_ESSIMPORT_CONTEXT_H

#include <map>

#include <components/esm3/controlsstate.hpp>
#include <components/esm3/dialoguestate.hpp>
#include <components/esm3/globalmap.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/player.hpp>

#include "importcntc.hpp"
#include "importcrec.hpp"
#include "importnpcc.hpp"
#include "importsplm.h"

namespace ESSImport
{

    struct Context
    {
        // set from the TES3 header
        std::string mPlayerCellName;

        ESM::Player mPlayer;
        ESM::NPC mPlayerBase;
        std::string mCustomPlayerClassName;

        ESM::DialogueState mDialogueState;

        ESM::ControlsState mControlsState;

        // cells which should show an explored overlay on the global map
        std::set<std::pair<int, int>> mExploredCells;

        ESM::GlobalMap mGlobalMapState;

        int mDay, mMonth, mYear;
        float mHour;

        // key <refIndex, refId>
        std::map<std::pair<int, ESM::RefId>, CREC> mCreatureChanges;
        std::map<std::pair<int, ESM::RefId>, NPCC> mNpcChanges;
        std::map<std::pair<int, ESM::RefId>, CNTC> mContainerChanges;

        std::map<std::pair<int, ESM::RefId>, ESM::RefNum> mActorIdMap;
        ESM::RefNum mNextRefNum;

        std::map<ESM::RefId, ESM::Creature> mCreatures;
        std::map<ESM::RefId, ESM::NPC> mNpcs;

        std::vector<SPLM::ActiveSpell> mActiveSpells;

        Context()
            : mDay(0)
            , mMonth(0)
            , mYear(0)
            , mHour(0.f)
        {
            mPlayer.mCellId = ESM::RefId::esm3ExteriorCell(0, 0);
            mPlayer.mLastKnownExteriorPosition[0] = mPlayer.mLastKnownExteriorPosition[1]
                = mPlayer.mLastKnownExteriorPosition[2] = 0.0f;
            mPlayer.mHasMark = 0;
            mPlayer.mCurrentCrimeId = -1; // TODO
            mPlayer.mPaidCrimeId = -1;
            mPlayer.mObject.blank();
            mPlayer.mObject.mEnabled = true;
            mPlayer.mObject.mRef.mRefID = ESM::RefId::stringRefId("player"); // REFR.mRefID would be PlayerSaveGame
            generateRefNum(mPlayer.mObject.mRef.mRefNum);

            mGlobalMapState.mBounds.mMinX = 0;
            mGlobalMapState.mBounds.mMaxX = 0;
            mGlobalMapState.mBounds.mMinY = 0;
            mGlobalMapState.mBounds.mMaxY = 0;

            mPlayerBase.blank();
        }

        void generateRefNum(ESM::RefNum& refNum)
        {
            if (!refNum.isSet())
            {
                mNextRefNum.mIndex++;
                if (mNextRefNum.mIndex == 0)
                    mNextRefNum.mContentFile--;
                refNum = mNextRefNum;
            }
        }
    };

}

#endif
