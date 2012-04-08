#ifndef _ESM_ALCH_H
#define _ESM_ALCH_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"
#include "defs.hpp"

namespace ESM
{

/*
 * Alchemy item (potions)
 */

struct Potion : public Record
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

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
    
    int getName() { return REC_ALCH; }
};
}
#endif
