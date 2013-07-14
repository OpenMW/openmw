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


    enum Effects
    {
        WaterBreathing = 0,
        SwiftSwim = 1,
        WaterWalking = 2,
        Shield = 3,
        FireShield = 4,
        LightningShield = 5,
        FrostShield = 6,
        Burden = 7,
        Feather = 8,
        Jump = 9,
        Levitate = 10,
        SlowFall = 11,
        Lock = 12,
        Open = 13,
        FireDamage = 14,
        ShockDamage = 15,
        FrostDamage = 16,
        DrainAttribute = 17,
        DrainHealth = 18,
        DrainMagicka = 19,
        DrainFatigue = 20,
        DrainSkill = 21,
        DamageAttribute = 22,
        DamageHealth = 23,
        DamageMagicka = 24,
        DamageFatigue = 25,
        DamageSkill = 26,
        Poison = 27,
        WeaknessToFire = 28,
        WeaknessToFrost = 29,
        WeaknessToShock = 30,
        WeaknessToMagicka = 31,
        WeaknessToCommonDisease = 32,
        WeaknessToBlightDisease = 33,
        WeaknessToCorprusDisease = 34,
        WeaknessToPoison = 35,
        WeaknessToNormalWeapons = 36,
        DisintegrateWeapon = 37,
        DisintegrateArmor = 38,
        Invisibility = 39,
        Chameleon = 40,
        Light = 41,
        Sanctuary = 42,
        NightEye = 43,
        Charm = 44,
        Paralyze = 45,
        Silence = 46,
        Blind = 47,
        Sound = 48,
        CalmHumanoid = 49,
        CalmCreature = 50,
        FrenzyHumanoid = 51,
        FrenzyCreature = 52,
        DemoralizeHumanoid = 53,
        DemoralizeCreature = 54,
        RallyHumanoid = 55,
        RallyCreature = 56,
        Dispel = 57,
        Soultrap = 58,
        Telekinesis = 59,
        Mark = 60,
        Recall = 61,
        DivineIntervention = 62,
        AlmsiviIntervention = 63,
        DetectAnimal = 64,
        DetectEnchantment = 65,
        DetectKey = 66,
        SpellAbsorption = 67,
        Reflect = 68,
        CureCommonDisease = 69,
        CureBlightDisease = 70,
        CureCorprusDisease = 71,
        CurePoison = 72,
        CureParalyzation = 73,
        RestoreAttribute = 74,
        RestoreHealth = 75,
        RestoreMagicka = 76,
        RestoreFatigue = 77,
        RestoreSkill = 78,
        FortifyAttribute = 79,
        FortifyHealth = 80,
        FortifyMagicka= 81,
        FortifyFatigue = 82,
        FortifySkill = 83,
        FortifyMaximumMagicka = 84,
        AbsorbAttribute = 85,
        AbsorbHealth = 86,
        AbsorbMagicka = 87,
        AbsorbFatigue = 88,
        AbsorbSkill = 89,
        ResistFire = 90,
        ResistFrost = 91,
        ResistShock = 92,
        ResistMagicka = 93,
        ResistCommonDisease = 94,
        ResistBlightDisease = 95,
        ResistCorprusDisease = 96,
        ResistPoison = 97,
        ResistNormalWeapons = 98,
        ResistParalysis = 99,
        RemoveCurse = 100,
        TurnUndead = 101,
        SummonScamp = 102,
        SummonClannfear = 103,
        SummonDaedroth = 104,
        SummonDremora = 105,
        SummonAncestralGhost = 106,
        SummonSkeletalMinion = 107,
        SummonBonewalker = 108,
        SummonGreaterBonewalker = 109,
        SummonBonelord = 110,
        SummonWingedTwilight = 111,
        SummonHunger = 112,
        SummonGoldenSaint = 113,
        SummonFlameAtronach = 114,
        SummonFrostAtronach = 115,
        SummonStormAtronach = 116,
        FortifyAttack = 117,
        CommandCreature = 118,
        CommandHumanoid = 119,
        BoundDagger = 120,
        BoundLongsword = 121,
        BoundMace = 122,
        BoundBattleAxe = 123,
        BoundSpear = 124,
        BoundLongbow = 125,
        ExtraSpell = 126,
        BoundCuirass = 127,
        BoundHelm = 128,
        BoundBoots = 129,
        BoundShield = 130,
        BoundGloves = 131,
        Corprus = 132,
        Vampirism = 133,
        SummonCenturionSphere = 134,
        SunDamage = 135,
        StuntedMagicka = 136,

        // Tribunal only
        SummonFabricant = 137,

        // Bloodmoon only
        SummonWolf = 138,
        SummonBear = 139,
        SummonBonewolf = 140,
        SummonCreature04 = 141,
        SummonCreature05 = 142
    };
};
}
#endif
