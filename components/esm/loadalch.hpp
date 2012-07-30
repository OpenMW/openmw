#ifndef _ESM_ALCH_H
#define _ESM_ALCH_H

#include "esm_reader.hpp"
#include "defs.hpp"

namespace ESM
{

/*
 * Alchemy item (potions)
 */

struct Potion
{
    struct ALDTstruct
    {
        float weight;
        int value;
        int autoCalc;
    };
    ALDTstruct data;

    std::string name, model, icon, script;
    EffectList effects;

    std::string mId;

    void load(ESMReader &esm, const std::string& id);
};
}
#endif
