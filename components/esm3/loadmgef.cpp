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

    const StringRefId MagicEffect::WaterBreathing("WaterBreathing");
    const StringRefId MagicEffect::SwiftSwim("SwiftSwim");
    const StringRefId MagicEffect::WaterWalking("WaterWalking");
    const StringRefId MagicEffect::Shield("Shield");
    const StringRefId MagicEffect::FireShield("FireShield");
    const StringRefId MagicEffect::LightningShield("LightningShield");
    const StringRefId MagicEffect::FrostShield("FrostShield");
    const StringRefId MagicEffect::Burden("Burden");
    const StringRefId MagicEffect::Feather("Feather");
    const StringRefId MagicEffect::Jump("Jump");
    const StringRefId MagicEffect::Levitate("Levitate");
    const StringRefId MagicEffect::SlowFall("SlowFall");
    const StringRefId MagicEffect::Lock("Lock");
    const StringRefId MagicEffect::Open("Open");
    const StringRefId MagicEffect::FireDamage("FireDamage");
    const StringRefId MagicEffect::ShockDamage("ShockDamage");
    const StringRefId MagicEffect::FrostDamage("FrostDamage");
    const StringRefId MagicEffect::DrainAttribute("DrainAttribute");
    const StringRefId MagicEffect::DrainHealth("DrainHealth");
    const StringRefId MagicEffect::DrainMagicka("DrainMagicka");
    const StringRefId MagicEffect::DrainFatigue("DrainFatigue");
    const StringRefId MagicEffect::DrainSkill("DrainSkill");
    const StringRefId MagicEffect::DamageAttribute("DamageAttribute");
    const StringRefId MagicEffect::DamageHealth("DamageHealth");
    const StringRefId MagicEffect::DamageMagicka("DamageMagicka");
    const StringRefId MagicEffect::DamageFatigue("DamageFatigue");
    const StringRefId MagicEffect::DamageSkill("DamageSkill");
    const StringRefId MagicEffect::Poison("Poison");
    const StringRefId MagicEffect::WeaknessToFire("WeaknessToFire");
    const StringRefId MagicEffect::WeaknessToFrost("WeaknessToFrost");
    const StringRefId MagicEffect::WeaknessToShock("WeaknessToShock");
    const StringRefId MagicEffect::WeaknessToMagicka("WeaknessToMagicka");
    const StringRefId MagicEffect::WeaknessToCommonDisease("WeaknessToCommonDisease");
    const StringRefId MagicEffect::WeaknessToBlightDisease("WeaknessToBlightDisease");
    const StringRefId MagicEffect::WeaknessToCorprusDisease("WeaknessToCorprusDisease");
    const StringRefId MagicEffect::WeaknessToPoison("WeaknessToPoison");
    const StringRefId MagicEffect::WeaknessToNormalWeapons("WeaknessToNormalWeapons");
    const StringRefId MagicEffect::DisintegrateWeapon("DisintegrateWeapon");
    const StringRefId MagicEffect::DisintegrateArmor("DisintegrateArmor");
    const StringRefId MagicEffect::Invisibility("Invisibility");
    const StringRefId MagicEffect::Chameleon("Chameleon");
    const StringRefId MagicEffect::Light("Light");
    const StringRefId MagicEffect::Sanctuary("Sanctuary");
    const StringRefId MagicEffect::NightEye("NightEye");
    const StringRefId MagicEffect::Charm("Charm");
    const StringRefId MagicEffect::Paralyze("Paralyze");
    const StringRefId MagicEffect::Silence("Silence");
    const StringRefId MagicEffect::Blind("Blind");
    const StringRefId MagicEffect::Sound("Sound");
    const StringRefId MagicEffect::CalmHumanoid("CalmHumanoid");
    const StringRefId MagicEffect::CalmCreature("CalmCreature");
    const StringRefId MagicEffect::FrenzyHumanoid("FrenzyHumanoid");
    const StringRefId MagicEffect::FrenzyCreature("FrenzyCreature");
    const StringRefId MagicEffect::DemoralizeHumanoid("DemoralizeHumanoid");
    const StringRefId MagicEffect::DemoralizeCreature("DemoralizeCreature");
    const StringRefId MagicEffect::RallyHumanoid("RallyHumanoid");
    const StringRefId MagicEffect::RallyCreature("RallyCreature");
    const StringRefId MagicEffect::Dispel("Dispel");
    const StringRefId MagicEffect::Soultrap("Soultrap");
    const StringRefId MagicEffect::Telekinesis("Telekinesis");
    const StringRefId MagicEffect::Mark("Mark");
    const StringRefId MagicEffect::Recall("Recall");
    const StringRefId MagicEffect::DivineIntervention("DivineIntervention");
    const StringRefId MagicEffect::AlmsiviIntervention("AlmsiviIntervention");
    const StringRefId MagicEffect::DetectAnimal("DetectAnimal");
    const StringRefId MagicEffect::DetectEnchantment("DetectEnchantment");
    const StringRefId MagicEffect::DetectKey("DetectKey");
    const StringRefId MagicEffect::SpellAbsorption("SpellAbsorption");
    const StringRefId MagicEffect::Reflect("Reflect");
    const StringRefId MagicEffect::CureCommonDisease("CureCommonDisease");
    const StringRefId MagicEffect::CureBlightDisease("CureBlightDisease");
    const StringRefId MagicEffect::CureCorprusDisease("CureCorprusDisease");
    const StringRefId MagicEffect::CurePoison("CurePoison");
    const StringRefId MagicEffect::CureParalyzation("CureParalyzation");
    const StringRefId MagicEffect::RestoreAttribute("RestoreAttribute");
    const StringRefId MagicEffect::RestoreHealth("RestoreHealth");
    const StringRefId MagicEffect::RestoreMagicka("RestoreMagicka");
    const StringRefId MagicEffect::RestoreFatigue("RestoreFatigue");
    const StringRefId MagicEffect::RestoreSkill("RestoreSkill");
    const StringRefId MagicEffect::FortifyAttribute("FortifyAttribute");
    const StringRefId MagicEffect::FortifyHealth("FortifyHealth");
    const StringRefId MagicEffect::FortifyMagicka("FortifyMagicka");
    const StringRefId MagicEffect::FortifyFatigue("FortifyFatigue");
    const StringRefId MagicEffect::FortifySkill("FortifySkill");
    const StringRefId MagicEffect::FortifyMaximumMagicka("FortifyMaximumMagicka");
    const StringRefId MagicEffect::AbsorbAttribute("AbsorbAttribute");
    const StringRefId MagicEffect::AbsorbHealth("AbsorbHealth");
    const StringRefId MagicEffect::AbsorbMagicka("AbsorbMagicka");
    const StringRefId MagicEffect::AbsorbFatigue("AbsorbFatigue");
    const StringRefId MagicEffect::AbsorbSkill("AbsorbSkill");
    const StringRefId MagicEffect::ResistFire("ResistFire");
    const StringRefId MagicEffect::ResistFrost("ResistFrost");
    const StringRefId MagicEffect::ResistShock("ResistShock");
    const StringRefId MagicEffect::ResistMagicka("ResistMagicka");
    const StringRefId MagicEffect::ResistCommonDisease("ResistCommonDisease");
    const StringRefId MagicEffect::ResistBlightDisease("ResistBlightDisease");
    const StringRefId MagicEffect::ResistCorprusDisease("ResistCorprusDisease");
    const StringRefId MagicEffect::ResistPoison("ResistPoison");
    const StringRefId MagicEffect::ResistNormalWeapons("ResistNormalWeapons");
    const StringRefId MagicEffect::ResistParalysis("ResistParalysis");
    const StringRefId MagicEffect::RemoveCurse("RemoveCurse");
    const StringRefId MagicEffect::TurnUndead("TurnUndead");
    const StringRefId MagicEffect::SummonScamp("SummonScamp");
    const StringRefId MagicEffect::SummonClannfear("SummonClannfear");
    const StringRefId MagicEffect::SummonDaedroth("SummonDaedroth");
    const StringRefId MagicEffect::SummonDremora("SummonDremora");
    const StringRefId MagicEffect::SummonAncestralGhost("SummonAncestralGhost");
    const StringRefId MagicEffect::SummonSkeletalMinion("SummonSkeletalMinion");
    const StringRefId MagicEffect::SummonBonewalker("SummonBonewalker");
    const StringRefId MagicEffect::SummonGreaterBonewalker("SummonGreaterBonewalker");
    const StringRefId MagicEffect::SummonBonelord("SummonBonelord");
    const StringRefId MagicEffect::SummonWingedTwilight("SummonWingedTwilight");
    const StringRefId MagicEffect::SummonHunger("SummonHunger");
    const StringRefId MagicEffect::SummonGoldenSaint("SummonGoldenSaint");
    const StringRefId MagicEffect::SummonFlameAtronach("SummonFlameAtronach");
    const StringRefId MagicEffect::SummonFrostAtronach("SummonFrostAtronach");
    const StringRefId MagicEffect::SummonStormAtronach("SummonStormAtronach");
    const StringRefId MagicEffect::FortifyAttack("FortifyAttack");
    const StringRefId MagicEffect::CommandCreature("CommandCreature");
    const StringRefId MagicEffect::CommandHumanoid("CommandHumanoid");
    const StringRefId MagicEffect::BoundDagger("BoundDagger");
    const StringRefId MagicEffect::BoundLongsword("BoundLongsword");
    const StringRefId MagicEffect::BoundMace("BoundMace");
    const StringRefId MagicEffect::BoundBattleAxe("BoundBattleAxe");
    const StringRefId MagicEffect::BoundSpear("BoundSpear");
    const StringRefId MagicEffect::BoundLongbow("BoundLongbow");
    const StringRefId MagicEffect::ExtraSpell("ExtraSpell");
    const StringRefId MagicEffect::BoundCuirass("BoundCuirass");
    const StringRefId MagicEffect::BoundHelm("BoundHelm");
    const StringRefId MagicEffect::BoundBoots("BoundBoots");
    const StringRefId MagicEffect::BoundShield("BoundShield");
    const StringRefId MagicEffect::BoundGloves("BoundGloves");
    const StringRefId MagicEffect::Corprus("Corprus");
    const StringRefId MagicEffect::Vampirism("Vampirism");
    const StringRefId MagicEffect::SummonCenturionSphere("SummonCenturionSphere");
    const StringRefId MagicEffect::SunDamage("SunDamage");
    const StringRefId MagicEffect::StuntedMagicka("StuntedMagicka");

    // Tribunal only
    const StringRefId MagicEffect::SummonFabricant("SummonFabricant");

    // Bloodmoon only
    const StringRefId MagicEffect::SummonWolf("SummonWolf");
    const StringRefId MagicEffect::SummonBear("SummonBear");
    const StringRefId MagicEffect::SummonBonewolf("SummonBonewolf");
    const StringRefId MagicEffect::SummonCreature04("SummonCreature04");
    const StringRefId MagicEffect::SummonCreature05("SummonCreature05");

    void MagicEffect::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // MagicEffect record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        int32_t index = -1;
        esm.getHNT(index, "INDX");
        if (index < 0 || index >= Length)
            esm.fail("Invalid Index!");

        mId = indexToRefId(index);

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
        std::unordered_map<RefId, RefId> makeResistancesMap()
        {
            std::unordered_map<RefId, RefId> effects{
                { MagicEffect::DisintegrateArmor, MagicEffect::Sanctuary },
                { MagicEffect::DisintegrateWeapon, MagicEffect::Sanctuary },

                { MagicEffect::DrainAttribute, MagicEffect::ResistMagicka },
                { MagicEffect::DrainHealth, MagicEffect::ResistMagicka },
                { MagicEffect::DrainMagicka, MagicEffect::ResistMagicka },
                { MagicEffect::DrainFatigue, MagicEffect::ResistMagicka },
                { MagicEffect::DrainSkill, MagicEffect::ResistMagicka },
                { MagicEffect::DamageAttribute, MagicEffect::ResistMagicka },
                { MagicEffect::DamageHealth, MagicEffect::ResistMagicka },
                { MagicEffect::DamageMagicka, MagicEffect::ResistMagicka },
                { MagicEffect::DamageFatigue, MagicEffect::ResistMagicka },
                { MagicEffect::DamageSkill, MagicEffect::ResistMagicka },

                { MagicEffect::AbsorbAttribute, MagicEffect::ResistMagicka },
                { MagicEffect::AbsorbHealth, MagicEffect::ResistMagicka },
                { MagicEffect::AbsorbMagicka, MagicEffect::ResistMagicka },
                { MagicEffect::AbsorbFatigue, MagicEffect::ResistMagicka },
                { MagicEffect::AbsorbSkill, MagicEffect::ResistMagicka },

                { MagicEffect::WeaknessToFire, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToFrost, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToShock, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToMagicka, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToCommonDisease, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToBlightDisease, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToCorprusDisease, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToPoison, MagicEffect::ResistMagicka },
                { MagicEffect::WeaknessToNormalWeapons, MagicEffect::ResistMagicka },

                { MagicEffect::Burden, MagicEffect::ResistMagicka },
                { MagicEffect::Charm, MagicEffect::ResistMagicka },
                { MagicEffect::Silence, MagicEffect::ResistMagicka },
                { MagicEffect::Blind, MagicEffect::ResistMagicka },
                { MagicEffect::Sound, MagicEffect::ResistMagicka },

                { MagicEffect::CalmHumanoid, MagicEffect::ResistMagicka },
                { MagicEffect::CalmCreature, MagicEffect::ResistMagicka },
                { MagicEffect::FrenzyHumanoid, MagicEffect::ResistMagicka },
                { MagicEffect::FrenzyCreature, MagicEffect::ResistMagicka },
                { MagicEffect::DemoralizeHumanoid, MagicEffect::ResistMagicka },
                { MagicEffect::DemoralizeCreature, MagicEffect::ResistMagicka },
                { MagicEffect::RallyHumanoid, MagicEffect::ResistMagicka },
                { MagicEffect::RallyCreature, MagicEffect::ResistMagicka },

                { MagicEffect::TurnUndead, MagicEffect::ResistMagicka },

                { MagicEffect::FireDamage, MagicEffect::ResistFire },
                { MagicEffect::FrostDamage, MagicEffect::ResistFrost },
                { MagicEffect::ShockDamage, MagicEffect::ResistShock },
                { MagicEffect::Vampirism, MagicEffect::ResistCommonDisease },
                { MagicEffect::Corprus, MagicEffect::ResistCorprusDisease },
                { MagicEffect::Poison, MagicEffect::ResistPoison },
                { MagicEffect::Paralyze, MagicEffect::ResistParalysis },
            };
            return effects;
        }
    }

    RefId MagicEffect::getResistanceEffect(RefId effectId)
    {
        // Source https://wiki.openmw.org/index.php?title=Research:Magic#Effect_attribute

        // <Effect, Effect providing resistance against first effect>
        static const std::unordered_map<RefId, RefId> effects = makeResistancesMap();

        if (const auto it = effects.find(effectId); it != effects.end())
            return it->second;

        return {};
    }

    namespace
    {
        std::unordered_map<RefId, RefId> makeWeaknessesMap()
        {
            std::unordered_map<RefId, RefId> effects{
                { MagicEffect::DrainAttribute, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DrainHealth, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DrainMagicka, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DrainFatigue, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DrainSkill, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DamageAttribute, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DamageHealth, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DamageMagicka, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DamageFatigue, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DamageSkill, MagicEffect::WeaknessToMagicka },

                { MagicEffect::AbsorbAttribute, MagicEffect::WeaknessToMagicka },
                { MagicEffect::AbsorbHealth, MagicEffect::WeaknessToMagicka },
                { MagicEffect::AbsorbMagicka, MagicEffect::WeaknessToMagicka },
                { MagicEffect::AbsorbFatigue, MagicEffect::WeaknessToMagicka },
                { MagicEffect::AbsorbSkill, MagicEffect::WeaknessToMagicka },

                { MagicEffect::WeaknessToFire, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToFrost, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToShock, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToMagicka, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToCommonDisease, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToBlightDisease, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToCorprusDisease, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToPoison, MagicEffect::WeaknessToMagicka },
                { MagicEffect::WeaknessToNormalWeapons, MagicEffect::WeaknessToMagicka },

                { MagicEffect::Burden, MagicEffect::WeaknessToMagicka },
                { MagicEffect::Charm, MagicEffect::WeaknessToMagicka },
                { MagicEffect::Silence, MagicEffect::WeaknessToMagicka },
                { MagicEffect::Blind, MagicEffect::WeaknessToMagicka },
                { MagicEffect::Sound, MagicEffect::WeaknessToMagicka },

                { MagicEffect::CalmHumanoid, MagicEffect::WeaknessToMagicka },
                { MagicEffect::CalmCreature, MagicEffect::WeaknessToMagicka },
                { MagicEffect::FrenzyHumanoid, MagicEffect::WeaknessToMagicka },
                { MagicEffect::FrenzyCreature, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DemoralizeHumanoid, MagicEffect::WeaknessToMagicka },
                { MagicEffect::DemoralizeCreature, MagicEffect::WeaknessToMagicka },
                { MagicEffect::RallyHumanoid, MagicEffect::WeaknessToMagicka },
                { MagicEffect::RallyCreature, MagicEffect::WeaknessToMagicka },

                { MagicEffect::TurnUndead, MagicEffect::WeaknessToMagicka },

                { MagicEffect::FireDamage, MagicEffect::WeaknessToFire },
                { MagicEffect::FrostDamage, MagicEffect::WeaknessToFrost },
                { MagicEffect::ShockDamage, MagicEffect::WeaknessToShock },
                { MagicEffect::Vampirism, MagicEffect::WeaknessToCommonDisease },
                { MagicEffect::Corprus, MagicEffect::WeaknessToCorprusDisease },
                { MagicEffect::Poison, MagicEffect::WeaknessToPoison },
            };
            return effects;
        }
    }

    RefId MagicEffect::getWeaknessEffect(RefId effectId)
    {
        static const std::unordered_map<RefId, RefId> effects = makeWeaknessesMap();

        if (const auto it = effects.find(effectId); it != effects.end())
            return it->second;

        return {};
    }

    // Map effect index to GMST name
    static constexpr std::array<std::string_view, MagicEffect::Length> sGmstEffectIds = {
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

    static const std::array<RefId, MagicEffect::Length> sMagicEffectIds{
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

    template <typename Collection, typename Comparator = std::less<typename Collection::value_type>>
    static std::map<typename Collection::value_type, int, Comparator> initToIndexMap(
        const Collection& data, Comparator comp = Comparator())
    {
        std::map<typename Collection::value_type, int, Comparator> map(comp);
        for (size_t i = 0; i < data.size(); ++i)
            map[data[i]] = static_cast<int>(i);
        return map;
    }

    const std::map<std::string_view, int, Misc::StringUtils::CiComp> sGmstEffectIdToIndexMap
        = initToIndexMap(sGmstEffectIds, Misc::StringUtils::CiComp());

    const std::map<RefId, int> sMagicEffectIdToIndexMap = initToIndexMap(sMagicEffectIds);

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
        if ((index >= 28 && index <= 36) || (index >= 90 && index <= 99) || index == 40 || index == 47 || index == 57
            || index == 68)
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

    std::string_view MagicEffect::refIdToGmstString(RefId effectId)
    {
        int index = refIdToIndex(effectId);
        if (index < 0 || index >= Length)
            return {};
        return sGmstEffectIds[index];
    }

    RefId MagicEffect::effectGmstIdToRefId(std::string_view gmstId)
    {
        auto it = sGmstEffectIdToIndexMap.find(gmstId);
        if (it == sGmstEffectIdToIndexMap.end())
            return {};
        return sMagicEffectIds[it->second];
    }

    RefId MagicEffect::indexToRefId(int index)
    {
        if (index < 0 || index >= Length)
            return {};
        return sMagicEffectIds[index];
    }

    int MagicEffect::refIdToIndex(RefId effectId)
    {
        const auto it = sMagicEffectIdToIndexMap.find(effectId);
        if (it != sMagicEffectIdToIndexMap.end())
            return it->second;
        return -1;
    }

    std::string_view MagicEffect::indexToName(int index)
    {
        if (index < 0 || index >= Length)
            return {};
        return sMagicEffectIds[index].getRefIdString();
    }
}
