#ifndef OPENMW_ESSIMPORT_CONTEXT_H
#define OPENMW_ESSIMPORT_CONTEXT_H

#include <map>

#include <components/esm/loadnpc.hpp>
#include <components/esm/player.hpp>
#include <components/esm/dialoguestate.hpp>
#include <components/esm/globalmap.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/controlsstate.hpp>

#include "importnpcc.hpp"
#include "importcrec.hpp"
#include "importcntc.hpp"
#include "importplayer.hpp"
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
        std::set<std::pair<int, int> > mExploredCells;

        ESM::GlobalMap mGlobalMapState;

        int mDay, mMonth, mYear;
        float mHour;

        // key <refIndex, refId>
        std::map<std::pair<int, std::string>, CREC> mCreatureChanges;
        std::map<std::pair<int, std::string>, NPCC> mNpcChanges;
        std::map<std::pair<int, std::string>, CNTC> mContainerChanges;

        std::map<std::pair<int, std::string>, int> mActorIdMap;
        int mNextActorId;

        std::map<std::string, ESM::Creature> mCreatures;
        std::map<std::string, ESM::NPC> mNpcs;

        std::vector<SPLM::ActiveSpell> mActiveSpells;

        Context()
            : mDay(0)
            , mMonth(0)
            , mYear(0)
            , mHour(0.f)
            , mNextActorId(0)
        {
            mPlayer.mAutoMove = 0;
            ESM::CellId playerCellId;
            playerCellId.mPaged = true;
            playerCellId.mIndex.mX = playerCellId.mIndex.mY = 0;
            mPlayer.mCellId = playerCellId;
            mPlayer.mLastKnownExteriorPosition[0]
                = mPlayer.mLastKnownExteriorPosition[1]
                = mPlayer.mLastKnownExteriorPosition[2]
                = 0.0f;
            mPlayer.mHasMark = 0;
            mPlayer.mCurrentCrimeId = -1; // TODO
            mPlayer.mPaidCrimeId = -1;
            mPlayer.mObject.blank();
            mPlayer.mObject.mEnabled = true;
            mPlayer.mObject.mRef.mRefID = "player"; // REFR.mRefID would be PlayerSaveGame
            mPlayer.mObject.mCreatureStats.mActorId = generateActorId();

            mGlobalMapState.mBounds.mMinX = 0;
            mGlobalMapState.mBounds.mMaxX = 0;
            mGlobalMapState.mBounds.mMinY = 0;
            mGlobalMapState.mBounds.mMaxY = 0;

            mPlayerBase.blank();
        }

        int generateActorId()
        {
            return mNextActorId++;
        }
    };

}

#endif
