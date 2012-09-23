#ifndef OPENMW_ESM_MGEF_H
#define OPENMW_ESM_MGEF_H

#include <string>

#include "record.hpp"

namespace ESM
{

struct MagicEffect : public Record
{
    enum Flags
    {
        NoDuration = 0x4,
        SpellMaking = 0x0200,
        Enchanting = 0x0400,
        Negative = 0x0800 // A harmful effect. Will determine whether
                          // eg. NPCs regard this spell as an attack.
    };

    struct MEDTstruct
    {
        int mSchool; // SpellSchool, see defs.hpp
        float mBaseCost;
        int mFlags;
        // Properties of the fired magic 'ball' I think
        int mRed, mBlue, mGreen;
        float mSpeed, mSize, mSizeCap;
    }; // 36 bytes

    MEDTstruct mData;

    std::string mIcon, mParticle; // Textures
    std::string mCasting, mHit, mArea; // Statics
    std::string mBolt; // Weapon
    std::string mCastSound, mBoltSound, mHitSound, mAreaSound; // Sounds
    std::string mDescription;

    // Index of this magical effect. Corresponds to one of the
    // hard-coded effects in the original engine:
    // 0-136 in Morrowind
    // 137 in Tribunal
    // 138-140 in Bloodmoon (also changes 64?)
    // 141-142 are summon effects introduced in bloodmoon, but not used
    // there. They can be redefined in mods by setting the name in GMST
    // sEffectSummonCreature04/05 creature id in
    // sMagicCreature04ID/05ID.
    int mIndex;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_MGEF; }
};
}
#endif
