#ifndef GAME_MWMECHANICS_MAGICSCHOOL_H
#define GAME_MWMECHANICS_MAGICSCHOOL_H

#include <components/esm/refid.hpp>

#include <array>

namespace MWMechanics
{
    struct MagicSchool
    {
        ESM::RefId mAreaSound;
        ESM::RefId mBoltSound;
        ESM::RefId mCastSound;
        ESM::RefId mFailureSound;
        ESM::RefId mHitSound;
        std::string mName;
        int mAutoCalcMax;

        static constexpr int Length = 6;
    };

    const MagicSchool& getMagicSchool(int index);
}

#endif
