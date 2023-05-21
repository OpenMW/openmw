#include "loadmgef.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
    namespace
    {
        const int NumberOfHardcodedFlags = 143;
        const int HardcodedFlags[NumberOfHardcodedFlags] = { 0x11c8, 0x11c0, 0x11c8, 0x11e0, 0x11e0, 0x11e0, 0x11e0,
            0x11d0, 0x11c0, 0x11c0, 0x11e0, 0x11c0, 0x11184, 0x11184, 0x1f0, 0x1f0, 0x1f0, 0x11d2, 0x11f0, 0x11d0,
            0x11d0, 0x11d1, 0x1d2, 0x1f0, 0x1d0, 0x1d0, 0x1d1, 0x1f0, 0x11d0, 0x11d0, 0x11d0, 0x11d0, 0x11d0, 0x11d0,
            0x11d0, 0x11d0, 0x11d0, 0x1d0, 0x1d0, 0x11c8, 0x31c0, 0x11c0, 0x11c0, 0x11c0, 0x1180, 0x11d8, 0x11d8,
            0x11d0, 0x11d0, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11c4, 0x111b8,
            0x1040, 0x104c, 0x104c, 0x104c, 0x104c, 0x1040, 0x1040, 0x1040, 0x11c0, 0x11c0, 0x1cc, 0x1cc, 0x1cc, 0x1cc,
            0x1cc, 0x1c2, 0x1c0, 0x1c0, 0x1c0, 0x1c1, 0x11c2, 0x11c0, 0x11c0, 0x11c0, 0x11c1, 0x11c0, 0x21192, 0x20190,
            0x20190, 0x20190, 0x21191, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0,
            0x1c0, 0x11190, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048,
            0x9048, 0x9048, 0x9048, 0x9048, 0x11c0, 0x1180, 0x1180, 0x5048, 0x5048, 0x5048, 0x5048, 0x5048, 0x5048,
            0x1188, 0x5048, 0x5048, 0x5048, 0x5048, 0x5048, 0x1048, 0x104c, 0x1048, 0x40, 0x11c8, 0x1048, 0x1048,
            0x1048, 0x1048, 0x1048, 0x1048 };
    }

    const std::string_view MagicEffect::sIndexNames[MagicEffect::Length] = {
        "WaterBreathing",
        "SwiftSwim",
        "WaterWalking",
        "Shield",
        "FireShield",
        "LightningShield",
        "FrostShield",
        "Burden",
        "Feather",
        "Jump",
        "Levitate",
        "SlowFall",
        "Lock",
        "Open",
        "FireDamage",
        "ShockDamage",
        "FrostDamage",
        "DrainAttribute",
        "DrainHealth",
        "DrainMagicka",
        "DrainFatigue",
        "DrainSkill",
        "DamageAttribute",
        "DamageHealth",
        "DamageMagicka",
        "DamageFatigue",
        "DamageSkill",
        "Poison",
        "WeaknessToFire",
        "WeaknessToFrost",
        "WeaknessToShock",
        "WeaknessToMagicka",
        "WeaknessToCommonDisease",
        "WeaknessToBlightDisease",
        "WeaknessToCorprusDisease",
        "WeaknessToPoison",
        "WeaknessToNormalWeapons",
        "DisintegrateWeapon",
        "DisintegrateArmor",
        "Invisibility",
        "Chameleon",
        "Light",
        "Sanctuary",
        "NightEye",
        "Charm",
        "Paralyze",
        "Silence",
        "Blind",
        "Sound",
        "CalmHumanoid",
        "CalmCreature",
        "FrenzyHumanoid",
        "FrenzyCreature",
        "DemoralizeHumanoid",
        "DemoralizeCreature",
        "RallyHumanoid",
        "RallyCreature",
        "Dispel",
        "Soultrap",
        "Telekinesis",
        "Mark",
        "Recall",
        "DivineIntervention",
        "AlmsiviIntervention",
        "DetectAnimal",
        "DetectEnchantment",
        "DetectKey",
        "SpellAbsorption",
        "Reflect",
        "CureCommonDisease",
        "CureBlightDisease",
        "CureCorprusDisease",
        "CurePoison",
        "CureParalyzation",
        "RestoreAttribute",
        "RestoreHealth",
        "RestoreMagicka",
        "RestoreFatigue",
        "RestoreSkill",
        "FortifyAttribute",
        "FortifyHealth",
        "FortifyMagicka",
        "FortifyFatigue",
        "FortifySkill",
        "FortifyMaximumMagicka",
        "AbsorbAttribute",
        "AbsorbHealth",
        "AbsorbMagicka",
        "AbsorbFatigue",
        "AbsorbSkill",
        "ResistFire",
        "ResistFrost",
        "ResistShock",
        "ResistMagicka",
        "ResistCommonDisease",
        "ResistBlightDisease",
        "ResistCorprusDisease",
        "ResistPoison",
        "ResistNormalWeapons",
        "ResistParalysis",
        "RemoveCurse",
        "TurnUndead",
        "SummonScamp",
        "SummonClannfear",
        "SummonDaedroth",
        "SummonDremora",
        "SummonAncestralGhost",
        "SummonSkeletalMinion",
        "SummonBonewalker",
        "SummonGreaterBonewalker",
        "SummonBonelord",
        "SummonWingedTwilight",
        "SummonHunger",
        "SummonGoldenSaint",
        "SummonFlameAtronach",
        "SummonFrostAtronach",
        "SummonStormAtronach",
        "FortifyAttack",
        "CommandCreature",
        "CommandHumanoid",
        "BoundDagger",
        "BoundLongsword",
        "BoundMace",
        "BoundBattleAxe",
        "BoundSpear",
        "BoundLongbow",
        "ExtraSpell",
        "BoundCuirass",
        "BoundHelm",
        "BoundBoots",
        "BoundShield",
        "BoundGloves",
        "Corprus",
        "Vampirism",
        "SummonCenturionSphere",
        "SunDamage",
        "StuntedMagicka",

        // Tribunal only
        "SummonFabricant",

        // Bloodmoon only
        "SummonWolf",
        "SummonBear",
        "SummonBonewolf",
        "SummonCreature04",
        "SummonCreature05",
    };

    void MagicEffect::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // MagicEffect record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        esm.getHNT(mIndex, "INDX");

        mId = indexToRefId(mIndex);

        esm.getHNTSized<36>(mData, "MEDT");
        if (esm.getFormatVersion() == DefaultFormatVersion)
        {
            // don't allow mods to change fixed flags in the legacy format
            mData.mFlags &= (AllowSpellmaking | AllowEnchanting | NegativeLight);
            if (mIndex >= 0 && mIndex < NumberOfHardcodedFlags)
                mData.mFlags |= HardcodedFlags[mIndex];
        }

        // vanilla MW accepts the _SND subrecords before or after DESC... I hope
        // this isn't true for other records, or we have to do a mass-refactor
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case fourCC("PTEX"):
                    mParticle = esm.getHString();
                    break;
                case fourCC("BSND"):
                    mBoltSound = esm.getRefId();
                    break;
                case fourCC("CSND"):
                    mCastSound = esm.getRefId();
                    break;
                case fourCC("HSND"):
                    mHitSound = esm.getRefId();
                    break;
                case fourCC("ASND"):
                    mAreaSound = esm.getRefId();
                    break;
                case fourCC("CVFX"):
                    mCasting = esm.getRefId();
                    break;
                case fourCC("BVFX"):
                    mBolt = esm.getRefId();
                    break;
                case fourCC("HVFX"):
                    mHit = esm.getRefId();
                    break;
                case fourCC("AVFX"):
                    mArea = esm.getRefId();
                    break;
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }
    }
    void MagicEffect::save(ESMWriter& esm, bool /*isDeleted*/) const
    {
        esm.writeHNT("INDX", mIndex);

        esm.writeHNT("MEDT", mData, 36);

        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNOCString("PTEX", mParticle);
        esm.writeHNOCRefId("BSND", mBoltSound);
        esm.writeHNOCRefId("CSND", mCastSound);
        esm.writeHNOCRefId("HSND", mHitSound);
        esm.writeHNOCRefId("ASND", mAreaSound);

        esm.writeHNOCRefId("CVFX", mCasting);
        esm.writeHNOCRefId("BVFX", mBolt);
        esm.writeHNOCRefId("HVFX", mHit);
        esm.writeHNOCRefId("AVFX", mArea);

        esm.writeHNOString("DESC", mDescription);
    }

    short MagicEffect::getResistanceEffect(short effect)
    {
        // Source https://wiki.openmw.org/index.php?title=Research:Magic#Effect_attribute

        // <Effect, Effect providing resistance against first effect>
        static std::map<short, short> effects;
        if (effects.empty())
        {
            effects[DisintegrateArmor] = Sanctuary;
            effects[DisintegrateWeapon] = Sanctuary;

            for (int i = DrainAttribute; i <= DamageSkill; ++i)
                effects[i] = ResistMagicka;
            for (int i = AbsorbAttribute; i <= AbsorbSkill; ++i)
                effects[i] = ResistMagicka;
            for (int i = WeaknessToFire; i <= WeaknessToNormalWeapons; ++i)
                effects[i] = ResistMagicka;

            effects[Burden] = ResistMagicka;
            effects[Charm] = ResistMagicka;
            effects[Silence] = ResistMagicka;
            effects[Blind] = ResistMagicka;
            effects[Sound] = ResistMagicka;

            for (int i = 0; i < 2; ++i)
            {
                effects[CalmHumanoid + i] = ResistMagicka;
                effects[FrenzyHumanoid + i] = ResistMagicka;
                effects[DemoralizeHumanoid + i] = ResistMagicka;
                effects[RallyHumanoid + i] = ResistMagicka;
            }

            effects[TurnUndead] = ResistMagicka;

            effects[FireDamage] = ResistFire;
            effects[FrostDamage] = ResistFrost;
            effects[ShockDamage] = ResistShock;
            effects[Vampirism] = ResistCommonDisease;
            effects[Corprus] = ResistCorprusDisease;
            effects[Poison] = ResistPoison;
            effects[Paralyze] = ResistParalysis;
        }

        if (effects.find(effect) != effects.end())
            return effects[effect];
        else
            return -1;
    }

    short MagicEffect::getWeaknessEffect(short effect)
    {
        static std::map<short, short> effects;
        if (effects.empty())
        {
            for (int i = DrainAttribute; i <= DamageSkill; ++i)
                effects[i] = WeaknessToMagicka;
            for (int i = AbsorbAttribute; i <= AbsorbSkill; ++i)
                effects[i] = WeaknessToMagicka;
            for (int i = WeaknessToFire; i <= WeaknessToNormalWeapons; ++i)
                effects[i] = WeaknessToMagicka;

            effects[Burden] = WeaknessToMagicka;
            effects[Charm] = WeaknessToMagicka;
            effects[Silence] = WeaknessToMagicka;
            effects[Blind] = WeaknessToMagicka;
            effects[Sound] = WeaknessToMagicka;

            for (int i = 0; i < 2; ++i)
            {
                effects[CalmHumanoid + i] = WeaknessToMagicka;
                effects[FrenzyHumanoid + i] = WeaknessToMagicka;
                effects[DemoralizeHumanoid + i] = WeaknessToMagicka;
                effects[RallyHumanoid + i] = WeaknessToMagicka;
            }

            effects[TurnUndead] = WeaknessToMagicka;

            effects[FireDamage] = WeaknessToFire;
            effects[FrostDamage] = WeaknessToFrost;
            effects[ShockDamage] = WeaknessToShock;
            effects[Vampirism] = WeaknessToCommonDisease;
            effects[Corprus] = WeaknessToCorprusDisease;
            effects[Poison] = WeaknessToPoison;

            effects[Paralyze] = -1;
        }

        if (effects.find(effect) != effects.end())
            return effects[effect];
        else
            return -1;
    }

    // Map effect ID to GMST name
    const std::map<int, std::string> MagicEffect::sGmstEffectIds = {
        { MagicEffect::AbsorbAttribute, "sEffectAbsorbAttribute" },
        { MagicEffect::AbsorbFatigue, "sEffectAbsorbFatigue" },
        { MagicEffect::AbsorbHealth, "sEffectAbsorbHealth" },
        { MagicEffect::AbsorbMagicka, "sEffectAbsorbSpellPoints" },
        { MagicEffect::AbsorbSkill, "sEffectAbsorbSkill" },
        { MagicEffect::AlmsiviIntervention, "sEffectAlmsiviIntervention" },
        { MagicEffect::Blind, "sEffectBlind" },
        { MagicEffect::BoundBattleAxe, "sEffectBoundBattleAxe" },
        { MagicEffect::BoundBoots, "sEffectBoundBoots" },
        { MagicEffect::BoundCuirass, "sEffectBoundCuirass" },
        { MagicEffect::BoundDagger, "sEffectBoundDagger" },
        { MagicEffect::BoundGloves, "sEffectBoundGloves" },
        { MagicEffect::BoundHelm, "sEffectBoundHelm" },
        { MagicEffect::BoundLongbow, "sEffectBoundLongbow" },
        { MagicEffect::ExtraSpell, "sEffectExtraSpell" },
        { MagicEffect::BoundLongsword, "sEffectBoundLongsword" },
        { MagicEffect::BoundMace, "sEffectBoundMace" },
        { MagicEffect::BoundShield, "sEffectBoundShield" },
        { MagicEffect::BoundSpear, "sEffectBoundSpear" },
        { MagicEffect::Burden, "sEffectBurden" },
        { MagicEffect::CalmCreature, "sEffectCalmCreature" },
        { MagicEffect::CalmHumanoid, "sEffectCalmHumanoid" },
        { MagicEffect::Chameleon, "sEffectChameleon" },
        { MagicEffect::Charm, "sEffectCharm" },
        { MagicEffect::CommandCreature, "sEffectCommandCreatures" },
        { MagicEffect::CommandHumanoid, "sEffectCommandHumanoids" },
        { MagicEffect::Corprus, "sEffectCorpus" }, // NB this typo. (bethesda made it)
        { MagicEffect::CureBlightDisease, "sEffectCureBlightDisease" },
        { MagicEffect::CureCommonDisease, "sEffectCureCommonDisease" },
        { MagicEffect::CureCorprusDisease, "sEffectCureCorprusDisease" },
        { MagicEffect::CureParalyzation, "sEffectCureParalyzation" },
        { MagicEffect::CurePoison, "sEffectCurePoison" },
        { MagicEffect::DamageAttribute, "sEffectDamageAttribute" },
        { MagicEffect::DamageFatigue, "sEffectDamageFatigue" },
        { MagicEffect::DamageHealth, "sEffectDamageHealth" },
        { MagicEffect::DamageMagicka, "sEffectDamageMagicka" },
        { MagicEffect::DamageSkill, "sEffectDamageSkill" },
        { MagicEffect::DemoralizeCreature, "sEffectDemoralizeCreature" },
        { MagicEffect::DemoralizeHumanoid, "sEffectDemoralizeHumanoid" },
        { MagicEffect::DetectAnimal, "sEffectDetectAnimal" },
        { MagicEffect::DetectEnchantment, "sEffectDetectEnchantment" },
        { MagicEffect::DetectKey, "sEffectDetectKey" },
        { MagicEffect::DisintegrateArmor, "sEffectDisintegrateArmor" },
        { MagicEffect::DisintegrateWeapon, "sEffectDisintegrateWeapon" },
        { MagicEffect::Dispel, "sEffectDispel" },
        { MagicEffect::DivineIntervention, "sEffectDivineIntervention" },
        { MagicEffect::DrainAttribute, "sEffectDrainAttribute" },
        { MagicEffect::DrainFatigue, "sEffectDrainFatigue" },
        { MagicEffect::DrainHealth, "sEffectDrainHealth" },
        { MagicEffect::DrainMagicka, "sEffectDrainSpellpoints" },
        { MagicEffect::DrainSkill, "sEffectDrainSkill" },
        { MagicEffect::Feather, "sEffectFeather" },
        { MagicEffect::FireDamage, "sEffectFireDamage" },
        { MagicEffect::FireShield, "sEffectFireShield" },
        { MagicEffect::FortifyAttack, "sEffectFortifyAttackBonus" },
        { MagicEffect::FortifyAttribute, "sEffectFortifyAttribute" },
        { MagicEffect::FortifyFatigue, "sEffectFortifyFatigue" },
        { MagicEffect::FortifyHealth, "sEffectFortifyHealth" },
        { MagicEffect::FortifyMagicka, "sEffectFortifySpellpoints" },
        { MagicEffect::FortifyMaximumMagicka, "sEffectFortifyMagickaMultiplier" },
        { MagicEffect::FortifySkill, "sEffectFortifySkill" },
        { MagicEffect::FrenzyCreature, "sEffectFrenzyCreature" },
        { MagicEffect::FrenzyHumanoid, "sEffectFrenzyHumanoid" },
        { MagicEffect::FrostDamage, "sEffectFrostDamage" },
        { MagicEffect::FrostShield, "sEffectFrostShield" },
        { MagicEffect::Invisibility, "sEffectInvisibility" },
        { MagicEffect::Jump, "sEffectJump" },
        { MagicEffect::Levitate, "sEffectLevitate" },
        { MagicEffect::Light, "sEffectLight" },
        { MagicEffect::LightningShield, "sEffectLightningShield" },
        { MagicEffect::Lock, "sEffectLock" },
        { MagicEffect::Mark, "sEffectMark" },
        { MagicEffect::NightEye, "sEffectNightEye" },
        { MagicEffect::Open, "sEffectOpen" },
        { MagicEffect::Paralyze, "sEffectParalyze" },
        { MagicEffect::Poison, "sEffectPoison" },
        { MagicEffect::RallyCreature, "sEffectRallyCreature" },
        { MagicEffect::RallyHumanoid, "sEffectRallyHumanoid" },
        { MagicEffect::Recall, "sEffectRecall" },
        { MagicEffect::Reflect, "sEffectReflect" },
        { MagicEffect::RemoveCurse, "sEffectRemoveCurse" },
        { MagicEffect::ResistBlightDisease, "sEffectResistBlightDisease" },
        { MagicEffect::ResistCommonDisease, "sEffectResistCommonDisease" },
        { MagicEffect::ResistCorprusDisease, "sEffectResistCorprusDisease" },
        { MagicEffect::ResistFire, "sEffectResistFire" },
        { MagicEffect::ResistFrost, "sEffectResistFrost" },
        { MagicEffect::ResistMagicka, "sEffectResistMagicka" },
        { MagicEffect::ResistNormalWeapons, "sEffectResistNormalWeapons" },
        { MagicEffect::ResistParalysis, "sEffectResistParalysis" },
        { MagicEffect::ResistPoison, "sEffectResistPoison" },
        { MagicEffect::ResistShock, "sEffectResistShock" },
        { MagicEffect::RestoreAttribute, "sEffectRestoreAttribute" },
        { MagicEffect::RestoreFatigue, "sEffectRestoreFatigue" },
        { MagicEffect::RestoreHealth, "sEffectRestoreHealth" },
        { MagicEffect::RestoreMagicka, "sEffectRestoreSpellPoints" },
        { MagicEffect::RestoreSkill, "sEffectRestoreSkill" },
        { MagicEffect::Sanctuary, "sEffectSanctuary" },
        { MagicEffect::Shield, "sEffectShield" },
        { MagicEffect::ShockDamage, "sEffectShockDamage" },
        { MagicEffect::Silence, "sEffectSilence" },
        { MagicEffect::SlowFall, "sEffectSlowFall" },
        { MagicEffect::Soultrap, "sEffectSoultrap" },
        { MagicEffect::Sound, "sEffectSound" },
        { MagicEffect::SpellAbsorption, "sEffectSpellAbsorption" },
        { MagicEffect::StuntedMagicka, "sEffectStuntedMagicka" },
        { MagicEffect::SummonAncestralGhost, "sEffectSummonAncestralGhost" },
        { MagicEffect::SummonBonelord, "sEffectSummonBonelord" },
        { MagicEffect::SummonBonewalker, "sEffectSummonLeastBonewalker" },
        { MagicEffect::SummonCenturionSphere, "sEffectSummonCenturionSphere" },
        { MagicEffect::SummonClannfear, "sEffectSummonClannfear" },
        { MagicEffect::SummonDaedroth, "sEffectSummonDaedroth" },
        { MagicEffect::SummonDremora, "sEffectSummonDremora" },
        { MagicEffect::SummonFlameAtronach, "sEffectSummonFlameAtronach" },
        { MagicEffect::SummonFrostAtronach, "sEffectSummonFrostAtronach" },
        { MagicEffect::SummonGoldenSaint, "sEffectSummonGoldenSaint" },
        { MagicEffect::SummonGreaterBonewalker, "sEffectSummonGreaterBonewalker" },
        { MagicEffect::SummonHunger, "sEffectSummonHunger" },
        { MagicEffect::SummonScamp, "sEffectSummonScamp" },
        { MagicEffect::SummonSkeletalMinion, "sEffectSummonSkeletalMinion" },
        { MagicEffect::SummonStormAtronach, "sEffectSummonStormAtronach" },
        { MagicEffect::SummonWingedTwilight, "sEffectSummonWingedTwilight" },
        { MagicEffect::SunDamage, "sEffectSunDamage" },
        { MagicEffect::SwiftSwim, "sEffectSwiftSwim" },
        { MagicEffect::Telekinesis, "sEffectTelekinesis" },
        { MagicEffect::TurnUndead, "sEffectTurnUndead" },
        { MagicEffect::Vampirism, "sEffectVampirism" },
        { MagicEffect::WaterBreathing, "sEffectWaterBreathing" },
        { MagicEffect::WaterWalking, "sEffectWaterWalking" },
        { MagicEffect::WeaknessToBlightDisease, "sEffectWeaknesstoBlightDisease" },
        { MagicEffect::WeaknessToCommonDisease, "sEffectWeaknesstoCommonDisease" },
        { MagicEffect::WeaknessToCorprusDisease, "sEffectWeaknesstoCorprusDisease" },
        { MagicEffect::WeaknessToFire, "sEffectWeaknesstoFire" },
        { MagicEffect::WeaknessToFrost, "sEffectWeaknesstoFrost" },
        { MagicEffect::WeaknessToMagicka, "sEffectWeaknesstoMagicka" },
        { MagicEffect::WeaknessToNormalWeapons, "sEffectWeaknesstoNormalWeapons" },
        { MagicEffect::WeaknessToPoison, "sEffectWeaknesstoPoison" },
        { MagicEffect::WeaknessToShock, "sEffectWeaknesstoShock" },

        // bloodmoon
        { MagicEffect::SummonWolf, "sEffectSummonCreature01" },
        { MagicEffect::SummonBear, "sEffectSummonCreature02" },
        { MagicEffect::SummonBonewolf, "sEffectSummonCreature03" },
        { MagicEffect::SummonCreature04, "sEffectSummonCreature04" },
        { MagicEffect::SummonCreature05, "sEffectSummonCreature05" },

        // tribunal
        { MagicEffect::SummonFabricant, "sEffectSummonFabricant" },
    };

    // Map effect ID to identifying name
    const std::map<int, std::string> MagicEffect::sEffectNames = {
        { MagicEffect::AbsorbAttribute, "AbsorbAttribute" },
        { MagicEffect::AbsorbFatigue, "AbsorbFatigue" },
        { MagicEffect::AbsorbHealth, "AbsorbHealth" },
        { MagicEffect::AbsorbMagicka, "AbsorbMagicka" },
        { MagicEffect::AbsorbSkill, "AbsorbSkill" },
        { MagicEffect::AlmsiviIntervention, "AlmsiviIntervention" },
        { MagicEffect::Blind, "Blind" },
        { MagicEffect::BoundBattleAxe, "BoundBattleAxe" },
        { MagicEffect::BoundBoots, "BoundBoots" },
        { MagicEffect::BoundCuirass, "BoundCuirass" },
        { MagicEffect::BoundDagger, "BoundDagger" },
        { MagicEffect::BoundGloves, "BoundGloves" },
        { MagicEffect::BoundHelm, "BoundHelm" },
        { MagicEffect::BoundLongbow, "BoundLongbow" },
        { MagicEffect::ExtraSpell, "ExtraSpell" },
        { MagicEffect::BoundLongsword, "BoundLongsword" },
        { MagicEffect::BoundMace, "BoundMace" },
        { MagicEffect::BoundShield, "BoundShield" },
        { MagicEffect::BoundSpear, "BoundSpear" },
        { MagicEffect::Burden, "Burden" },
        { MagicEffect::CalmCreature, "CalmCreature" },
        { MagicEffect::CalmHumanoid, "CalmHumanoid" },
        { MagicEffect::Chameleon, "Chameleon" },
        { MagicEffect::Charm, "Charm" },
        { MagicEffect::CommandCreature, "CommandCreature" },
        { MagicEffect::CommandHumanoid, "CommandHumanoid" },
        { MagicEffect::Corprus, "Corprus" }, // NB this typo. (bethesda made it)
        { MagicEffect::CureBlightDisease, "CureBlightDisease" },
        { MagicEffect::CureCommonDisease, "CureCommonDisease" },
        { MagicEffect::CureCorprusDisease, "CureCorprusDisease" },
        { MagicEffect::CureParalyzation, "CureParalyzation" },
        { MagicEffect::CurePoison, "CurePoison" },
        { MagicEffect::DamageAttribute, "DamageAttribute" },
        { MagicEffect::DamageFatigue, "DamageFatigue" },
        { MagicEffect::DamageHealth, "DamageHealth" },
        { MagicEffect::DamageMagicka, "DamageMagicka" },
        { MagicEffect::DamageSkill, "DamageSkill" },
        { MagicEffect::DemoralizeCreature, "DemoralizeCreature" },
        { MagicEffect::DemoralizeHumanoid, "DemoralizeHumanoid" },
        { MagicEffect::DetectAnimal, "DetectAnimal" },
        { MagicEffect::DetectEnchantment, "DetectEnchantment" },
        { MagicEffect::DetectKey, "DetectKey" },
        { MagicEffect::DisintegrateArmor, "DisintegrateArmor" },
        { MagicEffect::DisintegrateWeapon, "DisintegrateWeapon" },
        { MagicEffect::Dispel, "Dispel" },
        { MagicEffect::DivineIntervention, "DivineIntervention" },
        { MagicEffect::DrainAttribute, "DrainAttribute" },
        { MagicEffect::DrainFatigue, "DrainFatigue" },
        { MagicEffect::DrainHealth, "DrainHealth" },
        { MagicEffect::DrainMagicka, "DrainMagicka" },
        { MagicEffect::DrainSkill, "DrainSkill" },
        { MagicEffect::Feather, "Feather" },
        { MagicEffect::FireDamage, "FireDamage" },
        { MagicEffect::FireShield, "FireShield" },
        { MagicEffect::FortifyAttack, "FortifyAttack" },
        { MagicEffect::FortifyAttribute, "FortifyAttribute" },
        { MagicEffect::FortifyFatigue, "FortifyFatigue" },
        { MagicEffect::FortifyHealth, "FortifyHealth" },
        { MagicEffect::FortifyMagicka, "FortifyMagicka" },
        { MagicEffect::FortifyMaximumMagicka, "FortifyMaximumMagicka" },
        { MagicEffect::FortifySkill, "FortifySkill" },
        { MagicEffect::FrenzyCreature, "FrenzyCreature" },
        { MagicEffect::FrenzyHumanoid, "FrenzyHumanoid" },
        { MagicEffect::FrostDamage, "FrostDamage" },
        { MagicEffect::FrostShield, "FrostShield" },
        { MagicEffect::Invisibility, "Invisibility" },
        { MagicEffect::Jump, "Jump" },
        { MagicEffect::Levitate, "Levitate" },
        { MagicEffect::Light, "Light" },
        { MagicEffect::LightningShield, "LightningShield" },
        { MagicEffect::Lock, "Lock" },
        { MagicEffect::Mark, "Mark" },
        { MagicEffect::NightEye, "NightEye" },
        { MagicEffect::Open, "Open" },
        { MagicEffect::Paralyze, "Paralyze" },
        { MagicEffect::Poison, "Poison" },
        { MagicEffect::RallyCreature, "RallyCreature" },
        { MagicEffect::RallyHumanoid, "RallyHumanoid" },
        { MagicEffect::Recall, "Recall" },
        { MagicEffect::Reflect, "Reflect" },
        { MagicEffect::RemoveCurse, "RemoveCurse" },
        { MagicEffect::ResistBlightDisease, "ResistBlightDisease" },
        { MagicEffect::ResistCommonDisease, "ResistCommonDisease" },
        { MagicEffect::ResistCorprusDisease, "ResistCorprusDisease" },
        { MagicEffect::ResistFire, "ResistFire" },
        { MagicEffect::ResistFrost, "ResistFrost" },
        { MagicEffect::ResistMagicka, "ResistMagicka" },
        { MagicEffect::ResistNormalWeapons, "ResistNormalWeapons" },
        { MagicEffect::ResistParalysis, "ResistParalysis" },
        { MagicEffect::ResistPoison, "ResistPoison" },
        { MagicEffect::ResistShock, "ResistShock" },
        { MagicEffect::RestoreAttribute, "RestoreAttribute" },
        { MagicEffect::RestoreFatigue, "RestoreFatigue" },
        { MagicEffect::RestoreHealth, "RestoreHealth" },
        { MagicEffect::RestoreMagicka, "RestoreMagicka" },
        { MagicEffect::RestoreSkill, "RestoreSkill" },
        { MagicEffect::Sanctuary, "Sanctuary" },
        { MagicEffect::Shield, "Shield" },
        { MagicEffect::ShockDamage, "ShockDamage" },
        { MagicEffect::Silence, "Silence" },
        { MagicEffect::SlowFall, "SlowFall" },
        { MagicEffect::Soultrap, "Soultrap" },
        { MagicEffect::Sound, "Sound" },
        { MagicEffect::SpellAbsorption, "SpellAbsorption" },
        { MagicEffect::StuntedMagicka, "StuntedMagicka" },
        { MagicEffect::SummonAncestralGhost, "SummonAncestralGhost" },
        { MagicEffect::SummonBonelord, "SummonBonelord" },
        { MagicEffect::SummonBonewalker, "SummonBonewalker" },
        { MagicEffect::SummonCenturionSphere, "SummonCenturionSphere" },
        { MagicEffect::SummonClannfear, "SummonClannfear" },
        { MagicEffect::SummonDaedroth, "SummonDaedroth" },
        { MagicEffect::SummonDremora, "SummonDremora" },
        { MagicEffect::SummonFlameAtronach, "SummonFlameAtronach" },
        { MagicEffect::SummonFrostAtronach, "SummonFrostAtronach" },
        { MagicEffect::SummonGoldenSaint, "SummonGoldenSaint" },
        { MagicEffect::SummonGreaterBonewalker, "SummonGreaterBonewalker" },
        { MagicEffect::SummonHunger, "SummonHunger" },
        { MagicEffect::SummonScamp, "SummonScamp" },
        { MagicEffect::SummonSkeletalMinion, "SummonSkeletalMinion" },
        { MagicEffect::SummonStormAtronach, "SummonStormAtronach" },
        { MagicEffect::SummonWingedTwilight, "SummonWingedTwilight" },
        { MagicEffect::SunDamage, "SunDamage" },
        { MagicEffect::SwiftSwim, "SwiftSwim" },
        { MagicEffect::Telekinesis, "Telekinesis" },
        { MagicEffect::TurnUndead, "TurnUndead" },
        { MagicEffect::Vampirism, "Vampirism" },
        { MagicEffect::WaterBreathing, "WaterBreathing" },
        { MagicEffect::WaterWalking, "WaterWalking" },
        { MagicEffect::WeaknessToBlightDisease, "WeaknessToBlightDisease" },
        { MagicEffect::WeaknessToCommonDisease, "WeaknessToCommonDisease" },
        { MagicEffect::WeaknessToCorprusDisease, "WeaknessToCorprusDisease" },
        { MagicEffect::WeaknessToFire, "WeaknessToFire" },
        { MagicEffect::WeaknessToFrost, "WeaknessToFrost" },
        { MagicEffect::WeaknessToMagicka, "WeaknessToMagicka" },
        { MagicEffect::WeaknessToNormalWeapons, "WeaknessToNormalWeapons" },
        { MagicEffect::WeaknessToPoison, "WeaknessToPoison" },
        { MagicEffect::WeaknessToShock, "WeaknessToShock" },

        // bloodmoon
        { MagicEffect::SummonWolf, "SummonWolf" },
        { MagicEffect::SummonBear, "SummonBear" },
        { MagicEffect::SummonBonewolf, "SummonBonewolf" },
        { MagicEffect::SummonCreature04, "SummonCreature04" },
        { MagicEffect::SummonCreature05, "SummonCreature05" },

        // tribunal
        { MagicEffect::SummonFabricant, "SummonFabricant" },
    };

    class FindSecond
    {
        std::string_view mName;

    public:
        FindSecond(std::string_view name)
            : mName(name)
        {
        }

        bool operator()(const std::pair<short, std::string>& item) const
        {
            if (Misc::StringUtils::ciEqual(item.second, mName))
                return true;
            return false;
        }
    };

    MagicEffect::MagnitudeDisplayType MagicEffect::getMagnitudeDisplayType() const
    {
        if (mData.mFlags & NoMagnitude)
            return MDT_None;
        if (mIndex == 84)
            return MDT_TimesInt;
        if (mIndex == 59 || (mIndex >= 64 && mIndex <= 66))
            return MDT_Feet;
        if (mIndex == 118 || mIndex == 119)
            return MDT_Level;
        if ((mIndex >= 28 && mIndex <= 36) || (mIndex >= 90 && mIndex <= 99) || mIndex == 40 || mIndex == 47
            || mIndex == 57 || mIndex == 68)
            return MDT_Percentage;

        return MDT_Points;
    }

    void MagicEffect::blank()
    {
        mRecordFlags = 0;
        mData.mSchool = 0;
        mData.mBaseCost = 0;
        mData.mFlags = 0;
        mData.mRed = 0;
        mData.mGreen = 0;
        mData.mBlue = 0;
        mData.mSpeed = 0;

        mIcon.clear();
        mParticle.clear();
        mCasting = ESM::RefId();
        mHit = ESM::RefId();
        mArea = ESM::RefId();
        mBolt = ESM::RefId();
        mCastSound = ESM::RefId();
        mBoltSound = ESM::RefId();
        mHitSound = ESM::RefId();
        mAreaSound = ESM::RefId();
        mDescription.clear();
    }

    std::string_view MagicEffect::effectIdToGmstString(int effectID)
    {
        auto name = sGmstEffectIds.find(effectID);
        if (name == sGmstEffectIds.end())
            throw std::runtime_error(std::string("Unimplemented effect ID ") + std::to_string(effectID));

        return name->second;
    }

    std::string_view MagicEffect::effectIdToName(int effectID)
    {
        auto name = sEffectNames.find(effectID);
        if (name == sEffectNames.end())
            throw std::runtime_error(std::string("Unimplemented effect ID ") + std::to_string(effectID));

        return name->second;
    }

    int MagicEffect::effectNameToId(std::string_view effect)
    {
        auto name = std::find_if(sEffectNames.begin(), sEffectNames.end(), FindSecond(effect));
        if (name == sEffectNames.end())
            throw std::runtime_error("Unimplemented effect " + std::string(effect));

        return name->first;
    }

    int MagicEffect::effectGmstIdToId(std::string_view gmstId)
    {
        auto name = std::find_if(sGmstEffectIds.begin(), sGmstEffectIds.end(), FindSecond(gmstId));
        if (name == sGmstEffectIds.end())
            throw std::runtime_error("Unimplemented effect " + std::string(gmstId));

        return name->first;
    }

    RefId MagicEffect::indexToRefId(int index)
    {
        if (index == -1)
            return RefId();
        return RefId::index(sRecordId, static_cast<std::uint32_t>(index));
    }
}
