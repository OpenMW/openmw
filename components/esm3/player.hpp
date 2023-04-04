#ifndef OPENMW_ESM_PLAYER_H
#define OPENMW_ESM_PLAYER_H

#include <string>

#include "components/esm/defs.hpp"
#include "npcstate.hpp"

#include "components/esm/attr.hpp"
#include "loadskil.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct Player
    {
        NpcState mObject;
        RefId mCellId;
        float mLastKnownExteriorPosition[3];
        unsigned char mHasMark;
        bool mSetWerewolfAcrobatics;
        Position mMarkedPosition;
        RefId mMarkedCell;
        ESM::RefId mBirthsign;

        int mCurrentCrimeId;
        int mPaidCrimeId;

        float mSaveAttributes[Attribute::Length];
        float mSaveSkills[Skill::Length];

        typedef std::map<ESM::RefId, ESM::RefId> PreviousItems; // previous equipped items, needed for bound spells
        PreviousItems mPreviousItems;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
