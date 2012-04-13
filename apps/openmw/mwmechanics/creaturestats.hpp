#ifndef GAME_MWMECHANICS_CREATURESTATS_H
#define GAME_MWMECHANICS_CREATURESTATS_H

#include <set>
#include <string>

#include "stat.hpp"
#include "magiceffects.hpp"
#include "spells.hpp"

namespace MWMechanics
{
    struct CreatureStats
    {
        Stat<int> mAttributes[8];
        DynamicStat<int> mDynamic[3]; // health, magicka, fatigue
        int mLevel;
        Spells mSpells;
        MagicEffects mMagicEffects;
    };
}

#endif
