#include "loadmgef.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "loadskil.hpp"

#include <components/misc/concepts.hpp>
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

    const MagicEffectId MagicEffect::WaterBreathing("WaterBreathing");
    const MagicEffectId MagicEffect::SwiftSwim("SwiftSwim");
    const MagicEffectId MagicEffect::WaterWalking("WaterWalking");
    const MagicEffectId MagicEffect::Shield("Shield");
    const MagicEffectId MagicEffect::FireShield("FireShield");
    const MagicEffectId MagicEffect::LightningShield("LightningShield");
    const MagicEffectId MagicEffect::FrostShield("FrostShield");
    const MagicEffectId MagicEffect::Burden("Burden");
    const MagicEffectId MagicEffect::Feather("Feather");
    const MagicEffectId MagicEffect::Jump("Jump");
    const MagicEffectId MagicEffect::Levitate("Levitate");
    const MagicEffectId MagicEffect::SlowFall("SlowFall");
    const MagicEffectId MagicEffect::Lock("Lock");
    const MagicEffectId MagicEffect::Open("Open");
    const MagicEffectId MagicEffect::FireDamage("FireDamage");
    const MagicEffectId MagicEffect::ShockDamage("ShockDamage");
    const MagicEffectId MagicEffect::FrostDamage("FrostDamage");
    const MagicEffectId MagicEffect::DrainAttribute("DrainAttribute");
    const MagicEffectId MagicEffect::DrainHealth("DrainHealth");
    const MagicEffectId MagicEffect::DrainMagicka("DrainMagicka");
    const MagicEffectId MagicEffect::DrainFatigue("DrainFatigue");
    const MagicEffectId MagicEffect::DrainSkill("DrainSkill");
    const MagicEffectId MagicEffect::DamageAttribute("DamageAttribute");
    const MagicEffectId MagicEffect::DamageHealth("DamageHealth");
    const MagicEffectId MagicEffect::DamageMagicka("DamageMagicka");
    const MagicEffectId MagicEffect::DamageFatigue("DamageFatigue");
    const MagicEffectId MagicEffect::DamageSkill("DamageSkill");
    const MagicEffectId MagicEffect::Poison("Poison");
    const MagicEffectId MagicEffect::WeaknessToFire("WeaknessToFire");
    const MagicEffectId MagicEffect::WeaknessToFrost("WeaknessToFrost");
    const MagicEffectId MagicEffect::WeaknessToShock("WeaknessToShock");
    const MagicEffectId MagicEffect::WeaknessToMagicka("WeaknessToMagicka");
    const MagicEffectId MagicEffect::WeaknessToCommonDisease("WeaknessToCommonDisease");
    const MagicEffectId MagicEffect::WeaknessToBlightDisease("WeaknessToBlightDisease");
    const MagicEffectId MagicEffect::WeaknessToCorprusDisease("WeaknessToCorprusDisease");
    const MagicEffectId MagicEffect::WeaknessToPoison("WeaknessToPoison");
    const MagicEffectId MagicEffect::WeaknessToNormalWeapons("WeaknessToNormalWeapons");
    const MagicEffectId MagicEffect::DisintegrateWeapon("DisintegrateWeapon");
    const MagicEffectId MagicEffect::DisintegrateArmor("DisintegrateArmor");
    const MagicEffectId MagicEffect::Invisibility("Invisibility");
    const MagicEffectId MagicEffect::Chameleon("Chameleon");
    const MagicEffectId MagicEffect::Light("Light");
    const MagicEffectId MagicEffect::Sanctuary("Sanctuary");
    const MagicEffectId MagicEffect::NightEye("NightEye");
    const MagicEffectId MagicEffect::Charm("Charm");
    const MagicEffectId MagicEffect::Paralyze("Paralyze");
    const MagicEffectId MagicEffect::Silence("Silence");
    const MagicEffectId MagicEffect::Blind("Blind");
    const MagicEffectId MagicEffect::Sound("Sound");
    const MagicEffectId MagicEffect::CalmHumanoid("CalmHumanoid");
    const MagicEffectId MagicEffect::CalmCreature("CalmCreature");
    const MagicEffectId MagicEffect::FrenzyHumanoid("FrenzyHumanoid");
    const MagicEffectId MagicEffect::FrenzyCreature("FrenzyCreature");
    const MagicEffectId MagicEffect::DemoralizeHumanoid("DemoralizeHumanoid");
    const MagicEffectId MagicEffect::DemoralizeCreature("DemoralizeCreature");
    const MagicEffectId MagicEffect::RallyHumanoid("RallyHumanoid");
    const MagicEffectId MagicEffect::RallyCreature("RallyCreature");
    const MagicEffectId MagicEffect::Dispel("Dispel");
    const MagicEffectId MagicEffect::Soultrap("Soultrap");
    const MagicEffectId MagicEffect::Telekinesis("Telekinesis");
    const MagicEffectId MagicEffect::Mark("Mark");
    const MagicEffectId MagicEffect::Recall("Recall");
    const MagicEffectId MagicEffect::DivineIntervention("DivineIntervention");
    const MagicEffectId MagicEffect::AlmsiviIntervention("AlmsiviIntervention");
    const MagicEffectId MagicEffect::DetectAnimal("DetectAnimal");
    const MagicEffectId MagicEffect::DetectEnchantment("DetectEnchantment");
    const MagicEffectId MagicEffect::DetectKey("DetectKey");
    const MagicEffectId MagicEffect::SpellAbsorption("SpellAbsorption");
    const MagicEffectId MagicEffect::Reflect("Reflect");
    const MagicEffectId MagicEffect::CureCommonDisease("CureCommonDisease");
    const MagicEffectId MagicEffect::CureBlightDisease("CureBlightDisease");
    const MagicEffectId MagicEffect::CureCorprusDisease("CureCorprusDisease");
    const MagicEffectId MagicEffect::CurePoison("CurePoison");
    const MagicEffectId MagicEffect::CureParalyzation("CureParalyzation");
    const MagicEffectId MagicEffect::RestoreAttribute("RestoreAttribute");
    const MagicEffectId MagicEffect::RestoreHealth("RestoreHealth");
    const MagicEffectId MagicEffect::RestoreMagicka("RestoreMagicka");
    const MagicEffectId MagicEffect::RestoreFatigue("RestoreFatigue");
    const MagicEffectId MagicEffect::RestoreSkill("RestoreSkill");
    const MagicEffectId MagicEffect::FortifyAttribute("FortifyAttribute");
    const MagicEffectId MagicEffect::FortifyHealth("FortifyHealth");
    const MagicEffectId MagicEffect::FortifyMagicka("FortifyMagicka");
    const MagicEffectId MagicEffect::FortifyFatigue("FortifyFatigue");
    const MagicEffectId MagicEffect::FortifySkill("FortifySkill");
    const MagicEffectId MagicEffect::FortifyMaximumMagicka("FortifyMaximumMagicka");
    const MagicEffectId MagicEffect::AbsorbAttribute("AbsorbAttribute");
    const MagicEffectId MagicEffect::AbsorbHealth("AbsorbHealth");
    const MagicEffectId MagicEffect::AbsorbMagicka("AbsorbMagicka");
    const MagicEffectId MagicEffect::AbsorbFatigue("AbsorbFatigue");
    const MagicEffectId MagicEffect::AbsorbSkill("AbsorbSkill");
    const MagicEffectId MagicEffect::ResistFire("ResistFire");
    const MagicEffectId MagicEffect::ResistFrost("ResistFrost");
    const MagicEffectId MagicEffect::ResistShock("ResistShock");
    const MagicEffectId MagicEffect::ResistMagicka("ResistMagicka");
    const MagicEffectId MagicEffect::ResistCommonDisease("ResistCommonDisease");
    const MagicEffectId MagicEffect::ResistBlightDisease("ResistBlightDisease");
    const MagicEffectId MagicEffect::ResistCorprusDisease("ResistCorprusDisease");
    const MagicEffectId MagicEffect::ResistPoison("ResistPoison");
    const MagicEffectId MagicEffect::ResistNormalWeapons("ResistNormalWeapons");
    const MagicEffectId MagicEffect::ResistParalysis("ResistParalysis");
    const MagicEffectId MagicEffect::RemoveCurse("RemoveCurse");
    const MagicEffectId MagicEffect::TurnUndead("TurnUndead");
    const MagicEffectId MagicEffect::SummonScamp("SummonScamp");
    const MagicEffectId MagicEffect::SummonClannfear("SummonClannfear");
    const MagicEffectId MagicEffect::SummonDaedroth("SummonDaedroth");
    const MagicEffectId MagicEffect::SummonDremora("SummonDremora");
    const MagicEffectId MagicEffect::SummonAncestralGhost("SummonAncestralGhost");
    const MagicEffectId MagicEffect::SummonSkeletalMinion("SummonSkeletalMinion");
    const MagicEffectId MagicEffect::SummonBonewalker("SummonBonewalker");
    const MagicEffectId MagicEffect::SummonGreaterBonewalker("SummonGreaterBonewalker");
    const MagicEffectId MagicEffect::SummonBonelord("SummonBonelord");
    const MagicEffectId MagicEffect::SummonWingedTwilight("SummonWingedTwilight");
    const MagicEffectId MagicEffect::SummonHunger("SummonHunger");
    const MagicEffectId MagicEffect::SummonGoldenSaint("SummonGoldenSaint");
    const MagicEffectId MagicEffect::SummonFlameAtronach("SummonFlameAtronach");
    const MagicEffectId MagicEffect::SummonFrostAtronach("SummonFrostAtronach");
    const MagicEffectId MagicEffect::SummonStormAtronach("SummonStormAtronach");
    const MagicEffectId MagicEffect::FortifyAttack("FortifyAttack");
    const MagicEffectId MagicEffect::CommandCreature("CommandCreature");
    const MagicEffectId MagicEffect::CommandHumanoid("CommandHumanoid");
    const MagicEffectId MagicEffect::BoundDagger("BoundDagger");
    const MagicEffectId MagicEffect::BoundLongsword("BoundLongsword");
    const MagicEffectId MagicEffect::BoundMace("BoundMace");
    const MagicEffectId MagicEffect::BoundBattleAxe("BoundBattleAxe");
    const MagicEffectId MagicEffect::BoundSpear("BoundSpear");
    const MagicEffectId MagicEffect::BoundLongbow("BoundLongbow");
    const MagicEffectId MagicEffect::ExtraSpell("ExtraSpell");
    const MagicEffectId MagicEffect::BoundCuirass("BoundCuirass");
    const MagicEffectId MagicEffect::BoundHelm("BoundHelm");
    const MagicEffectId MagicEffect::BoundBoots("BoundBoots");
    const MagicEffectId MagicEffect::BoundShield("BoundShield");
    const MagicEffectId MagicEffect::BoundGloves("BoundGloves");
    const MagicEffectId MagicEffect::Corprus("Corprus");
    const MagicEffectId MagicEffect::Vampirism("Vampirism");
    const MagicEffectId MagicEffect::SummonCenturionSphere("SummonCenturionSphere");
    const MagicEffectId MagicEffect::SunDamage("SunDamage");
    const MagicEffectId MagicEffect::StuntedMagicka("StuntedMagicka");

    // Tribunal only
    const MagicEffectId MagicEffect::SummonFabricant("SummonFabricant");

    // Bloodmoon only
    const MagicEffectId MagicEffect::SummonWolf("SummonWolf");
    const MagicEffectId MagicEffect::SummonBear("SummonBear");
    const MagicEffectId MagicEffect::SummonBonewolf("SummonBonewolf");
    const MagicEffectId MagicEffect::SummonCreature04("SummonCreature04");
    const MagicEffectId MagicEffect::SummonCreature05("SummonCreature05");

    void MagicEffect::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // MagicEffect record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        int32_t index = -1;
        esm.getHNT(index, "INDX");
        if (index < 0 || index >= Length)
            esm.fail("Invalid Index!");

        mId = *indexToRefId(index).getIf<MagicEffectId>();

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
            if (index >= 0 && index < NumberOfHardcodedFlags)
                mData.mFlags |= HardcodedFlags[index];
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
        esm.writeHNT("INDX", refIdToIndex(mId));

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
        std::map<MagicEffectId, MagicEffectId> makeResistancesMap()
        {
            std::map<MagicEffectId, MagicEffectId> effects;

            effects[MagicEffect::DisintegrateArmor] = MagicEffect::Sanctuary;
            effects[MagicEffect::DisintegrateWeapon] = MagicEffect::Sanctuary;

            effects[MagicEffect::DrainAttribute] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DrainHealth] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DrainMagicka] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DrainFatigue] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DrainSkill] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DamageAttribute] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DamageHealth] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DamageMagicka] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DamageFatigue] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DamageSkill] = MagicEffect::ResistMagicka;

            effects[MagicEffect::AbsorbAttribute] = MagicEffect::ResistMagicka;
            effects[MagicEffect::AbsorbHealth] = MagicEffect::ResistMagicka;
            effects[MagicEffect::AbsorbMagicka] = MagicEffect::ResistMagicka;
            effects[MagicEffect::AbsorbFatigue] = MagicEffect::ResistMagicka;
            effects[MagicEffect::AbsorbSkill] = MagicEffect::ResistMagicka;

            effects[MagicEffect::WeaknessToFire] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToFrost] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToShock] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToMagicka] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToCommonDisease] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToBlightDisease] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToCorprusDisease] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToPoison] = MagicEffect::ResistMagicka;
            effects[MagicEffect::WeaknessToNormalWeapons] = MagicEffect::ResistMagicka;

            effects[MagicEffect::Burden] = MagicEffect::ResistMagicka;
            effects[MagicEffect::Charm] = MagicEffect::ResistMagicka;
            effects[MagicEffect::Silence] = MagicEffect::ResistMagicka;
            effects[MagicEffect::Blind] = MagicEffect::ResistMagicka;
            effects[MagicEffect::Sound] = MagicEffect::ResistMagicka;

            effects[MagicEffect::CalmHumanoid] = MagicEffect::ResistMagicka;
            effects[MagicEffect::CalmCreature] = MagicEffect::ResistMagicka;
            effects[MagicEffect::FrenzyHumanoid] = MagicEffect::ResistMagicka;
            effects[MagicEffect::FrenzyCreature] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DemoralizeHumanoid] = MagicEffect::ResistMagicka;
            effects[MagicEffect::DemoralizeCreature] = MagicEffect::ResistMagicka;
            effects[MagicEffect::RallyHumanoid] = MagicEffect::ResistMagicka;
            effects[MagicEffect::RallyCreature] = MagicEffect::ResistMagicka;

            effects[MagicEffect::TurnUndead] = MagicEffect::ResistMagicka;

            effects[MagicEffect::FireDamage] = MagicEffect::ResistFire;
            effects[MagicEffect::FrostDamage] = MagicEffect::ResistFrost;
            effects[MagicEffect::ShockDamage] = MagicEffect::ResistShock;
            effects[MagicEffect::Vampirism] = MagicEffect::ResistCommonDisease;
            effects[MagicEffect::Corprus] = MagicEffect::ResistCorprusDisease;
            effects[MagicEffect::Poison] = MagicEffect::ResistPoison;
            effects[MagicEffect::Paralyze] = MagicEffect::ResistParalysis;

            return effects;
        }
    }

    RefId MagicEffect::getResistanceEffect(const MagicEffectId& effectId)
    {
        // Source https://wiki.openmw.org/index.php?title=Research:Magic#Effect_attribute

        // <Effect, Effect providing resistance against first effect>
        static const std::map<MagicEffectId, MagicEffectId> effects = makeResistancesMap();

        if (const auto it = effects.find(effectId); it != effects.end())
            return it->second;

        return {};
    }

    namespace
    {
        std::map<MagicEffectId, MagicEffectId> makeWeaknessesMap()
        {
            static std::map<MagicEffectId, MagicEffectId> effects;

            effects[MagicEffect::DrainAttribute] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DrainHealth] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DrainMagicka] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DrainFatigue] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DrainSkill] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DamageAttribute] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DamageHealth] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DamageMagicka] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DamageFatigue] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DamageSkill] = MagicEffect::WeaknessToMagicka;

            effects[MagicEffect::AbsorbAttribute] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::AbsorbHealth] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::AbsorbMagicka] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::AbsorbFatigue] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::AbsorbSkill] = MagicEffect::WeaknessToMagicka;

            effects[MagicEffect::WeaknessToFire] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToFrost] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToShock] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToMagicka] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToCommonDisease] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToBlightDisease] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToCorprusDisease] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToPoison] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::WeaknessToNormalWeapons] = MagicEffect::WeaknessToMagicka;

            effects[MagicEffect::Burden] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::Charm] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::Silence] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::Blind] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::Sound] = MagicEffect::WeaknessToMagicka;

            effects[MagicEffect::CalmHumanoid] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::CalmCreature] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::FrenzyHumanoid] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::FrenzyCreature] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DemoralizeHumanoid] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::DemoralizeCreature] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::RallyHumanoid] = MagicEffect::WeaknessToMagicka;
            effects[MagicEffect::RallyCreature] = MagicEffect::WeaknessToMagicka;

            effects[MagicEffect::TurnUndead] = MagicEffect::WeaknessToMagicka;

            effects[MagicEffect::FireDamage] = MagicEffect::WeaknessToFire;
            effects[MagicEffect::FrostDamage] = MagicEffect::WeaknessToFrost;
            effects[MagicEffect::ShockDamage] = MagicEffect::WeaknessToShock;
            effects[MagicEffect::Vampirism] = MagicEffect::WeaknessToCommonDisease;
            effects[MagicEffect::Corprus] = MagicEffect::WeaknessToCorprusDisease;
            effects[MagicEffect::Poison] = MagicEffect::WeaknessToPoison;

            return effects;
        }
    }

    RefId MagicEffect::getWeaknessEffect(const MagicEffectId& effectId)
    {
        static const std::map<MagicEffectId, MagicEffectId> effects = makeWeaknessesMap();

        if (const auto it = effects.find(effectId); it != effects.end())
            return it->second;

        return {};
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

    static const std::array<MagicEffectId, MagicEffect::Length> sMagicEffectIds
    {
        MagicEffect::WaterBreathing,
        MagicEffect::SwiftSwim,
        MagicEffect::WaterWalking,
        MagicEffect::Shield,
        MagicEffect::FireShield,
        MagicEffect::LightningShield,
        MagicEffect::FrostShield,
        MagicEffect::Burden,
        MagicEffect::Feather,
        MagicEffect::Jump,
        MagicEffect::Levitate,
        MagicEffect::SlowFall,
        MagicEffect::Lock,
        MagicEffect::Open,
        MagicEffect::FireDamage,
        MagicEffect::ShockDamage,
        MagicEffect::FrostDamage,
        MagicEffect::DrainAttribute,
        MagicEffect::DrainHealth,
        MagicEffect::DrainMagicka,
        MagicEffect::DrainFatigue,
        MagicEffect::DrainSkill,
        MagicEffect::DamageAttribute,
        MagicEffect::DamageHealth,
        MagicEffect::DamageMagicka,
        MagicEffect::DamageFatigue,
        MagicEffect::DamageSkill,
        MagicEffect::Poison,
        MagicEffect::WeaknessToFire,
        MagicEffect::WeaknessToFrost,
        MagicEffect::WeaknessToShock,
        MagicEffect::WeaknessToMagicka,
        MagicEffect::WeaknessToCommonDisease,
        MagicEffect::WeaknessToBlightDisease,
        MagicEffect::WeaknessToCorprusDisease,
        MagicEffect::WeaknessToPoison,
        MagicEffect::WeaknessToNormalWeapons,
        MagicEffect::DisintegrateWeapon,
        MagicEffect::DisintegrateArmor,
        MagicEffect::Invisibility,
        MagicEffect::Chameleon,
        MagicEffect::Light,
        MagicEffect::Sanctuary,
        MagicEffect::NightEye,
        MagicEffect::Charm,
        MagicEffect::Paralyze,
        MagicEffect::Silence,
        MagicEffect::Blind,
        MagicEffect::Sound,
        MagicEffect::CalmHumanoid,
        MagicEffect::CalmCreature,
        MagicEffect::FrenzyHumanoid,
        MagicEffect::FrenzyCreature,
        MagicEffect::DemoralizeHumanoid,
        MagicEffect::DemoralizeCreature,
        MagicEffect::RallyHumanoid,
        MagicEffect::RallyCreature,
        MagicEffect::Dispel,
        MagicEffect::Soultrap,
        MagicEffect::Telekinesis,
        MagicEffect::Mark,
        MagicEffect::Recall,
        MagicEffect::DivineIntervention,
        MagicEffect::AlmsiviIntervention,
        MagicEffect::DetectAnimal,
        MagicEffect::DetectEnchantment,
        MagicEffect::DetectKey,
        MagicEffect::SpellAbsorption,
        MagicEffect::Reflect,
        MagicEffect::CureCommonDisease,
        MagicEffect::CureBlightDisease,
        MagicEffect::CureCorprusDisease,
        MagicEffect::CurePoison,
        MagicEffect::CureParalyzation,
        MagicEffect::RestoreAttribute,
        MagicEffect::RestoreHealth,
        MagicEffect::RestoreMagicka,
        MagicEffect::RestoreFatigue,
        MagicEffect::RestoreSkill,
        MagicEffect::FortifyAttribute,
        MagicEffect::FortifyHealth,
        MagicEffect::FortifyMagicka,
        MagicEffect::FortifyFatigue,
        MagicEffect::FortifySkill,
        MagicEffect::FortifyMaximumMagicka,
        MagicEffect::AbsorbAttribute,
        MagicEffect::AbsorbHealth,
        MagicEffect::AbsorbMagicka,
        MagicEffect::AbsorbFatigue,
        MagicEffect::AbsorbSkill,
        MagicEffect::ResistFire,
        MagicEffect::ResistFrost,
        MagicEffect::ResistShock,
        MagicEffect::ResistMagicka,
        MagicEffect::ResistCommonDisease,
        MagicEffect::ResistBlightDisease,
        MagicEffect::ResistCorprusDisease,
        MagicEffect::ResistPoison,
        MagicEffect::ResistNormalWeapons,
        MagicEffect::ResistParalysis,
        MagicEffect::RemoveCurse,
        MagicEffect::TurnUndead,
        MagicEffect::SummonScamp,
        MagicEffect::SummonClannfear,
        MagicEffect::SummonDaedroth,
        MagicEffect::SummonDremora,
        MagicEffect::SummonAncestralGhost,
        MagicEffect::SummonSkeletalMinion,
        MagicEffect::SummonBonewalker,
        MagicEffect::SummonGreaterBonewalker,
        MagicEffect::SummonBonelord,
        MagicEffect::SummonWingedTwilight,
        MagicEffect::SummonHunger,
        MagicEffect::SummonGoldenSaint,
        MagicEffect::SummonFlameAtronach,
        MagicEffect::SummonFrostAtronach,
        MagicEffect::SummonStormAtronach,
        MagicEffect::FortifyAttack,
        MagicEffect::CommandCreature,
        MagicEffect::CommandHumanoid,
        MagicEffect::BoundDagger,
        MagicEffect::BoundLongsword,
        MagicEffect::BoundMace,
        MagicEffect::BoundBattleAxe,
        MagicEffect::BoundSpear,
        MagicEffect::BoundLongbow,
        MagicEffect::ExtraSpell,
        MagicEffect::BoundCuirass,
        MagicEffect::BoundHelm,
        MagicEffect::BoundBoots,
        MagicEffect::BoundShield,
        MagicEffect::BoundGloves,
        MagicEffect::Corprus,
        MagicEffect::Vampirism,
        MagicEffect::SummonCenturionSphere,
        MagicEffect::SunDamage,
        MagicEffect::StuntedMagicka,

        // tribunal
        MagicEffect::SummonFabricant,

        // bloodmoon
        MagicEffect::SummonWolf,
        MagicEffect::SummonBear,
        MagicEffect::SummonBonewolf,
        MagicEffect::SummonCreature04,
        MagicEffect::SummonCreature05,
     };

    template <typename Collection>
    static std::map<std::string_view, int, Misc::StringUtils::CiComp> initStringToIntMap(const Collection& strings)
    {
        std::map<std::string_view, int, Misc::StringUtils::CiComp> map;
        for (size_t i = 0; i < strings.size(); i++)
            map[strings[i]] = static_cast<int>(i);

        return map;
    }

    const std::map<std::string_view, int, Misc::StringUtils::CiComp> MagicEffect::sGmstEffectIdToIndexMap
        = initStringToIntMap(MagicEffect::sGmstEffectIds);

    MagicEffect::MagnitudeDisplayType MagicEffect::getMagnitudeDisplayType() const
    {
        int index = refIdToIndex(mId);
        if (mData.mFlags & NoMagnitude)
            return MDT_None;
        if (index == 84)
            return MDT_TimesInt;
        if (index == 59 || (index >= 64 && index <= 66))
            return MDT_Feet;
        if (index == 118 || index == 119)
            return MDT_Level;
        if ((index >= 28 && index <= 36) || (index >= 90 && index <= 99) || index == 40 || index == 47
            || index == 57 || index == 68)
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

    std::string_view MagicEffect::refIdToGmstString(const RefId& effectId)
    {
        if (effectId.empty())
            return {};
        int index = refIdToIndex(effectId);
        if (index < 0 || index >= Length)
            return effectId.getRefIdString();
        return sGmstEffectIds[index];
    }

    RefId MagicEffect::effectGmstIdToRefId(std::string_view gmstId)
    {
        auto name = sGmstEffectIdToIndexMap.find(gmstId);
        if (name == sGmstEffectIdToIndexMap.end())
            return {};
        return sMagicEffectIds[name->second];
    }

    RefId MagicEffect::indexToRefId(int index)
    {
        if (index < 0 || index >= Length)
            return {};
        return sMagicEffectIds[index];
    }

    int MagicEffect::refIdToIndex(const RefId& effectId)
    {
        if (effectId.empty())
            return -1;
        for (size_t i = 0; i < sMagicEffectIds.size(); ++i)
            if (sMagicEffectIds[i] == effectId)
                return static_cast<int>(i);
        return -1;
    }

    RefId MagicEffect::nameToRefId(std::string_view name)
    {
        for (const RefId& effect : sMagicEffectIds)
            if (effect.getRefIdString() == name)
                return effect;
        return {};
    }

    std::string_view MagicEffect::refIdToName(const RefId& effectId)
    {
        if (!effectId.empty() && effectId.getIf<MagicEffectId>())
            return effectId.getRefIdString();
        return {};
    }

    std::string_view MagicEffect::indexToName(int index)
    {
        if (index < 0 || index >= Length)
            return {};
        return sMagicEffectIds[index].getValue();
    }

    int MagicEffect::indexNameToIndex(std::string_view name)
    {
        for (size_t i = 0; i < sMagicEffectIds.size(); ++i)
            if (sMagicEffectIds[i].getValue() == name)
                return static_cast<int>(i);
        return -1;
    }
}
