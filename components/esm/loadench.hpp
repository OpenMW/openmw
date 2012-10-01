#ifndef OPENMW_ESM_ENCH_H
#define OPENMW_ESM_ENCH_H

#include "effectlist.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Enchantments
 */

struct Enchantment
{
    enum Type
    {
        CastOnce = 0,
        WhenStrikes = 1,
        WhenUsed = 2,
        ConstantEffect = 3
    };

    struct ENDTstruct
    {
        int mType;
        int mCost;
        int mCharge;
        int mAutocalc; // Guessing this is 1 if we are supposed to auto
        // calculate
    };

    ENDTstruct mData;
    EffectList mEffects;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
