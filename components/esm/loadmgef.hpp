#ifndef OPENMW_ESM_MGEF_H
#define OPENMW_ESM_MGEF_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct MagicEffect
{
    enum Flags
    {
        TargetSkill = 0x1, // Affects a specific skill, which is specified elsewhere in the effect structure.
        TargetAttribute = 0x2, // Affects a specific attribute, which is specified elsewhere in the effect structure.
        NoDuration = 0x4, // Has no duration. Only runs effect once on cast.
        NoMagnitude = 0x8, // Has no magnitude.
        Harmful = 0x10, // Counts as a negative effect. Interpreted as useful for attack, and is treated as a bad effect in alchemy.
        ContinuousVfx = 0x20, // The effect's hit particle VFX repeats for the full duration of the spell, rather than occuring once on hit.
        CastSelf = 0x40, // Allows range - cast on self.
        CastTouch = 0x80, // Allows range - cast on touch.
        CastTarget = 0x100, // Allows range - cast on target.
        UncappedDamage = 0x1000, // Negates multiple cap behaviours. Allows an effect to reduce an attribute below zero; removes the normal minimum effect duration of 1 second.
        NonRecastable = 0x4000,	// Does not land if parent spell is already affecting target. Shows "you cannot re-cast" message for self target.
        Unreflectable = 0x10000, // Cannot be reflected, the effect always lands normally.
        CasterLinked = 0x20000,	// Must quench if caster is dead, or not an NPC/creature. Not allowed in containter/door trap spells.
        SpellMaking = 0x0200,
        Enchanting = 0x0400,
        Negative = 0x0800 // A harmful effect. Will determine whether
                          // eg. NPCs regard this spell as an attack. (same as 0x10?)
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

    static std::string effectIdToString(short effectID);


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
};
}
#endif
