#ifndef OPENMW_ESM_SPEL_H
#define OPENMW_ESM_SPEL_H

#include <string>

#include "effectlist.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Spell
{
    static unsigned int sRecordId;

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
        F_Autocalc = 1, // Can be selected by NPC spells auto-calc
        F_PCStart = 2, // Can be selected by player spells auto-calc
        F_Always = 4 // Casting always succeeds
    };

    struct SPDTstruct
    {
        int mType; // SpellType
        int mCost; // Mana cost
        int mFlags; // Flags
    };

    SPDTstruct mData;
    std::string mId, mName;
    EffectList mEffects;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID/index).
};
}
#endif
