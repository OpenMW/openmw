#ifndef OPENMW_ESSIMPORT_CONTEXT_H
#define OPENMW_ESSIMPORT_CONTEXT_H

#include <map>

#include <components/esm/loadnpc.hpp>
#include <components/esm/player.hpp>
#include <components/esm/dialoguestate.hpp>

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

        int mDay, mMonth, mYear;
        float mHour;

        // key <refIndex, refId>
        std::map<std::pair<int, std::string>, CREC> mCreatureChanges;
        std::map<std::pair<int, std::string>, NPCC> mNpcChanges;
        std::map<std::pair<int, std::string>, CNTC> mContainerChanges;

        Context()
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
        }
    };

}

#endif
