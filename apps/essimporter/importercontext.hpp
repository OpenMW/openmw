#ifndef OPENMW_ESSIMPORT_CONTEXT_H
#define OPENMW_ESSIMPORT_CONTEXT_H

#include <map>

#include <components/esm/loadnpc.hpp>
#include <components/esm/player.hpp>

#include "importnpcc.hpp"
#include "importcrec.hpp"
#include "importplayer.hpp"



namespace ESSImport
{

    struct Context
    {
        ESM::Player mPlayer;
        ESM::NPC mPlayerBase;
        std::string mCustomPlayerClassName;

        int mDay, mMonth, mYear;
        float mHour;

        // key <refIndex, refId>
        std::map<std::pair<int, std::string>, CREC> mCreatureChanges;
        std::map<std::pair<int, std::string>, NPCC> mNpcChanges;

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
            mPlayer.mObject.mCount = 1;
            mPlayer.mObject.mEnabled = 1;
            mPlayer.mObject.mHasLocals = false;
            mPlayer.mObject.mRef.mRefID = "player"; // REFR.mRefID would be PlayerSaveGame
            mPlayer.mObject.mCreatureStats.mHasAiSettings = true;
            mPlayer.mObject.mCreatureStats.mDead = false;
            mPlayer.mObject.mCreatureStats.mDied = false;
            mPlayer.mObject.mCreatureStats.mKnockdown = false;
            mPlayer.mObject.mCreatureStats.mKnockdownOneFrame = false;
            mPlayer.mObject.mCreatureStats.mKnockdownOverOneFrame = false;
            mPlayer.mObject.mCreatureStats.mHitRecovery = false;
            mPlayer.mObject.mCreatureStats.mBlock = false;
            mPlayer.mObject.mCreatureStats.mMovementFlags = 0;
            mPlayer.mObject.mCreatureStats.mAttackStrength = 0.f;
            mPlayer.mObject.mCreatureStats.mFallHeight = 0.f;
            mPlayer.mObject.mCreatureStats.mRecalcDynamicStats = false;
            mPlayer.mObject.mCreatureStats.mDrawState = 0;
            mPlayer.mObject.mCreatureStats.mDeathAnimation = 0;
            mPlayer.mObject.mNpcStats.mIsWerewolf = false;
            mPlayer.mObject.mNpcStats.mTimeToStartDrowning = 20;
            mPlayer.mObject.mNpcStats.mLevelProgress = 0;
            mPlayer.mObject.mNpcStats.mDisposition = 0;
            mPlayer.mObject.mNpcStats.mTimeToStartDrowning = 20;
            mPlayer.mObject.mNpcStats.mReputation = 0;
            mPlayer.mObject.mNpcStats.mCrimeId = -1;
            mPlayer.mObject.mNpcStats.mWerewolfKills = 0;
            mPlayer.mObject.mNpcStats.mProfit = 0;
        }
    };

}

#endif
