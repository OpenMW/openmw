#ifndef _ESM_ENCH_H
#define _ESM_ENCH_H

#include "record.hpp"
#include "effectlist.hpp"

namespace ESM
{
/*
 * Enchantments
 */

struct Enchantment : public Record
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

    int getName() { return REC_ENCH; }
};
}
#endif
