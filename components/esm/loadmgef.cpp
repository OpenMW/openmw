#include "loadmgef.hpp"

#include <sstream>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace
{
    static const char *sIds[ESM::MagicEffect::Length] =
    {
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
        "SummonCreature05"
    };

    const int NumberOfHardcodedFlags = 143;
    const int HardcodedFlags[NumberOfHardcodedFlags] = {
        0x11c8, 0x11c0, 0x11c8, 0x11e0, 0x11e0, 0x11e0, 0x11e0, 0x11d0,
        0x11c0, 0x11c0, 0x11e0, 0x11c0, 0x11184, 0x11184, 0x1f0, 0x1f0,
        0x1f0, 0x11d2, 0x11f0, 0x11d0, 0x11d0, 0x11d1, 0x1d2, 0x1f0,
        0x1d0, 0x1d0, 0x1d1, 0x1f0, 0x11d0, 0x11d0, 0x11d0, 0x11d0,
        0x11d0, 0x11d0, 0x11d0, 0x11d0, 0x11d0, 0x1d0, 0x1d0, 0x11c8,
        0x31c0, 0x11c0, 0x11c0, 0x11c0, 0x1180, 0x11d8, 0x11d8, 0x11d0,
        0x11d0, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180,
        0x11180, 0x11c4, 0x111b8, 0x1040, 0x104c, 0x104c, 0x104c, 0x104c,
        0x1040, 0x1040, 0x1040, 0x11c0, 0x11c0, 0x1cc, 0x1cc, 0x1cc,
        0x1cc, 0x1cc, 0x1c2, 0x1c0, 0x1c0, 0x1c0, 0x1c1, 0x11c2,
        0x11c0, 0x11c0, 0x11c0, 0x11c1, 0x11c0, 0x21192, 0x20190, 0x20190,
        0x20190, 0x21191, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0,
        0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x1c0, 0x11190, 0x9048, 0x9048,
        0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048,
        0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x11c0, 0x1180, 0x1180,
        0x5048, 0x5048, 0x5048, 0x5048, 0x5048, 0x5048, 0x1188, 0x5048,
        0x5048, 0x5048, 0x5048, 0x5048, 0x1048, 0x104c, 0x1048, 0x40,
        0x11c8, 0x1048, 0x1048, 0x1048, 0x1048, 0x1048, 0x1048
    };
}

