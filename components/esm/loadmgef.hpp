#ifndef OPENMW_ESM_MGEF_H
#define OPENMW_ESM_MGEF_H

#include <string>
#include <map>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct MagicEffect
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "MagicEffect"; }

    std::string mId;

    enum Flags
    {
        // Originally fixed flags (HardcodedFlags array consists of just these)
        TargetSkill = 0x1, // Affects a specific skill, which is specified elsewhere in the effect structure.
        TargetAttribute = 0x2, // Affects a specific attribute, which is specified elsewhere in the effect structure.
        NoDuration = 0x4, // Has no duration. Only runs effect once on cast.
        NoMagnitude = 0x8, // Has no magnitude.
        Harmful = 0x10, // Counts as a negative effect. Interpreted as useful for attack, and is treated as a bad effect in alchemy.
        ContinuousVfx = 0x20, // The effect's hit particle VFX repeats for the full duration of the spell, rather than occuring once on hit.
        CastSelf = 0x40, // Allows range - cast on self.
        CastTouch = 0x80, // Allows range - cast on touch.
        CastTarget = 0x100, // Allows range - cast on target.
        AppliedOnce = 0x1000, // An effect that is applied once it lands, instead of continuously. Allows an effect to reduce an attribute below zero; removes the normal minimum effect duration of 1 second.
        Stealth = 0x2000, // Unused
        NonRecastable = 0x4000, // Does not land if parent spell is already affecting target. Shows "you cannot re-cast" message for self target.
        IllegalDaedra = 0x8000, // Unused
        Unreflectable = 0x10000, // Cannot be reflected, the effect always lands normally.
        CasterLinked = 0x20000, // Must quench if caster is dead, or not an NPC/creature. Not allowed in containter/door trap spells.

        // Originally modifiable flags
        AllowSpellmaking = 0x200, // Can be used for spellmaking
        AllowEnchanting = 0x400, // Can be used for enchanting
        NegativeLight = 0x800 // Unused
    };

    enum MagnitudeDisplayType
    {
        MDT_None,
        MDT_Feet,
        MDT_Level,
        MDT_Percentage,
        MDT_Points,
        MDT_TimesInt
    };

    struct MEDTstruct
    {
        int mSchool; // SpellSchool, see defs.hpp
        float mBaseCost;
        int mFlags;
        // Glow color for enchanted items with this effect
        int mRed, mGreen, mBlue;

        float mUnknown1; // Called "Size X" in CS
        float mSpeed; // Speed of fired projectile
        float mUnknown2; // Called "Size Cap" in CS
    }; // 36 bytes

    static const std::map<short,std::string> sNames;

    static const std::string &effectIdToString(short effectID);
    static short effectStringToId(const std::string &effect);

    /// Returns the effect that provides resistance against \a effect (or -1 if there's none)
    static short getResistanceEffect(short effect);
    /// Returns the effect that induces weakness against \a effect (or -1 if there's none)
    static short getWeaknessEffect(short effect);

    MagnitudeDisplayType getMagnitudeDisplayType() const;


    MEDTstruct mData;

    std::string mIcon, mParticle; // Textures
    std::string mCasting, mHit, mArea; // ESM::Static
    std::string mBolt; // ESM::Weapon
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

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

     /// Set record to default state (does not touch the ID/index).
    void blank();

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
        SummonCreature05 = 142,

        Length
    };

    static std::string indexToId (int index);
};
}
#endif
