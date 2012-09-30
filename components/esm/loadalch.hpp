#ifndef OPENMW_ESM_ALCH_H
#define OPENMW_ESM_ALCH_H

#include <string>

#include "record.hpp"
#include "effectlist.hpp"

namespace ESM
{
/*
 * Alchemy item (potions)
 */

struct Potion
{
    struct ALDTstruct
    {
        float mWeight;
        int mValue;
        int mAutoCalc;
    };
    ALDTstruct mData;

    std::string mId, mName, mModel, mIcon, mScript;
    EffectList mEffects;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
    };
}
#endif
