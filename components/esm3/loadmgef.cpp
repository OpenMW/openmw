#include "loadmgef.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "loadskil.hpp"

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

    void MagicEffect::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // MagicEffect record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        esm.getHNT(mIndex, "INDX");

        mId = indexToRefId(mIndex);

        esm.getSubNameIs("MEDT");
        esm.getSubHeader();
        int32_t school;
        esm.getT(school);
        mData.mSchool = MagicSchool::indexToSkillRefId(school);
        esm.getT(mData.mBaseCost);
        esm.getT(mData.mFlags);
        esm.getT(mData.mRed);
        esm.getT(mData.mGreen);
        esm.getT(mData.mBlue);
        esm.getT(mData.mUnknown1);
        esm.getT(mData.mSpeed);
        esm.getT(mData.mUnknown2);

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

        esm.startSubRecord("MEDT");
        esm.writeT(MagicSchool::skillRefIdToIndex(mData.mSchool));
        esm.writeT(mData.mBaseCost);
        esm.writeT(mData.mFlags);
        esm.writeT(mData.mRed);
        esm.writeT(mData.mGreen);
        esm.writeT(mData.mBlue);
        esm.writeT(mData.mUnknown1);
        esm.writeT(mData.mSpeed);
        esm.writeT(mData.mUnknown2);
        esm.endRecord("MEDT");

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

    namespace
    {
        std::map<short, short> makeEffectsMap()
        {
            std::map<short, short> effects;

            effects[MagicEffect::Effects::DisintegrateArmor] = MagicEffect::Effects::Sanctuary;
            effects[MagicEffect::Effects::DisintegrateWeapon] = MagicEffect::Effects::Sanctuary;

            for (short i = MagicEffect::Effects::DrainAttribute; i <= MagicEffect::Effects::DamageSkill; ++i)
                effects[i] = MagicEffect::Effects::ResistMagicka;
            for (short i = MagicEffect::Effects::AbsorbAttribute; i <= MagicEffect::Effects::AbsorbSkill; ++i)
                effects[i] = MagicEffect::Effects::ResistMagicka;
            for (short i = MagicEffect::Effects::WeaknessToFire; i <= MagicEffect::Effects::WeaknessToNormalWeapons; ++i)
                effects[i] = MagicEffect::Effects::ResistMagicka;

            effects[MagicEffect::Effects::Burden] = MagicEffect::Effects::ResistMagicka;
            effects[MagicEffect::Effects::Charm] = MagicEffect::Effects::ResistMagicka;
            effects[MagicEffect::Effects::Silence] = MagicEffect::Effects::ResistMagicka;
            effects[MagicEffect::Effects::Blind] = MagicEffect::Effects::ResistMagicka;
            effects[MagicEffect::Effects::Sound] = MagicEffect::Effects::ResistMagicka;

            for (short i = 0; i < 2; ++i)
            {
                effects[MagicEffect::Effects::CalmHumanoid + i] = MagicEffect::Effects::ResistMagicka;
                effects[MagicEffect::Effects::FrenzyHumanoid + i] = MagicEffect::Effects::ResistMagicka;
                effects[MagicEffect::Effects::DemoralizeHumanoid + i] = MagicEffect::Effects::ResistMagicka;
                effects[MagicEffect::Effects::RallyHumanoid + i] = MagicEffect::Effects::ResistMagicka;
            }

            effects[MagicEffect::Effects::TurnUndead] = MagicEffect::Effects::ResistMagicka;

            effects[MagicEffect::Effects::FireDamage] = MagicEffect::Effects::ResistFire;
            effects[MagicEffect::Effects::FrostDamage] = MagicEffect::Effects::ResistFrost;
            effects[MagicEffect::Effects::ShockDamage] = MagicEffect::Effects::ResistShock;
            effects[MagicEffect::Effects::Vampirism] = MagicEffect::Effects::ResistCommonDisease;
            effects[MagicEffect::Effects::Corprus] = MagicEffect::Effects::ResistCorprusDisease;
            effects[MagicEffect::Effects::Poison] = MagicEffect::Effects::ResistPoison;
            effects[MagicEffect::Effects::Paralyze] = MagicEffect::Effects::ResistParalysis;

            return effects;
        }
    }

    short MagicEffect::getResistanceEffect(short effect)
    {
        // Source https://wiki.openmw.org/index.php?title=Research:Magic#Effect_attribute

        // <Effect, Effect providing resistance against first effect>
        static const std::map<short, short> effects = makeEffectsMap();

        if (const auto it = effects.find(effect); it != effects.end())
            return it->second;

        return -1;
    }

    short MagicEffect::getWeaknessEffect(short effect)
    {
        static std::map<short, short> effects;
        if (effects.empty())
        {
            for (short i = DrainAttribute; i <= DamageSkill; ++i)
                effects[i] = WeaknessToMagicka;
            for (short i = AbsorbAttribute; i <= AbsorbSkill; ++i)
                effects[i] = WeaknessToMagicka;
            for (short i = WeaknessToFire; i <= WeaknessToNormalWeapons; ++i)
                effects[i] = WeaknessToMagicka;

            effects[Burden] = WeaknessToMagicka;
            effects[Charm] = WeaknessToMagicka;
            effects[Silence] = WeaknessToMagicka;
            effects[Blind] = WeaknessToMagicka;
            effects[Sound] = WeaknessToMagicka;

            for (short i = 0; i < 2; ++i)
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
    const std::array<std::string, MagicEffect::Length> MagicEffect::sGmstEffectIds = {

        "sEffectWaterBreathing",
        "sEffectSwiftSwim",
        "sEffectWaterWalking",
        "sEffectShield",
        "sEffectFireShield",
        "sEffectLightningShield",
        "sEffectFrostShield",
        "sEffectBurden",
        "sEffectFeather",
        "sEffectJump",
        "sEffectLevitate",
        "sEffectSlowFall",
        "sEffectLock",
        "sEffectOpen",
        "sEffectFireDamage",
        "sEffectShockDamage",
        "sEffectFrostDamage",
        "sEffectDrainAttribute",
        "sEffectDrainHealth",
        "sEffectDrainSpellpoints",
        "sEffectDrainFatigue",
        "sEffectDrainSkill",
        "sEffectDamageAttribute",
        "sEffectDamageHealth",
        "sEffectDamageMagicka",
        "sEffectDamageFatigue",
        "sEffectDamageSkill",
        "sEffectPoison",
        "sEffectWeaknesstoFire",
        "sEffectWeaknesstoFrost",
        "sEffectWeaknesstoShock",
        "sEffectWeaknesstoMagicka",
        "sEffectWeaknesstoCommonDisease",
        "sEffectWeaknesstoBlightDisease",
        "sEffectWeaknesstoCorprusDisease",
        "sEffectWeaknesstoPoison",
        "sEffectWeaknesstoNormalWeapons",
        "sEffectDisintegrateWeapon",
        "sEffectDisintegrateArmor",
        "sEffectInvisibility",
        "sEffectChameleon",
        "sEffectLight",
        "sEffectSanctuary",
        "sEffectNightEye",
        "sEffectCharm",
        "sEffectParalyze",
        "sEffectSilence",
        "sEffectBlind",
        "sEffectSound",
        "sEffectCalmHumanoid",
        "sEffectCalmCreature",
        "sEffectFrenzyHumanoid",
        "sEffectFrenzyCreature",
        "sEffectDemoralizeHumanoid",
        "sEffectDemoralizeCreature",
        "sEffectRallyHumanoid",
        "sEffectRallyCreature",
        "sEffectDispel",
        "sEffectSoultrap",
        "sEffectTelekinesis",
        "sEffectMark",
        "sEffectRecall",
        "sEffectDivineIntervention",
        "sEffectAlmsiviIntervention",
        "sEffectDetectAnimal",
        "sEffectDetectEnchantment",
        "sEffectDetectKey",
        "sEffectSpellAbsorption",
        "sEffectReflect",
        "sEffectCureCommonDisease",
        "sEffectCureBlightDisease",
        "sEffectCureCorprusDisease",
        "sEffectCurePoison",
        "sEffectCureParalyzation",
        "sEffectRestoreAttribute",
        "sEffectRestoreHealth",
        "sEffectRestoreSpellPoints",
        "sEffectRestoreFatigue",
        "sEffectRestoreSkill",
        "sEffectFortifyAttribute",
        "sEffectFortifyHealth",
        "sEffectFortifySpellpoints",
        "sEffectFortifyFatigue",
        "sEffectFortifySkill",
        "sEffectFortifyMagickaMultiplier",
        "sEffectAbsorbAttribute",
        "sEffectAbsorbHealth",
        "sEffectAbsorbSpellPoints",
        "sEffectAbsorbFatigue",
        "sEffectAbsorbSkill",
        "sEffectResistFire",
        "sEffectResistFrost",
        "sEffectResistShock",
        "sEffectResistMagicka",
        "sEffectResistCommonDisease",
        "sEffectResistBlightDisease",
        "sEffectResistCorprusDisease",
        "sEffectResistPoison",
        "sEffectResistNormalWeapons",
        "sEffectResistParalysis",
        "sEffectRemoveCurse",
        "sEffectTurnUndead",
        "sEffectSummonScamp",
        "sEffectSummonClannfear",
        "sEffectSummonDaedroth",
        "sEffectSummonDremora",
        "sEffectSummonAncestralGhost",
        "sEffectSummonSkeletalMinion",
        "sEffectSummonLeastBonewalker",
        "sEffectSummonGreaterBonewalker",
        "sEffectSummonBonelord",
        "sEffectSummonWingedTwilight",
        "sEffectSummonHunger",
        "sEffectSummonGoldenSaint",
        "sEffectSummonFlameAtronach",
        "sEffectSummonFrostAtronach",
        "sEffectSummonStormAtronach",
        "sEffectFortifyAttackBonus",
        "sEffectCommandCreatures",
        "sEffectCommandHumanoids",
        "sEffectBoundDagger",
        "sEffectBoundLongsword",
        "sEffectBoundMace",
        "sEffectBoundBattleAxe",
        "sEffectBoundSpear",
        "sEffectBoundLongbow",
        "sEffectExtraSpell",
        "sEffectBoundCuirass",
        "sEffectBoundHelm",
        "sEffectBoundBoots",
        "sEffectBoundShield",
        "sEffectBoundGloves",
        "sEffectCorpus", // NB this typo. (bethesda made it)
        "sEffectVampirism",
        "sEffectSummonCenturionSphere",
        "sEffectSunDamage",
        "sEffectStuntedMagicka",

        // tribunal
        "sEffectSummonFabricant",

        // bloodmoon
        "sEffectSummonCreature01",
        "sEffectSummonCreature02",
        "sEffectSummonCreature03",
        "sEffectSummonCreature04",
        "sEffectSummonCreature05",
    };

    // Map effect ID to identifying name
    const std::array<std::string_view, MagicEffect::Length> MagicEffect::sIndexNames = {
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

        // tribunal
        "SummonFabricant",

        // bloodmoon
        "SummonWolf",
        "SummonBear",
        "SummonBonewolf",
        "SummonCreature04",
        "SummonCreature05",
    };

    template <typename Collection>
    static std::map<std::string_view, int, Misc::StringUtils::CiComp> initStringToIntMap(const Collection& strings)
    {
        std::map<std::string_view, int, Misc::StringUtils::CiComp> map;
        for (size_t i = 0; i < strings.size(); i++)
            map[strings[i]] = i;

        return map;
    }

    const std::map<std::string_view, int, Misc::StringUtils::CiComp> MagicEffect::sGmstEffectIdToIndexMap
        = initStringToIntMap(MagicEffect::sGmstEffectIds);

    const std::map<std::string_view, int, Misc::StringUtils::CiComp> MagicEffect::sIndexNameToIndexMap
        = initStringToIntMap(MagicEffect::sIndexNames);

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
        mData.mSchool = ESM::Skill::Alteration;
        mData.mBaseCost = 0;
        mData.mFlags = 0;
        mData.mRed = 0;
        mData.mGreen = 0;
        mData.mBlue = 0;
        mData.mSpeed = 1;

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

    osg::Vec4f MagicEffect::getColor() const
    {
        osg::Vec4f color{ mData.mRed / 255.f, mData.mGreen / 255.f, mData.mBlue / 255.f, 1.f };
        if (mData.mFlags & NegativeLight)
            return osg::Vec4f(1.f, 1.f, 1.f, 2.f) - color;
        return color;
    }

    const std::string& MagicEffect::indexToGmstString(int effectID)
    {
        if (effectID < 0 || static_cast<std::size_t>(effectID) >= sGmstEffectIds.size())
            throw std::runtime_error(std::string("Unimplemented effect ID ") + std::to_string(effectID));

        return sGmstEffectIds[effectID];
    }

    std::string_view MagicEffect::indexToName(int effectID)
    {
        if (effectID < 0 || static_cast<std::size_t>(effectID) >= sIndexNames.size())
            throw std::runtime_error(std::string("Unimplemented effect ID ") + std::to_string(effectID));

        return sIndexNames[effectID];
    }

    int MagicEffect::indexNameToIndex(std::string_view effect)
    {
        auto name = sIndexNameToIndexMap.find(effect);
        if (name == sIndexNameToIndexMap.end())
            return -1;

        return name->second;
    }

    int MagicEffect::effectGmstIdToIndex(std::string_view gmstId)
    {
        auto name = sGmstEffectIdToIndexMap.find(gmstId);
        if (name == sGmstEffectIdToIndexMap.end())
            throw std::runtime_error("Unimplemented effect " + std::string(gmstId));

        return name->second;
    }

    RefId MagicEffect::indexToRefId(int index)
    {
        if (index == -1)
            return RefId();
        return RefId::index(sRecordId, static_cast<std::uint32_t>(index));
    }
}
