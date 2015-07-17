#ifndef OPENMW_ESSIMPORT_CONTEXT_H
#define OPENMW_ESSIMPORT_CONTEXT_H

#include <map>

#include <components/esm/loadnpc.hpp>
#include <components/esm/player.hpp>
#include <components/esm/dialoguestate.hpp>
#include <components/esm/globalmap.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/loadnpc.hpp>

#include "importnpcc.hpp"
#include "importcrec.hpp"
#include "importcntc.hpp"
#include "importplayer.hpp"




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

        // cells which should show an explored overlay on the global map
        std::set<std::pair<int, int> > mExploredCells;

        ESM::GlobalMap mGlobalMapState;

        int mDay, mMonth, mYear;
        float mHour;

        // key <refIndex, refId>
        std::map<std::pair<int, std::string>, CREC> mCreatureChanges;
        std::map<std::pair<int, std::string>, NPCC> mNpcChanges;
        std::map<std::pair<int, std::string>, CNTC> mContainerChanges;

        std::map<std::string, ESM::Creature> mCreatures;
        std::map<std::string, ESM::NPC> mNpcs;

        Context()
            : mDay(0)
            , mMonth(0)
            , mYear(0)
            , mHour(0.f)
        {
            mPlayer.mAutoMove = 0;
            ESM::CellId playerCellId;
            playerCellId.mPaged = true;
            playerCellId.mIndex.mX = playerCellId.mIndex.mY = 0;
            mPlayer.mCellId = playerCellId;
            //mPlayer.mLastKnownExteriorPosition
            mPlayer.mHasMark = 0; // TODO
            mPlayer.mCurrentCrimeId = 0; // TODO
            mPlayer.mObject.blank();
            mPlayer.mObject.mRef.mRefID = "player"; // REFR.mRefID would be PlayerSaveGame

            mGlobalMapState.mBounds.mMinX = 0;
            mGlobalMapState.mBounds.mMaxX = 0;
            mGlobalMapState.mBounds.mMinY = 0;
            mGlobalMapState.mBounds.mMaxY = 0;
        }
    };

}

#endif
