#ifndef OPENMW_ESM_PLAYER_H
#define OPENMW_ESM_PLAYER_H

#include <string>

#include <components/esm/attr.hpp>
#include <components/esm/position.hpp>

#include "loadskil.hpp"
#include "npcstate.hpp"

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

        int32_t mCurrentCrimeId;
        int32_t mPaidCrimeId;

        float mSaveAttributes[Attribute::Length];
        float mSaveSkills[Skill::Length];

        std::map<ESM::RefId, ESM::RefId> mPreviousItems; // previous equipped items, needed for bound spells

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
