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

    struct MagicEffect
    {
        constexpr static RecNameInts sRecordId = REC_MGEF;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "MagicEffect"; }

        uint32_t mRecordFlags;
        RefId mId;

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
        static RefId getResistanceEffect(RefId effect);
        /// Returns the effect that induces weakness against \a effect (or empty RefId if there's none)
        static RefId getWeaknessEffect(RefId effect);

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

        static const StringRefId WaterBreathing;
        static const StringRefId SwiftSwim;
        static const StringRefId WaterWalking;
        static const StringRefId Shield;
        static const StringRefId FireShield;
        static const StringRefId LightningShield;
        static const StringRefId FrostShield;
        static const StringRefId Burden;
        static const StringRefId Feather;
        static const StringRefId Jump;
        static const StringRefId Levitate;
        static const StringRefId SlowFall;
        static const StringRefId Lock;
        static const StringRefId Open;
        static const StringRefId FireDamage;
        static const StringRefId ShockDamage;
        static const StringRefId FrostDamage;
        static const StringRefId DrainAttribute;
        static const StringRefId DrainHealth;
        static const StringRefId DrainMagicka;
        static const StringRefId DrainFatigue;
        static const StringRefId DrainSkill;
        static const StringRefId DamageAttribute;
        static const StringRefId DamageHealth;
        static const StringRefId DamageMagicka;
        static const StringRefId DamageFatigue;
        static const StringRefId DamageSkill;
        static const StringRefId Poison;
        static const StringRefId WeaknessToFire;
        static const StringRefId WeaknessToFrost;
        static const StringRefId WeaknessToShock;
        static const StringRefId WeaknessToMagicka;
        static const StringRefId WeaknessToCommonDisease;
        static const StringRefId WeaknessToBlightDisease;
        static const StringRefId WeaknessToCorprusDisease;
        static const StringRefId WeaknessToPoison;
        static const StringRefId WeaknessToNormalWeapons;
        static const StringRefId DisintegrateWeapon;
        static const StringRefId DisintegrateArmor;
        static const StringRefId Invisibility;
        static const StringRefId Chameleon;
        static const StringRefId Light;
        static const StringRefId Sanctuary;
        static const StringRefId NightEye;
        static const StringRefId Charm;
        static const StringRefId Paralyze;
        static const StringRefId Silence;
        static const StringRefId Blind;
        static const StringRefId Sound;
        static const StringRefId CalmHumanoid;
        static const StringRefId CalmCreature;
        static const StringRefId FrenzyHumanoid;
        static const StringRefId FrenzyCreature;
        static const StringRefId DemoralizeHumanoid;
        static const StringRefId DemoralizeCreature;
        static const StringRefId RallyHumanoid;
        static const StringRefId RallyCreature;
        static const StringRefId Dispel;
        static const StringRefId Soultrap;
        static const StringRefId Telekinesis;
        static const StringRefId Mark;
        static const StringRefId Recall;
        static const StringRefId DivineIntervention;
        static const StringRefId AlmsiviIntervention;
        static const StringRefId DetectAnimal;
        static const StringRefId DetectEnchantment;
        static const StringRefId DetectKey;
        static const StringRefId SpellAbsorption;
        static const StringRefId Reflect;
        static const StringRefId CureCommonDisease;
        static const StringRefId CureBlightDisease;
        static const StringRefId CureCorprusDisease;
        static const StringRefId CurePoison;
        static const StringRefId CureParalyzation;
        static const StringRefId RestoreAttribute;
        static const StringRefId RestoreHealth;
        static const StringRefId RestoreMagicka;
        static const StringRefId RestoreFatigue;
        static const StringRefId RestoreSkill;
        static const StringRefId FortifyAttribute;
        static const StringRefId FortifyHealth;
        static const StringRefId FortifyMagicka;
        static const StringRefId FortifyFatigue;
        static const StringRefId FortifySkill;
        static const StringRefId FortifyMaximumMagicka;
        static const StringRefId AbsorbAttribute;
        static const StringRefId AbsorbHealth;
        static const StringRefId AbsorbMagicka;
        static const StringRefId AbsorbFatigue;
        static const StringRefId AbsorbSkill;
        static const StringRefId ResistFire;
        static const StringRefId ResistFrost;
        static const StringRefId ResistShock;
        static const StringRefId ResistMagicka;
        static const StringRefId ResistCommonDisease;
        static const StringRefId ResistBlightDisease;
        static const StringRefId ResistCorprusDisease;
        static const StringRefId ResistPoison;
        static const StringRefId ResistNormalWeapons;
        static const StringRefId ResistParalysis;
        static const StringRefId RemoveCurse;
        static const StringRefId TurnUndead;
        static const StringRefId SummonScamp;
        static const StringRefId SummonClannfear;
        static const StringRefId SummonDaedroth;
        static const StringRefId SummonDremora;
        static const StringRefId SummonAncestralGhost;
        static const StringRefId SummonSkeletalMinion;
        static const StringRefId SummonBonewalker;
        static const StringRefId SummonGreaterBonewalker;
        static const StringRefId SummonBonelord;
        static const StringRefId SummonWingedTwilight;
        static const StringRefId SummonHunger;
        static const StringRefId SummonGoldenSaint;
        static const StringRefId SummonFlameAtronach;
        static const StringRefId SummonFrostAtronach;
        static const StringRefId SummonStormAtronach;
        static const StringRefId FortifyAttack;
        static const StringRefId CommandCreature;
        static const StringRefId CommandHumanoid;
        static const StringRefId BoundDagger;
        static const StringRefId BoundLongsword;
        static const StringRefId BoundMace;
        static const StringRefId BoundBattleAxe;
        static const StringRefId BoundSpear;
        static const StringRefId BoundLongbow;
        static const StringRefId ExtraSpell;
        static const StringRefId BoundCuirass;
        static const StringRefId BoundHelm;
        static const StringRefId BoundBoots;
        static const StringRefId BoundShield;
        static const StringRefId BoundGloves;
        static const StringRefId Corprus;
        static const StringRefId Vampirism;
        static const StringRefId SummonCenturionSphere;
        static const StringRefId SunDamage;
        static const StringRefId StuntedMagicka;

        // Tribunal only
        static const StringRefId SummonFabricant;

        // Bloodmoon only
        static const StringRefId SummonWolf;
        static const StringRefId SummonBear;
        static const StringRefId SummonBonewolf;
        static const StringRefId SummonCreature04;
        static const StringRefId SummonCreature05;

        static constexpr int Length = 143;

        static std::string_view refIdToGmstString(RefId effectId);
        static RefId effectGmstIdToRefId(std::string_view gmstId);

        static RefId indexToRefId(int index);
        static int refIdToIndex(RefId effectId);

        static std::string_view indexToName(int index);
    };
}
#endif
