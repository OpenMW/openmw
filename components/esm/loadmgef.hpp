#ifndef _ESM_MGEF_H
#define _ESM_MGEF_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct MagicEffect
{
    enum Flags
    {
        SpellMaking = 0x0200,
        Enchanting = 0x0400,
        Negative = 0x0800 // A harmful effect. Will determine whether
                          // eg. NPCs regard this spell as an attack.
    };

    struct MEDTstruct
    {
        int school; // SpellSchool, see defs.hpp
        float baseCost;
        int flags;
        // Properties of the fired magic 'ball' I think
        int red, blue, green;
        float speed, size, sizeCap;
    }; // 36 bytes

    MEDTstruct data;

    std::string icon, particle, // Textures
            casting, hit, area, // Statics
            bolt, // Weapon
            castSound, boltSound, hitSound, areaSound, // Sounds
            description;

    // Index of this magical effect. Corresponds to one of the
    // hard-coded effects in the original engine:
    // 0-136 in Morrowind
    // 137 in Tribunal
    // 138-140 in Bloodmoon (also changes 64?)
    // 141-142 are summon effects introduced in bloodmoon, but not used
    // there. They can be redefined in mods by setting the name in GMST
    // sEffectSummonCreature04/05 creature id in
    // sMagicCreature04ID/05ID.
    int index;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
