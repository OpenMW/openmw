#ifndef OPENMW_ESM_MGEF_H
#define OPENMW_ESM_MGEF_H

#include <array>
#include <map>
#include <string>
#include <string_view>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "components/misc/strings/algorithm.hpp"

#include <osg/Vec4>

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    using MagicEffectId = StringRefId;

    struct MagicEffect
    {
        constexpr static RecNameInts sRecordId = REC_MGEF;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "MagicEffect"; }

        uint32_t mRecordFlags;
        MagicEffectId mId;

        enum Flags
        {
            // Originally fixed flags (HardcodedFlags array consists of just these)
            TargetSkill = 0x1, // Affects a specific skill, which is specified elsewhere in the effect structure.
            TargetAttribute
            = 0x2, // Affects a specific attribute, which is specified elsewhere in the effect structure.
            NoDuration = 0x4, // Has no duration. Only runs effect once on cast.
            NoMagnitude = 0x8, // Has no magnitude.
            Harmful = 0x10, // Counts as a negative effect. Interpreted as useful for attack, and is treated as a bad
                            // effect in alchemy.
            ContinuousVfx = 0x20, // The effect's hit particle VFX repeats for the full duration of the spell, rather
                                  // than occuring once on hit.
            CastSelf = 0x40, // Allows range - cast on self.
            CastTouch = 0x80, // Allows range - cast on touch.
            CastTarget = 0x100, // Allows range - cast on target.
            AppliedOnce
            = 0x1000, // An effect that is applied once it lands, instead of continuously. Allows an effect to reduce an
                      // attribute below zero; removes the normal minimum effect duration of 1 second.
            Stealth = 0x2000, // Unused
            NonRecastable = 0x4000, // Does not land if parent spell is already affecting target. Shows "you cannot
                                    // re-cast" message for self target.
            IllegalDaedra = 0x8000, // Unused
            Unreflectable = 0x10000, // Cannot be reflected, the effect always lands normally.
            CasterLinked = 0x20000, // Must quench if caster is dead, or not an NPC/creature. Not allowed in
                                    // containter/door trap spells.

            // Originally modifiable flags
            AllowSpellmaking = 0x200, // Can be used for spellmaking
            AllowEnchanting = 0x400, // Can be used for enchanting
            NegativeLight = 0x800 // Inverts the effect's color
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
            RefId mSchool; // Skill id
            float mBaseCost;
            int32_t mFlags;
            // Glow color for enchanted items with this effect
            int32_t mRed, mGreen, mBlue;

            float mUnknown1; // Called "Size X" in CS
            float mSpeed; // Speed of fired projectile
            float mUnknown2; // Called "Size Cap" in CS
        }; // 36 bytes

        /// Returns the effect that provides resistance against \a effect (or empty RefId if there's none)
        static RefId getResistanceEffect(const MagicEffectId& effect);
        /// Returns the effect that induces weakness against \a effect (or empty RefId if there's none)
        static RefId getWeaknessEffect(const MagicEffectId& effect);

        MagnitudeDisplayType getMagnitudeDisplayType() const;

        MEDTstruct mData;

        std::string mIcon, mParticle; // Textures
        ESM::RefId mCasting, mHit, mArea; // Static
        ESM::RefId mBolt; // Weapon
        ESM::RefId mCastSound, mBoltSound, mHitSound, mAreaSound; // Sounds
        std::string mDescription;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        /// Set record to default state (does not touch the ID/index).
        void blank();

        osg::Vec4f getColor() const;

        static const MagicEffectId WaterBreathing;
        static const MagicEffectId SwiftSwim;
        static const MagicEffectId WaterWalking;
        static const MagicEffectId Shield;
        static const MagicEffectId FireShield;
        static const MagicEffectId LightningShield;
        static const MagicEffectId FrostShield;
        static const MagicEffectId Burden;
        static const MagicEffectId Feather;
        static const MagicEffectId Jump;
        static const MagicEffectId Levitate;
        static const MagicEffectId SlowFall;
        static const MagicEffectId Lock;
        static const MagicEffectId Open;
        static const MagicEffectId FireDamage;
        static const MagicEffectId ShockDamage;
        static const MagicEffectId FrostDamage;
        static const MagicEffectId DrainAttribute;
        static const MagicEffectId DrainHealth;
        static const MagicEffectId DrainMagicka;
        static const MagicEffectId DrainFatigue;
        static const MagicEffectId DrainSkill;
        static const MagicEffectId DamageAttribute;
        static const MagicEffectId DamageHealth;
        static const MagicEffectId DamageMagicka;
        static const MagicEffectId DamageFatigue;
        static const MagicEffectId DamageSkill;
        static const MagicEffectId Poison;
        static const MagicEffectId WeaknessToFire;
        static const MagicEffectId WeaknessToFrost;
        static const MagicEffectId WeaknessToShock;
        static const MagicEffectId WeaknessToMagicka;
        static const MagicEffectId WeaknessToCommonDisease;
        static const MagicEffectId WeaknessToBlightDisease;
        static const MagicEffectId WeaknessToCorprusDisease;
        static const MagicEffectId WeaknessToPoison;
        static const MagicEffectId WeaknessToNormalWeapons;
        static const MagicEffectId DisintegrateWeapon;
        static const MagicEffectId DisintegrateArmor;
        static const MagicEffectId Invisibility;
        static const MagicEffectId Chameleon;
        static const MagicEffectId Light;
        static const MagicEffectId Sanctuary;
        static const MagicEffectId NightEye;
        static const MagicEffectId Charm;
        static const MagicEffectId Paralyze;
        static const MagicEffectId Silence;
        static const MagicEffectId Blind;
        static const MagicEffectId Sound;
        static const MagicEffectId CalmHumanoid;
        static const MagicEffectId CalmCreature;
        static const MagicEffectId FrenzyHumanoid;
        static const MagicEffectId FrenzyCreature;
        static const MagicEffectId DemoralizeHumanoid;
        static const MagicEffectId DemoralizeCreature;
        static const MagicEffectId RallyHumanoid;
        static const MagicEffectId RallyCreature;
        static const MagicEffectId Dispel;
        static const MagicEffectId Soultrap;
        static const MagicEffectId Telekinesis;
        static const MagicEffectId Mark;
        static const MagicEffectId Recall;
        static const MagicEffectId DivineIntervention;
        static const MagicEffectId AlmsiviIntervention;
        static const MagicEffectId DetectAnimal;
        static const MagicEffectId DetectEnchantment;
        static const MagicEffectId DetectKey;
        static const MagicEffectId SpellAbsorption;
        static const MagicEffectId Reflect;
        static const MagicEffectId CureCommonDisease;
        static const MagicEffectId CureBlightDisease;
        static const MagicEffectId CureCorprusDisease;
        static const MagicEffectId CurePoison;
        static const MagicEffectId CureParalyzation;
        static const MagicEffectId RestoreAttribute;
        static const MagicEffectId RestoreHealth;
        static const MagicEffectId RestoreMagicka;
        static const MagicEffectId RestoreFatigue;
        static const MagicEffectId RestoreSkill;
        static const MagicEffectId FortifyAttribute;
        static const MagicEffectId FortifyHealth;
        static const MagicEffectId FortifyMagicka;
        static const MagicEffectId FortifyFatigue;
        static const MagicEffectId FortifySkill;
        static const MagicEffectId FortifyMaximumMagicka;
        static const MagicEffectId AbsorbAttribute;
        static const MagicEffectId AbsorbHealth;
        static const MagicEffectId AbsorbMagicka;
        static const MagicEffectId AbsorbFatigue;
        static const MagicEffectId AbsorbSkill;
        static const MagicEffectId ResistFire;
        static const MagicEffectId ResistFrost;
        static const MagicEffectId ResistShock;
        static const MagicEffectId ResistMagicka;
        static const MagicEffectId ResistCommonDisease;
        static const MagicEffectId ResistBlightDisease;
        static const MagicEffectId ResistCorprusDisease;
        static const MagicEffectId ResistPoison;
        static const MagicEffectId ResistNormalWeapons;
        static const MagicEffectId ResistParalysis;
        static const MagicEffectId RemoveCurse;
        static const MagicEffectId TurnUndead;
        static const MagicEffectId SummonScamp;
        static const MagicEffectId SummonClannfear;
        static const MagicEffectId SummonDaedroth;
        static const MagicEffectId SummonDremora;
        static const MagicEffectId SummonAncestralGhost;
        static const MagicEffectId SummonSkeletalMinion;
        static const MagicEffectId SummonBonewalker;
        static const MagicEffectId SummonGreaterBonewalker;
        static const MagicEffectId SummonBonelord;
        static const MagicEffectId SummonWingedTwilight;
        static const MagicEffectId SummonHunger;
        static const MagicEffectId SummonGoldenSaint;
        static const MagicEffectId SummonFlameAtronach;
        static const MagicEffectId SummonFrostAtronach;
        static const MagicEffectId SummonStormAtronach;
        static const MagicEffectId FortifyAttack;
        static const MagicEffectId CommandCreature;
        static const MagicEffectId CommandHumanoid;
        static const MagicEffectId BoundDagger;
        static const MagicEffectId BoundLongsword;
        static const MagicEffectId BoundMace;
        static const MagicEffectId BoundBattleAxe;
        static const MagicEffectId BoundSpear;
        static const MagicEffectId BoundLongbow;
        static const MagicEffectId ExtraSpell;
        static const MagicEffectId BoundCuirass;
        static const MagicEffectId BoundHelm;
        static const MagicEffectId BoundBoots;
        static const MagicEffectId BoundShield;
        static const MagicEffectId BoundGloves;
        static const MagicEffectId Corprus;
        static const MagicEffectId Vampirism;
        static const MagicEffectId SummonCenturionSphere;
        static const MagicEffectId SunDamage;
        static const MagicEffectId StuntedMagicka;

        // Tribunal only
        static const MagicEffectId SummonFabricant;

        // Bloodmoon only
        static const MagicEffectId SummonWolf;
        static const MagicEffectId SummonBear;
        static const MagicEffectId SummonBonewolf;
        static const MagicEffectId SummonCreature04;
        static const MagicEffectId SummonCreature05;

        static constexpr short Length = 143;

        static const std::array<std::string, Length> sGmstEffectIds;
        static const std::map<std::string_view, short, Misc::StringUtils::CiComp> sGmstEffectIdToIndexMap;

        static const std::string& indexToGmstString(short effectID);
        static short effectGmstIdToIndex(std::string_view gmstId);

        static RefId indexToRefId(short index);
        static short refIdToIndex(const RefId& effectId);
    };
}
#endif