namespace ESM
{
    unsigned int MagicEffect::sRecordId = REC_MGEF;

void MagicEffect::load(ESMReader &esm, bool &isDeleted)
{
    isDeleted = false; // MagicEffect record can't be deleted now (may be changed in the future)

    esm.getHNT(mIndex, "INDX");

    mId = indexToId (mIndex);

    esm.getHNT(mData, "MEDT", 36);
    if (esm.getFormat() == 0)
    {
        // don't allow mods to change fixed flags in the legacy format
        mData.mFlags &= (AllowSpellmaking | AllowEnchanting | NegativeLight);
        if (mIndex>=0 && mIndex<NumberOfHardcodedFlags)
        mData.mFlags |= HardcodedFlags[mIndex];
    }

    // vanilla MW accepts the _SND subrecords before or after DESC... I hope
    // this isn't true for other records, or we have to do a mass-refactor
    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        switch (esm.retSubName().intval)
        {
            case ESM::FourCC<'I','T','E','X'>::value:
                mIcon = esm.getHString();
                break;
            case ESM::FourCC<'P','T','E','X'>::value:
                mParticle = esm.getHString();
                break;
            case ESM::FourCC<'B','S','N','D'>::value:
                mBoltSound = esm.getHString();
                break;
            case ESM::FourCC<'C','S','N','D'>::value:
                mCastSound = esm.getHString();
                break;
            case ESM::FourCC<'H','S','N','D'>::value:
                mHitSound = esm.getHString();
                break;
            case ESM::FourCC<'A','S','N','D'>::value:
                mAreaSound = esm.getHString();
                break;
            case ESM::FourCC<'C','V','F','X'>::value:
                mCasting = esm.getHString();
                break;
            case ESM::FourCC<'B','V','F','X'>::value:
                mBolt = esm.getHString();
                break;
            case ESM::FourCC<'H','V','F','X'>::value:
                mHit = esm.getHString();
                break;
            case ESM::FourCC<'A','V','F','X'>::value:
                mArea = esm.getHString();
                break;
            case ESM::FourCC<'D','E','S','C'>::value:
                mDescription = esm.getHString();
                break;
            default:
                esm.fail("Unknown subrecord");
        }
    }
}
void MagicEffect::save(ESMWriter &esm, bool /*isDeleted*/) const
{
    esm.writeHNT("INDX", mIndex);

    esm.writeHNT("MEDT", mData, 36);

    esm.writeHNOCString("ITEX", mIcon);
    esm.writeHNOCString("PTEX", mParticle);
    esm.writeHNOCString("BSND", mBoltSound);
    esm.writeHNOCString("CSND", mCastSound);
    esm.writeHNOCString("HSND", mHitSound);
    esm.writeHNOCString("ASND", mAreaSound);

    esm.writeHNOCString("CVFX", mCasting);
    esm.writeHNOCString("BVFX", mBolt);
    esm.writeHNOCString("HVFX", mHit);
    esm.writeHNOCString("AVFX", mArea);

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

        for (int i=0; i<5; ++i)
            effects[DrainAttribute+i] = ResistMagicka;
        for (int i=0; i<5; ++i)
            effects[DamageAttribute+i] = ResistMagicka;
        for (int i=0; i<5; ++i)
            effects[AbsorbAttribute+i] = ResistMagicka;
        for (int i=0; i<10; ++i)
            effects[WeaknessToFire+i] = ResistMagicka;

        effects[Burden] = ResistMagicka;
        effects[Charm] = ResistMagicka;
        effects[Silence] = ResistMagicka;
        effects[Blind] = ResistMagicka;
        effects[Sound] = ResistMagicka;

        for (int i=0; i<2; ++i)
        {
            effects[CalmHumanoid+i] = ResistMagicka;
            effects[FrenzyHumanoid+i] = ResistMagicka;
            effects[DemoralizeHumanoid+i] = ResistMagicka;
            effects[RallyHumanoid+i] = ResistMagicka;
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
        for (int i=0; i<5; ++i)
            effects[DrainAttribute+i] = WeaknessToMagicka;
        for (int i=0; i<5; ++i)
            effects[DamageAttribute+i] = WeaknessToMagicka;
        for (int i=0; i<5; ++i)
            effects[AbsorbAttribute+i] = WeaknessToMagicka;
        for (int i=0; i<10; ++i)
            effects[WeaknessToFire+i] = WeaknessToMagicka;

        effects[Burden] = WeaknessToMagicka;
        effects[Charm] = WeaknessToMagicka;
        effects[Silence] = WeaknessToMagicka;
        effects[Blind] = WeaknessToMagicka;
        effects[Sound] = WeaknessToMagicka;

        for (int i=0; i<2; ++i)
        {
            effects[CalmHumanoid+i] = WeaknessToMagicka;
            effects[FrenzyHumanoid+i] = WeaknessToMagicka;
            effects[DemoralizeHumanoid+i] = WeaknessToMagicka;
            effects[RallyHumanoid+i] = WeaknessToMagicka;
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

static std::map<short,std::string> genNameMap()
{
    // Map effect ID to GMST name
    // http://www.uesp.net/morrow/hints/mweffects.shtml
    std::map<short, std::string> names;
    names[85] ="sEffectAbsorbAttribute";
    names[88] ="sEffectAbsorbFatigue";
    names[86] ="sEffectAbsorbHealth";
    names[87] ="sEffectAbsorbSpellPoints";
    names[89] ="sEffectAbsorbSkill";
    names[63] ="sEffectAlmsiviIntervention";
    names[47] ="sEffectBlind";
    names[123] ="sEffectBoundBattleAxe";
    names[129] ="sEffectBoundBoots";
    names[127] ="sEffectBoundCuirass";
    names[120] ="sEffectBoundDagger";
    names[131] ="sEffectBoundGloves";
    names[128] ="sEffectBoundHelm";
    names[125] ="sEffectBoundLongbow";
    names[126] ="sEffectExtraSpell";
    names[121] ="sEffectBoundLongsword";
    names[122] ="sEffectBoundMace";
    names[130] ="sEffectBoundShield";
    names[124] ="sEffectBoundSpear";
    names[7] ="sEffectBurden";
    names[50] ="sEffectCalmCreature";
    names[49] ="sEffectCalmHumanoid";
    names[40] ="sEffectChameleon";
    names[44] ="sEffectCharm";
    names[118] ="sEffectCommandCreatures";
    names[119] ="sEffectCommandHumanoids";
    names[132] ="sEffectCorpus"; // NB this typo. (bethesda made it)
    names[70] ="sEffectCureBlightDisease";
    names[69] ="sEffectCureCommonDisease";
    names[71] ="sEffectCureCorprusDisease";
    names[73] ="sEffectCureParalyzation";
    names[72] ="sEffectCurePoison";
    names[22] ="sEffectDamageAttribute";
    names[25] ="sEffectDamageFatigue";
    names[23] ="sEffectDamageHealth";
    names[24] ="sEffectDamageMagicka";
    names[26] ="sEffectDamageSkill";
    names[54] ="sEffectDemoralizeCreature";
    names[53] ="sEffectDemoralizeHumanoid";
    names[64] ="sEffectDetectAnimal";
    names[65] ="sEffectDetectEnchantment";
    names[66] ="sEffectDetectKey";
    names[38] ="sEffectDisintegrateArmor";
    names[37] ="sEffectDisintegrateWeapon";
    names[57] ="sEffectDispel";
    names[62] ="sEffectDivineIntervention";
    names[17] ="sEffectDrainAttribute";
    names[20] ="sEffectDrainFatigue";
    names[18] ="sEffectDrainHealth";
    names[19] ="sEffectDrainSpellpoints";
    names[21] ="sEffectDrainSkill";
    names[8] ="sEffectFeather";
    names[14] ="sEffectFireDamage";
    names[4] ="sEffectFireShield";
    names[117] ="sEffectFortifyAttackBonus";
    names[79] ="sEffectFortifyAttribute";
    names[82] ="sEffectFortifyFatigue";
    names[80] ="sEffectFortifyHealth";
    names[81] ="sEffectFortifySpellpoints";
    names[84] ="sEffectFortifyMagickaMultiplier";
    names[83] ="sEffectFortifySkill";
    names[52] ="sEffectFrenzyCreature";
    names[51] ="sEffectFrenzyHumanoid";
    names[16] ="sEffectFrostDamage";
    names[6] ="sEffectFrostShield";
    names[39] ="sEffectInvisibility";
    names[9] ="sEffectJump";
    names[10] ="sEffectLevitate";
    names[41] ="sEffectLight";
    names[5] ="sEffectLightningShield";
    names[12] ="sEffectLock";
    names[60] ="sEffectMark";
    names[43] ="sEffectNightEye";
    names[13] ="sEffectOpen";
    names[45] ="sEffectParalyze";
    names[27] ="sEffectPoison";
    names[56] ="sEffectRallyCreature";
    names[55] ="sEffectRallyHumanoid";
    names[61] ="sEffectRecall";
    names[68] ="sEffectReflect";
    names[100] ="sEffectRemoveCurse";
    names[95] ="sEffectResistBlightDisease";
    names[94] ="sEffectResistCommonDisease";
    names[96] ="sEffectResistCorprusDisease";
    names[90] ="sEffectResistFire";
    names[91] ="sEffectResistFrost";
    names[93] ="sEffectResistMagicka";
    names[98] ="sEffectResistNormalWeapons";
    names[99] ="sEffectResistParalysis";
    names[97] ="sEffectResistPoison";
    names[92] ="sEffectResistShock";
    names[74] ="sEffectRestoreAttribute";
    names[77] ="sEffectRestoreFatigue";
    names[75] ="sEffectRestoreHealth";
    names[76] ="sEffectRestoreSpellPoints";
    names[78] ="sEffectRestoreSkill";
    names[42] ="sEffectSanctuary";
    names[3] ="sEffectShield";
    names[15] ="sEffectShockDamage";
    names[46] ="sEffectSilence";
    names[11] ="sEffectSlowFall";
    names[58] ="sEffectSoultrap";
    names[48] ="sEffectSound";
    names[67] ="sEffectSpellAbsorption";
    names[136] ="sEffectStuntedMagicka";
    names[106] ="sEffectSummonAncestralGhost";
    names[110] ="sEffectSummonBonelord";
    names[108] ="sEffectSummonLeastBonewalker";
    names[134] ="sEffectSummonCenturionSphere";
    names[103] ="sEffectSummonClannfear";
    names[104] ="sEffectSummonDaedroth";
    names[105] ="sEffectSummonDremora";
    names[114] ="sEffectSummonFlameAtronach";
    names[115] ="sEffectSummonFrostAtronach";
    names[113] ="sEffectSummonGoldenSaint";
    names[109] ="sEffectSummonGreaterBonewalker";
    names[112] ="sEffectSummonHunger";
    names[102] ="sEffectSummonScamp";
    names[107] ="sEffectSummonSkeletalMinion";
    names[116] ="sEffectSummonStormAtronach";
    names[111] ="sEffectSummonWingedTwilight";
    names[135] ="sEffectSunDamage";
    names[1] ="sEffectSwiftSwim";
    names[59] ="sEffectTelekinesis";
    names[101] ="sEffectTurnUndead";
    names[133] ="sEffectVampirism";
    names[0] ="sEffectWaterBreathing";
    names[2] ="sEffectWaterWalking";
    names[33] ="sEffectWeaknesstoBlightDisease";
    names[32] ="sEffectWeaknesstoCommonDisease";
    names[34] ="sEffectWeaknesstoCorprusDisease";
    names[28] ="sEffectWeaknesstoFire";
    names[29] ="sEffectWeaknesstoFrost";
    names[31] ="sEffectWeaknesstoMagicka";
    names[36] ="sEffectWeaknesstoNormalWeapons";
    names[35] ="sEffectWeaknesstoPoison";
    names[30] ="sEffectWeaknesstoShock";

    // bloodmoon
    names[138] ="sEffectSummonCreature01";
    names[139] ="sEffectSummonCreature02";
    names[140] ="sEffectSummonCreature03";
    names[141] ="sEffectSummonCreature04";
    names[142] ="sEffectSummonCreature05";

    // tribunal
    names[137] ="sEffectSummonFabricant";

    return names;
}
const std::map<short,std::string> MagicEffect::sNames = genNameMap();

const std::string &MagicEffect::effectIdToString(short effectID)
{
    std::map<short,std::string>::const_iterator name = sNames.find(effectID);
    if(name == sNames.end())
        throw std::runtime_error(std::string("Unimplemented effect ID ")+std::to_string(effectID));

    return name->second;
}

class FindSecond {
    const std::string &mName;

public:
    FindSecond(const std::string &name) : mName(name) { }

    bool operator()(const std::pair<short,std::string> &item) const
    {
        if(Misc::StringUtils::ciEqual(item.second, mName))
            return true;
        return false;
    }
};

short MagicEffect::effectStringToId(const std::string &effect)
{
    std::map<short,std::string>::const_iterator name;

    name = std::find_if(sNames.begin(), sNames.end(), FindSecond(effect));
    if(name == sNames.end())
        throw std::runtime_error(std::string("Unimplemented effect ")+effect);

    return name->first;
}

MagicEffect::MagnitudeDisplayType MagicEffect::getMagnitudeDisplayType() const {
    if ( mData.mFlags & NoMagnitude )
        return MDT_None;
    if ( mIndex == 84 )
        return MDT_TimesInt;
    if ( mIndex == 59 ||
        ( mIndex >= 64 && mIndex <= 66) )
        return MDT_Feet;
    if ( mIndex == 118 || mIndex == 119 )
        return MDT_Level;
    if (   ( mIndex >= 28 && mIndex <= 36 )
        || ( mIndex >= 90 && mIndex <= 99 )
        ||   mIndex == 40 || mIndex == 47
        ||   mIndex == 57 || mIndex == 68 )
        return MDT_Percentage;

    return MDT_Points;
}

    void MagicEffect::blank()
    {
        mData.mSchool = 0;
        mData.mBaseCost = 0;
        mData.mFlags = 0;
        mData.mRed = 0;
        mData.mGreen = 0;
        mData.mBlue = 0;
        mData.mSpeed = 0;

        mIcon.clear();
        mParticle.clear();
        mCasting.clear();
        mHit.clear();
        mArea.clear();
        mBolt.clear();
        mCastSound.clear();
        mBoltSound.clear();
        mHitSound.clear();
        mAreaSound.clear();
        mDescription.clear();
    }

    std::string MagicEffect::indexToId (int index)
    {
        std::ostringstream stream;

        if (index!=-1)
        {
            stream << "#";

            if (index<100)
            {
                stream << "0";

                if (index<10)
                    stream << "0";
            }

            stream << index;

            if (index>=0 && index<Length)
                stream << sIds[index];
        }

        return stream.str();
    }
}
