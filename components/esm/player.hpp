#ifndef OPENMW_ESM_PLAYER_H
#define OPENMW_ESM_PLAYER_H

#include <string>

#include "npcstate.hpp"
#include "cellid.hpp"
#include "defs.hpp"

#include "loadskil.hpp"
#include "attr.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct Player
    {
        NpcState mObject;
        CellId mCellId;
        float mLastKnownExteriorPosition[3];
        unsigned char mHasMark;
        ESM::Position mMarkedPosition;
        CellId mMarkedCell;
        unsigned char mAutoMove;
        std::string mBirthsign;
        
        int mCurrentCrimeId;
        int mPaidCrimeId;

        StatState<int> mSaveAttributes[ESM::Attribute::Length];
        StatState<int> mSaveSkills[ESM::Skill::Length];

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
