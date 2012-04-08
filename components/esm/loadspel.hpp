#ifndef _ESM_SPEL_H
#define _ESM_SPEL_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"
#include "defs.hpp"

namespace ESM
{

struct Spell : public Record
{
    enum SpellType
    {
        ST_Spell = 0,   // Normal spell, must be cast and costs mana
        ST_Ability = 1, // Inert ability, always in effect
        ST_Blight = 2,  // Blight disease
        ST_Disease = 3, // Common disease
        ST_Curse = 4,   // Curse (?)
        ST_Power = 5    // Power, can use once a day
    };

    enum Flags
    {
        F_Autocalc = 1,
        F_PCStart = 2,
        F_Always = 4 // Casting always succeeds
    };

    struct SPDTstruct
    {
        int type; // SpellType
        int cost; // Mana cost
        int flags; // Flags
    };

    SPDTstruct data;
    std::string name;
    EffectList effects;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_SPEL; }
};
}
#endif
