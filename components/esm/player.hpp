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
        std::string mBirthsign;

        int mCurrentCrimeId;
        int mPaidCrimeId;

        StatState<float> mSaveAttributes[ESM::Attribute::Length];
        StatState<float> mSaveSkills[ESM::Skill::Length];

        typedef std::map<std::string, std::string> PreviousItems; // previous equipped items, needed for bound spells
        PreviousItems mPreviousItems;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
