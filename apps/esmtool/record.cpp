#include "record.hpp"
#include "labels.hpp"

#include <iostream>
#include <sstream>

#include <components/misc/stringops.hpp>

namespace
{

void printAIPackage(ESM::AIPackage p)
{
    std::cout << "  AI Type: " << aiTypeLabel(p.mType)
              << " (" << Misc::StringUtils::format("0x%08X", p.mType) << ")" << std::endl;
    if (p.mType == ESM::AI_Wander)
    {
        std::cout << "    Distance: " << p.mWander.mDistance << std::endl;
        std::cout << "    Duration: " << p.mWander.mDuration << std::endl;
        std::cout << "    Time of Day: " << (int)p.mWander.mTimeOfDay << std::endl;
        if (p.mWander.mShouldRepeat != 1)
            std::cout << "    Should repeat: " << (bool)(p.mWander.mShouldRepeat != 0) << std::endl;

        std::cout << "    Idle: ";
        for (int i = 0; i != 8; i++)
            std::cout << (int)p.mWander.mIdle[i] << " ";
        std::cout << std::endl;
    }
    else if (p.mType == ESM::AI_Travel)
    {
        std::cout << "    Travel Coordinates: (" << p.mTravel.mX << ","
                  << p.mTravel.mY << "," << p.mTravel.mZ << ")" << std::endl;
        std::cout << "    Travel Unknown: " << p.mTravel.mUnk << std::endl;
    }
    else if (p.mType == ESM::AI_Follow || p.mType == ESM::AI_Escort)
    {
        std::cout << "    Follow Coordinates: (" << p.mTarget.mX << ","
                  << p.mTarget.mY << "," << p.mTarget.mZ << ")" << std::endl;
        std::cout << "    Duration: " << p.mTarget.mDuration << std::endl;
        std::cout << "    Target ID: " << p.mTarget.mId.toString() << std::endl;
        std::cout << "    Unknown: " << p.mTarget.mUnk << std::endl;
    }
    else if (p.mType == ESM::AI_Activate)
    {
        std::cout << "    Name: " << p.mActivate.mName.toString() << std::endl;
        std::cout << "    Activate Unknown: " << p.mActivate.mUnk << std::endl;
    }
    else {
        std::cout << "    BadPackage: " << Misc::StringUtils::format("0x%08X", p.mType) << std::endl;
    }

    if (!p.mCellName.empty())
        std::cout << "    Cell Name: " << p.mCellName << std::endl;
}

std::string ruleString(ESM::DialInfo::SelectStruct ss)
{
    std::string rule = ss.mSelectRule;

    if (rule.length() < 5)
        return "INVALID";

    char type = rule[1];
    char indicator = rule[2];

    std::string type_str = "INVALID";
    std::string func_str = Misc::StringUtils::format("INVALID=%s", rule.substr(1,3));
    int func;
    std::istringstream iss(rule.substr(2,2));
    iss >> func;

    switch(type)
    {
    case '1':
        type_str = "Function";
        func_str = ruleFunction(func);
        break;
    case '2':
        if (indicator == 's') type_str = "Global short";
        else if (indicator == 'l') type_str = "Global long";
        else if (indicator == 'f') type_str = "Global float";
        break;
    case '3':
        if (indicator == 's') type_str = "Local short";
        else if (indicator == 'l') type_str = "Local long";
        else if (indicator == 'f') type_str = "Local float";
        break;
    case '4': if (indicator == 'J') type_str = "Journal"; break;
    case '5': if (indicator == 'I') type_str = "Item type"; break;
    case '6': if (indicator == 'D') type_str = "NPC Dead"; break;
    case '7': if (indicator == 'X') type_str = "Not ID"; break;
    case '8': if (indicator == 'F') type_str = "Not Faction"; break;
    case '9': if (indicator == 'C') type_str = "Not Class"; break;
    case 'A': if (indicator == 'R') type_str = "Not Race"; break;
    case 'B': if (indicator == 'L') type_str = "Not Cell"; break;
    case 'C': if (indicator == 's') type_str = "Not Local"; break;
    default: break;
    }

    // Append the variable name to the function string if any.
    if (type != '1') func_str = rule.substr(5);

    // In the previous switch, we assumed that the second char was X
    // for all types not qual to one.  If this wasn't true, go back to
    // the error message.
    if (type != '1' && rule[3] != 'X')
        func_str = Misc::StringUtils::format("INVALID=%s", rule.substr(1,3));

    char oper = rule[4];
    std::string oper_str = "??";
    switch (oper)
    {
    case '0': oper_str = "=="; break;
    case '1': oper_str = "!="; break;
    case '2': oper_str = "> "; break;
    case '3': oper_str = ">="; break;
    case '4': oper_str = "< "; break;
    case '5': oper_str = "<="; break;
    default: break;
    }

    std::ostringstream stream;
    stream << ss.mValue;

    std::string result = Misc::StringUtils::format("%-12s %-32s %2s %s", type_str, func_str, oper_str, stream.str());
    return result;
}

void printEffectList(ESM::EffectList effects)
{
    int i = 0;
    for (const ESM::ENAMstruct& effect : effects.mList)
    {
        std::cout << "  Effect[" << i << "]: " << magicEffectLabel(effect.mEffectID)
                  << " (" << effect.mEffectID << ")" << std::endl;
        if (effect.mSkill != -1)
            std::cout << "    Skill: " << skillLabel(effect.mSkill)
                      << " (" << (int)effect.mSkill << ")" << std::endl;
        if (effect.mAttribute != -1)
            std::cout << "    Attribute: " << attributeLabel(effect.mAttribute)
                      << " (" << (int)effect.mAttribute << ")" << std::endl;
        std::cout << "    Range: " << rangeTypeLabel(effect.mRange)
                  << " (" << effect.mRange << ")" << std::endl;
        // Area is always zero if range type is "Self"
        if (effect.mRange != ESM::RT_Self)
            std::cout << "    Area: " << effect.mArea << std::endl;
        std::cout << "    Duration: " << effect.mDuration << std::endl;
        std::cout << "    Magnitude: " << effect.mMagnMin << "-" << effect.mMagnMax << std::endl;
        i++;
    }
}

void printTransport(const std::vector<ESM::Transport::Dest>& transport)
{
    for (const ESM::Transport::Dest& dest : transport)
    {
        std::cout << "  Destination Position: "
                  << Misc::StringUtils::format("%12.3f", dest.mPos.pos[0]) << ","
                  << Misc::StringUtils::format("%12.3f", dest.mPos.pos[1]) << ","
                  << Misc::StringUtils::format("%12.3f", dest.mPos.pos[2]) << ")" << std::endl;
        std::cout << "  Destination Rotation: "
                  << Misc::StringUtils::format("%9.6f", dest.mPos.rot[0]) << ","
                  << Misc::StringUtils::format("%9.6f", dest.mPos.rot[1]) << ","
                  << Misc::StringUtils::format("%9.6f", dest.mPos.rot[2]) << ")" << std::endl;
        if (!dest.mCellName.empty())
            std::cout << "  Destination Cell: " << dest.mCellName << std::endl;
    }
}

}

namespace EsmTool {

RecordBase *
RecordBase::create(ESM::NAME type)
{
    RecordBase *record = 0;

    switch (type.intval) {
    case ESM::REC_ACTI:
    {
        record = new EsmTool::Record<ESM::Activator>;
        break;
    }
    case ESM::REC_ALCH:
    {
        record = new EsmTool::Record<ESM::Potion>;
        break;
    }
    case ESM::REC_APPA:
    {
        record = new EsmTool::Record<ESM::Apparatus>;
        break;
    }
    case ESM::REC_ARMO:
    {
        record = new EsmTool::Record<ESM::Armor>;
        break;
    }
    case ESM::REC_BODY:
    {
        record = new EsmTool::Record<ESM::BodyPart>;
        break;
    }
    case ESM::REC_BOOK:
    {
        record = new EsmTool::Record<ESM::Book>;
        break;
    }
    case ESM::REC_BSGN:
    {
        record = new EsmTool::Record<ESM::BirthSign>;
        break;
    }
    case ESM::REC_CELL:
    {
        record = new EsmTool::Record<ESM::Cell>;
        break;
    }
    case ESM::REC_CLAS:
    {
        record = new EsmTool::Record<ESM::Class>;
        break;
    }
    case ESM::REC_CLOT:
    {
        record = new EsmTool::Record<ESM::Clothing>;
        break;
    }
    case ESM::REC_CONT:
    {
        record = new EsmTool::Record<ESM::Container>;
        break;
    }
    case ESM::REC_CREA:
    {
        record = new EsmTool::Record<ESM::Creature>;
        break;
    }
    case ESM::REC_DIAL:
    {
        record = new EsmTool::Record<ESM::Dialogue>;
        break;
    }
    case ESM::REC_DOOR:
    {
        record = new EsmTool::Record<ESM::Door>;
        break;
    }
    case ESM::REC_ENCH:
    {
        record = new EsmTool::Record<ESM::Enchantment>;
        break;
    }
    case ESM::REC_FACT:
    {
        record = new EsmTool::Record<ESM::Faction>;
        break;
    }
    case ESM::REC_GLOB:
    {
        record = new EsmTool::Record<ESM::Global>;
        break;
    }
    case ESM::REC_GMST:
    {
        record = new EsmTool::Record<ESM::GameSetting>;
        break;
    }
    case ESM::REC_INFO:
    {
        record = new EsmTool::Record<ESM::DialInfo>;
        break;
    }
    case ESM::REC_INGR:
    {
        record = new EsmTool::Record<ESM::Ingredient>;
        break;
    }
    case ESM::REC_LAND:
    {
        record = new EsmTool::Record<ESM::Land>;
        break;
    }
    case ESM::REC_LEVI:
    {
        record = new EsmTool::Record<ESM::ItemLevList>;
        break;
    }
    case ESM::REC_LEVC:
    {
        record = new EsmTool::Record<ESM::CreatureLevList>;
        break;
    }
    case ESM::REC_LIGH:
    {
        record = new EsmTool::Record<ESM::Light>;
        break;
    }
    case ESM::REC_LOCK:
    {
        record = new EsmTool::Record<ESM::Lockpick>;
        break;
    }
    case ESM::REC_LTEX:
    {
        record = new EsmTool::Record<ESM::LandTexture>;
        break;
    }
    case ESM::REC_MISC:
    {
        record = new EsmTool::Record<ESM::Miscellaneous>;
        break;
    }
    case ESM::REC_MGEF:
    {
        record = new EsmTool::Record<ESM::MagicEffect>;
        break;
    }
    case ESM::REC_NPC_:
    {
        record = new EsmTool::Record<ESM::NPC>;
        break;
    }
    case ESM::REC_PGRD:
    {
        record = new EsmTool::Record<ESM::Pathgrid>;
        break;
    }
    case ESM::REC_PROB:
    {
        record = new EsmTool::Record<ESM::Probe>;
        break;
    }
    case ESM::REC_RACE:
    {
        record = new EsmTool::Record<ESM::Race>;
        break;
    }
    case ESM::REC_REGN:
    {
        record = new EsmTool::Record<ESM::Region>;
        break;
    }
    case ESM::REC_REPA:
    {
        record = new EsmTool::Record<ESM::Repair>;
        break;
    }
    case ESM::REC_SCPT:
    {
        record = new EsmTool::Record<ESM::Script>;
        break;
    }
    case ESM::REC_SKIL:
    {
        record = new EsmTool::Record<ESM::Skill>;
        break;
    }
    case ESM::REC_SNDG:
    {
        record = new EsmTool::Record<ESM::SoundGenerator>;
        break;
    }
    case ESM::REC_SOUN:
    {
        record = new EsmTool::Record<ESM::Sound>;
        break;
    }
    case ESM::REC_SPEL:
    {
        record = new EsmTool::Record<ESM::Spell>;
        break;
    }
    case ESM::REC_STAT:
    {
        record = new EsmTool::Record<ESM::Static>;
        break;
    }
    case ESM::REC_WEAP:
    {
        record = new EsmTool::Record<ESM::Weapon>;
        break;
    }
    case ESM::REC_SSCR:
    {
        record = new EsmTool::Record<ESM::StartScript>;
        break;
    }
    default:
        record = 0;
    }
    if (record) {
        record->mType = type;
    }
    return record;
}

template<>
void Record<ESM::Activator>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Potion>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  AutoCalc: " << mData.mData.mAutoCalc << std::endl;
    printEffectList(mData.mEffects);
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Armor>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    if (!mData.mEnchant.empty())
        std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
    std::cout << "  Type: " << armorTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Health: " << mData.mData.mHealth << std::endl;
    std::cout << "  Armor: " << mData.mData.mArmor << std::endl;
    std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
    for (const ESM::PartReference &part : mData.mParts.mParts)
    {
        std::cout << "  Body Part: " << bodyPartLabel(part.mPart)
                  << " (" << (int)(part.mPart) << ")" << std::endl;
        std::cout << "    Male Name: " << part.mMale << std::endl;
        if (!part.mFemale.empty())
            std::cout << "    Female Name: " << part.mFemale << std::endl;
    }

    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Apparatus>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Type: " << apparatusTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::BodyPart>::print()
{
    std::cout << "  Race: " << mData.mRace << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Type: " << meshTypeLabel(mData.mData.mType)
              << " (" << (int)mData.mData.mType << ")" << std::endl;
    std::cout << "  Flags: " << bodyPartFlags(mData.mData.mFlags) << std::endl;
    std::cout << "  Part: " << meshPartLabel(mData.mData.mPart)
              << " (" << (int)mData.mData.mPart << ")" << std::endl;
    std::cout << "  Vampire: " << (int)mData.mData.mVampire << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Book>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    if (!mData.mEnchant.empty())
        std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  IsScroll: " << mData.mData.mIsScroll << std::endl;
    std::cout << "  SkillId: " << mData.mData.mSkillId << std::endl;
    std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
    if (mPrintPlain)
    {
        std::cout << "  Text:" << std::endl;
        std::cout << "START--------------------------------------" << std::endl;
        std::cout << mData.mText << std::endl;
        std::cout << "END----------------------------------------" << std::endl;
    }
    else
    {
        std::cout << "  Text: [skipped]" << std::endl;
    }
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::BirthSign>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Texture: " << mData.mTexture << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
    for (const std::string &power : mData.mPowers.mList)
        std::cout << "  Power: " << power << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Cell>::print()
{
    // None of the cells have names...
    if (!mData.mName.empty())
        std::cout << "  Name: " << mData.mName << std::endl;
    if (!mData.mRegion.empty())
        std::cout << "  Region: " << mData.mRegion << std::endl;
    std::cout << "  Flags: " << cellFlags(mData.mData.mFlags) << std::endl;

    std::cout << "  Coordinates: " << " (" << mData.getGridX() << ","
              << mData.getGridY() << ")" << std::endl;

    if (mData.mData.mFlags & ESM::Cell::Interior &&
        !(mData.mData.mFlags & ESM::Cell::QuasiEx))
    {
        if (mData.hasAmbient())
        {
            // TODO: see if we can change the integer representation to something more sensible
            std::cout << "  Ambient Light Color: " << mData.mAmbi.mAmbient << std::endl;
            std::cout << "  Sunlight Color: " << mData.mAmbi.mSunlight << std::endl;
            std::cout << "  Fog Color: " << mData.mAmbi.mFog << std::endl;
            std::cout << "  Fog Density: " << mData.mAmbi.mFogDensity << std::endl;
        }
        else
        {
            std::cout << "  No Ambient Information" << std::endl;
        }
        std::cout << "  Water Level: " << mData.mWater << std::endl;
    }
    else
        std::cout << "  Map Color: " << Misc::StringUtils::format("0x%08X", mData.mMapColor) << std::endl;
    std::cout << "  Water Level Int: " << mData.mWaterInt << std::endl;
    std::cout << "  RefId counter: " << mData.mRefNumCounter << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;

}

template<>
void Record<ESM::Class>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
    std::cout << "  Playable: " << mData.mData.mIsPlayable << std::endl;
    std::cout << "  AutoCalc: " << mData.mData.mCalc << std::endl;
    std::cout << "  Attribute1: " << attributeLabel(mData.mData.mAttribute[0])
              << " (" << mData.mData.mAttribute[0] << ")" << std::endl;
    std::cout << "  Attribute2: " << attributeLabel(mData.mData.mAttribute[1])
              << " (" << mData.mData.mAttribute[1] << ")" << std::endl;
    std::cout << "  Specialization: " << specializationLabel(mData.mData.mSpecialization)
              << " (" << mData.mData.mSpecialization << ")" << std::endl;
    for (int i = 0; i != 5; i++)
        std::cout << "  Minor Skill: " << skillLabel(mData.mData.mSkills[i][0])
                  << " (" << mData.mData.mSkills[i][0] << ")" << std::endl;
    for (int i = 0; i != 5; i++)
        std::cout << "  Major Skill: " << skillLabel(mData.mData.mSkills[i][1])
                  << " (" << mData.mData.mSkills[i][1] << ")" << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Clothing>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    if (!mData.mEnchant.empty())
        std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
    std::cout << "  Type: " << clothingTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
    for (const ESM::PartReference &part : mData.mParts.mParts)
    {
        std::cout << "  Body Part: " << bodyPartLabel(part.mPart)
                  << " (" << (int)(part.mPart) << ")" << std::endl;
        std::cout << "    Male Name: " << part.mMale << std::endl;
        if (!part.mFemale.empty())
            std::cout << "    Female Name: " << part.mFemale << std::endl;
    }
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Container>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Flags: " << containerFlags(mData.mFlags) << std::endl;
    std::cout << "  Weight: " << mData.mWeight << std::endl;
    for (const ESM::ContItem &item : mData.mInventory.mList)
        std::cout << "  Inventory: Count: " << Misc::StringUtils::format("%4d", item.mCount)
                  << " Item: " << item.mItem << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Creature>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Flags: " << creatureFlags((int)mData.mFlags) << std::endl;
    std::cout << "  Blood Type: " << mData.mBloodType+1 << std::endl;
    std::cout << "  Original: " << mData.mOriginal << std::endl;
    std::cout << "  Scale: " << mData.mScale << std::endl;

    std::cout << "  Type: " << creatureTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Level: " << mData.mData.mLevel << std::endl;

    std::cout << "  Attributes:" << std::endl;
    std::cout << "    Strength: " << mData.mData.mStrength << std::endl;
    std::cout << "    Intelligence: " << mData.mData.mIntelligence << std::endl;
    std::cout << "    Willpower: " << mData.mData.mWillpower << std::endl;
    std::cout << "    Agility: " << mData.mData.mAgility << std::endl;
    std::cout << "    Speed: " << mData.mData.mSpeed << std::endl;
    std::cout << "    Endurance: " << mData.mData.mEndurance << std::endl;
    std::cout << "    Personality: " << mData.mData.mPersonality << std::endl;
    std::cout << "    Luck: " << mData.mData.mLuck << std::endl;

    std::cout << "  Health: " << mData.mData.mHealth << std::endl;
    std::cout << "  Magicka: " << mData.mData.mMana << std::endl;
    std::cout << "  Fatigue: " << mData.mData.mFatigue << std::endl;
    std::cout << "  Soul: " << mData.mData.mSoul << std::endl;
    std::cout << "  Combat: " << mData.mData.mCombat << std::endl;
    std::cout << "  Magic: " << mData.mData.mMagic << std::endl;
    std::cout << "  Stealth: " << mData.mData.mStealth << std::endl;
    std::cout << "  Attack1: " << mData.mData.mAttack[0]
              << "-" <<  mData.mData.mAttack[1] << std::endl;
    std::cout << "  Attack2: " << mData.mData.mAttack[2]
              << "-" <<  mData.mData.mAttack[3] << std::endl;
    std::cout << "  Attack3: " << mData.mData.mAttack[4]
              << "-" <<  mData.mData.mAttack[5] << std::endl;
    std::cout << "  Gold: " << mData.mData.mGold << std::endl;

    for (const ESM::ContItem &item : mData.mInventory.mList)
        std::cout << "  Inventory: Count: " << Misc::StringUtils::format("%4d", item.mCount)
                  << " Item: " << item.mItem << std::endl;

    for (const std::string &spell : mData.mSpells.mList)
        std::cout << "  Spell: " << spell << std::endl;

    printTransport(mData.getTransport());

    std::cout << "  Artificial Intelligence: " << std::endl;
    std::cout << "    AI Hello:" << (int)mData.mAiData.mHello << std::endl;
    std::cout << "    AI Fight:" << (int)mData.mAiData.mFight << std::endl;
    std::cout << "    AI Flee:" << (int)mData.mAiData.mFlee << std::endl;
    std::cout << "    AI Alarm:" << (int)mData.mAiData.mAlarm << std::endl;
    std::cout << "    AI U1:" << (int)mData.mAiData.mU1 << std::endl;
    std::cout << "    AI U2:" << (int)mData.mAiData.mU2 << std::endl;
    std::cout << "    AI U3:" << (int)mData.mAiData.mU3 << std::endl;
    std::cout << "    AI Services:" << Misc::StringUtils::format("0x%08X", mData.mAiData.mServices) << std::endl;

    for (const ESM::AIPackage &package : mData.mAiPackage.mList)
        printAIPackage(package);
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Dialogue>::print()
{
    std::cout << "  Type: " << dialogTypeLabel(mData.mType)
              << " (" << (int)mData.mType << ")" << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
    // Sadly, there are no DialInfos, because the loader dumps as it
    // loads, rather than loading and then dumping. :-( Anyone mind if
    // I change this?
    for (const ESM::DialInfo &info : mData.mInfo)
        std::cout << "INFO!" << info.mId << std::endl;
}

template<>
void Record<ESM::Door>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  OpenSound: " << mData.mOpenSound << std::endl;
    std::cout << "  CloseSound: " << mData.mCloseSound << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Enchantment>::print()
{
    std::cout << "  Type: " << enchantTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Cost: " << mData.mData.mCost << std::endl;
    std::cout << "  Charge: " << mData.mData.mCharge << std::endl;
    std::cout << "  AutoCalc: " << mData.mData.mAutocalc << std::endl;
    printEffectList(mData.mEffects);
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Faction>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Hidden: " << mData.mData.mIsHidden << std::endl;
    std::cout << "  Attribute1: " << attributeLabel(mData.mData.mAttribute[0])
              << " (" << mData.mData.mAttribute[0] << ")" << std::endl;
    std::cout << "  Attribute2: " << attributeLabel(mData.mData.mAttribute[1])
              << " (" << mData.mData.mAttribute[1] << ")" << std::endl;
    for (int i = 0; i < 7; i++)
        if (mData.mData.mSkills[i] != -1)
            std::cout << "  Skill: " << skillLabel(mData.mData.mSkills[i])
                      << " (" << mData.mData.mSkills[i] << ")" << std::endl;
    for (int i = 0; i != 10; i++)
        if (!mData.mRanks[i].empty())
        {
            std::cout << "  Rank: " << mData.mRanks[i] << std::endl;
            std::cout << "    Attribute1 Requirement: "
                      << mData.mData.mRankData[i].mAttribute1 << std::endl;
            std::cout << "    Attribute2 Requirement: "
                      << mData.mData.mRankData[i].mAttribute2 << std::endl;
            std::cout << "    One Skill at Level: "
                      << mData.mData.mRankData[i].mSkill1 << std::endl;
            std::cout << "    Two Skills at Level: "
                      << mData.mData.mRankData[i].mSkill2 << std::endl;
            std::cout << "    Faction Reaction: "
                      << mData.mData.mRankData[i].mFactReaction << std::endl;
        }
    for (const auto &reaction : mData.mReactions)
        std::cout << "  Reaction: " << reaction.second << " = " << reaction.first << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Global>::print()
{
    std::cout << "  " << mData.mValue << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::GameSetting>::print()
{
    std::cout << "  " << mData.mValue << std::endl;
}

template<>
void Record<ESM::DialInfo>::print()
{
    std::cout << "  Id: " << mData.mId << std::endl;
    if (!mData.mPrev.empty())
        std::cout << "  Previous ID: " << mData.mPrev << std::endl;
    if (!mData.mNext.empty())
        std::cout << "  Next ID: " << mData.mNext << std::endl;
    std::cout << "  Text: " << mData.mResponse << std::endl;
    if (!mData.mActor.empty())
        std::cout << "  Actor: " << mData.mActor << std::endl;
    if (!mData.mRace.empty())
        std::cout << "  Race: " << mData.mRace << std::endl;
    if (!mData.mClass.empty())
        std::cout << "  Class: " << mData.mClass << std::endl;
    std::cout << "  Factionless: " << mData.mFactionLess << std::endl;
    if (!mData.mFaction.empty())
        std::cout << "  NPC Faction: " << mData.mFaction << std::endl;
    if (mData.mData.mRank != -1)
        std::cout << "  NPC Rank: " << (int)mData.mData.mRank << std::endl;
    if (!mData.mPcFaction.empty())
        std::cout << "  PC Faction: " << mData.mPcFaction << std::endl;
    // CHANGE? non-standard capitalization mPCrank -> mPCRank (mPcRank?)
    if (mData.mData.mPCrank != -1)
        std::cout << "  PC Rank: " << (int)mData.mData.mPCrank << std::endl;
    if (!mData.mCell.empty())
        std::cout << "  Cell: " << mData.mCell << std::endl;
    if (mData.mData.mDisposition > 0)
        std::cout << "  Disposition/Journal index: " << mData.mData.mDisposition << std::endl;
    if (mData.mData.mGender != ESM::DialInfo::NA)
        std::cout << "  Gender: " << mData.mData.mGender << std::endl;
    if (!mData.mSound.empty())
        std::cout << "  Sound File: " << mData.mSound << std::endl;


    std::cout << "  Quest Status: " << questStatusLabel(mData.mQuestStatus)
              << " (" << mData.mQuestStatus << ")" << std::endl;
    std::cout << "  Unknown1: " << mData.mData.mUnknown1 << std::endl;
    std::cout << "  Unknown2: " << (int)mData.mData.mUnknown2 << std::endl;

    for (const ESM::DialInfo::SelectStruct &rule : mData.mSelects)
        std::cout << "  Select Rule: " << ruleString(rule) << std::endl;

    if (!mData.mResultScript.empty())
    {
        if (mPrintPlain)
        {
            std::cout << "  Result Script:" << std::endl;
            std::cout << "START--------------------------------------" << std::endl;
            std::cout << mData.mResultScript << std::endl;
            std::cout << "END----------------------------------------" << std::endl;
        }
        else
        {
            std::cout << "  Result Script: [skipped]" << std::endl;
        }
    }
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Ingredient>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    for (int i = 0; i !=4; i++)
    {
        // A value of -1 means no effect
        if (mData.mData.mEffectID[i] == -1) continue;
        std::cout << "  Effect: " << magicEffectLabel(mData.mData.mEffectID[i])
                  << " (" << mData.mData.mEffectID[i] << ")" << std::endl;
        std::cout << "  Skill: " << skillLabel(mData.mData.mSkills[i])
                  << " (" << mData.mData.mSkills[i] << ")" << std::endl;
        std::cout << "  Attribute: " << attributeLabel(mData.mData.mAttributes[i])
                  << " (" << mData.mData.mAttributes[i] << ")" << std::endl;
    }
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Land>::print()
{
    std::cout << "  Coordinates: (" << mData.mX << "," << mData.mY << ")" << std::endl;
    std::cout << "  Flags: " << landFlags(mData.mFlags) << std::endl;
    std::cout << "  DataTypes: " << mData.mDataTypes << std::endl;

    if (const ESM::Land::LandData *data = mData.getLandData (mData.mDataTypes))
    {
        std::cout << "  Height Offset: " << data->mHeightOffset << std::endl;
        // Lots of missing members.
        std::cout << "  Unknown1: " << data->mUnk1 << std::endl;
        std::cout << "  Unknown2: " << data->mUnk2 << std::endl;
    }
    mData.unloadData();
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::CreatureLevList>::print()
{
    std::cout << "  Chance for None: " << (int)mData.mChanceNone << std::endl;
    std::cout << "  Flags: " << creatureListFlags(mData.mFlags) << std::endl;
    std::cout << "  Number of items: " << mData.mList.size() << std::endl;
    for (const ESM::LevelledListBase::LevelItem &item : mData.mList)
        std::cout << "  Creature: Level: " << item.mLevel
                  << " Creature: " << item.mId << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::ItemLevList>::print()
{
    std::cout << "  Chance for None: " << (int)mData.mChanceNone << std::endl;
    std::cout << "  Flags: " << itemListFlags(mData.mFlags) << std::endl;
    std::cout << "  Number of items: " << mData.mList.size() << std::endl;
    for (const ESM::LevelledListBase::LevelItem &item : mData.mList)
        std::cout << "  Inventory: Level: " << item.mLevel
                  << " Item: " << item.mId << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Light>::print()
{
    if (!mData.mName.empty())
        std::cout << "  Name: " << mData.mName << std::endl;
    if (!mData.mModel.empty())
        std::cout << "  Model: " << mData.mModel << std::endl;
    if (!mData.mIcon.empty())
        std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Flags: " << lightFlags(mData.mData.mFlags) << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Sound: " << mData.mSound << std::endl;
    std::cout << "  Duration: " << mData.mData.mTime << std::endl;
    std::cout << "  Radius: " << mData.mData.mRadius << std::endl;
    std::cout << "  Color: " << mData.mData.mColor << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Lockpick>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
    std::cout << "  Uses: " << mData.mData.mUses << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Probe>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
    std::cout << "  Uses: " << mData.mData.mUses << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Repair>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
    std::cout << "  Uses: " << mData.mData.mUses << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::LandTexture>::print()
{
    std::cout << "  Id: " << mData.mId << std::endl;
    std::cout << "  Index: " << mData.mIndex << std::endl;
    std::cout << "  Texture: " << mData.mTexture << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::MagicEffect>::print()
{
    std::cout << "  Index: " << magicEffectLabel(mData.mIndex)
              << " (" << mData.mIndex << ")" << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    std::cout << "  Flags: " << magicEffectFlags(mData.mData.mFlags) << std::endl;
    std::cout << "  Particle Texture: " << mData.mParticle << std::endl;
    if (!mData.mCasting.empty())
        std::cout << "  Casting Static: " << mData.mCasting << std::endl;
    if (!mData.mCastSound.empty())
        std::cout << "  Casting Sound: " << mData.mCastSound << std::endl;
    if (!mData.mBolt.empty())
        std::cout << "  Bolt Static: " << mData.mBolt << std::endl;
    if (!mData.mBoltSound.empty())
        std::cout << "  Bolt Sound: " << mData.mBoltSound << std::endl;
    if (!mData.mHit.empty())
        std::cout << "  Hit Static: " << mData.mHit << std::endl;
    if (!mData.mHitSound.empty())
        std::cout << "  Hit Sound: " << mData.mHitSound << std::endl;
    if (!mData.mArea.empty())
        std::cout << "  Area Static: " << mData.mArea << std::endl;
    if (!mData.mAreaSound.empty())
        std::cout << "  Area Sound: " << mData.mAreaSound << std::endl;
    std::cout << "  School: " << schoolLabel(mData.mData.mSchool)
              << " (" << mData.mData.mSchool << ")" << std::endl;
    std::cout << "  Base Cost: " << mData.mData.mBaseCost << std::endl;
    std::cout << "  Unknown 1: " << mData.mData.mUnknown1 << std::endl;
    std::cout << "  Speed: " << mData.mData.mSpeed << std::endl;
    std::cout << "  Unknown 2: " << mData.mData.mUnknown2 << std::endl;
    std::cout << "  RGB Color: " << "("
              << mData.mData.mRed << ","
              << mData.mData.mGreen << ","
              << mData.mData.mBlue << ")" << std::endl;
}

template<>
void Record<ESM::Miscellaneous>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Is Key: " << mData.mData.mIsKey << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::NPC>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Animation: " << mData.mModel << std::endl;
    std::cout << "  Hair Model: " << mData.mHair << std::endl;
    std::cout << "  Head Model: " << mData.mHead << std::endl;
    std::cout << "  Race: " << mData.mRace << std::endl;
    std::cout << "  Class: " << mData.mClass << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    if (!mData.mFaction.empty())
        std::cout << "  Faction: " << mData.mFaction << std::endl;
    std::cout << "  Flags: " << npcFlags((int)mData.mFlags) << std::endl;
    if (mData.mBloodType != 0)
        std::cout << "  Blood Type: " << mData.mBloodType+1 << std::endl;

    if (mData.mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
    {
        std::cout << "  Level: " << mData.mNpdt.mLevel << std::endl;
        std::cout << "  Reputation: " << (int)mData.mNpdt.mReputation << std::endl;
        std::cout << "  Disposition: " << (int)mData.mNpdt.mDisposition << std::endl;
        std::cout << "  Rank: " << (int)mData.mNpdt.mRank << std::endl;
        std::cout << "  Gold: " << mData.mNpdt.mGold << std::endl;
    }
    else {
        std::cout << "  Level: " << mData.mNpdt.mLevel << std::endl;
        std::cout << "  Reputation: " << (int)mData.mNpdt.mReputation << std::endl;
        std::cout << "  Disposition: " << (int)mData.mNpdt.mDisposition << std::endl;
        std::cout << "  Rank: " << (int)mData.mNpdt.mRank << std::endl;

        std::cout << "  Attributes:" << std::endl;
        std::cout << "    Strength: " << (int)mData.mNpdt.mStrength << std::endl;
        std::cout << "    Intelligence: " << (int)mData.mNpdt.mIntelligence << std::endl;
        std::cout << "    Willpower: " << (int)mData.mNpdt.mWillpower << std::endl;
        std::cout << "    Agility: " << (int)mData.mNpdt.mAgility << std::endl;
        std::cout << "    Speed: " << (int)mData.mNpdt.mSpeed << std::endl;
        std::cout << "    Endurance: " << (int)mData.mNpdt.mEndurance << std::endl;
        std::cout << "    Personality: " << (int)mData.mNpdt.mPersonality << std::endl;
        std::cout << "    Luck: " << (int)mData.mNpdt.mLuck << std::endl;

        std::cout << "  Skills:" << std::endl;
        for (int i = 0; i != ESM::Skill::Length; i++)
            std::cout << "    " << skillLabel(i) << ": "
                      << (int)(mData.mNpdt.mSkills[i]) << std::endl;

        std::cout << "  Health: " << mData.mNpdt.mHealth << std::endl;
        std::cout << "  Magicka: " << mData.mNpdt.mMana << std::endl;
        std::cout << "  Fatigue: " << mData.mNpdt.mFatigue << std::endl;
        std::cout << "  Gold: " << mData.mNpdt.mGold << std::endl;
    }

    for (const ESM::ContItem &item : mData.mInventory.mList)
        std::cout << "  Inventory: Count: " << Misc::StringUtils::format("%4d", item.mCount)
                  << " Item: " << item.mItem << std::endl;

    for (const std::string &spell : mData.mSpells.mList)
        std::cout << "  Spell: " << spell << std::endl;

    printTransport(mData.getTransport());

    std::cout << "  Artificial Intelligence: " << std::endl;
    std::cout << "    AI Hello:" << (int)mData.mAiData.mHello << std::endl;
    std::cout << "    AI Fight:" << (int)mData.mAiData.mFight << std::endl;
    std::cout << "    AI Flee:" << (int)mData.mAiData.mFlee << std::endl;
    std::cout << "    AI Alarm:" << (int)mData.mAiData.mAlarm << std::endl;
    std::cout << "    AI U1:" << (int)mData.mAiData.mU1 << std::endl;
    std::cout << "    AI U2:" << (int)mData.mAiData.mU2 << std::endl;
    std::cout << "    AI U3:" << (int)mData.mAiData.mU3 << std::endl;
    std::cout << "    AI Services:" << Misc::StringUtils::format("0x%08X", mData.mAiData.mServices) << std::endl;

    for (const ESM::AIPackage &package : mData.mAiPackage.mList)
        printAIPackage(package);

    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Pathgrid>::print()
{
    std::cout << "  Cell: " << mData.mCell << std::endl;
    std::cout << "  Coordinates: (" << mData.mData.mX << "," << mData.mData.mY << ")" << std::endl;
    std::cout << "  Unknown S1: " << mData.mData.mS1 << std::endl;
    if ((unsigned int)mData.mData.mS2 != mData.mPoints.size())
        std::cout << "  Reported Point Count: " << mData.mData.mS2 << std::endl;
    std::cout << "  Point Count: " << mData.mPoints.size() << std::endl;
    std::cout << "  Edge Count: " << mData.mEdges.size() << std::endl;

    int i = 0;
    for (const ESM::Pathgrid::Point &point : mData.mPoints)
    {
        std::cout << "  Point[" << i << "]:" << std::endl;
        std::cout << "    Coordinates: (" << point.mX << ","
             << point.mY << "," << point.mZ << ")" << std::endl;
        std::cout << "    Auto-Generated: " << (int)point.mAutogenerated << std::endl;
        std::cout << "    Connections: " << (int)point.mConnectionNum << std::endl;
        std::cout << "    Unknown: " << point.mUnknown << std::endl;
        i++;
    }

    i = 0;
    for (const ESM::Pathgrid::Edge &edge : mData.mEdges)
    {
        std::cout << "  Edge[" << i << "]: " << edge.mV0 << " -> " << edge.mV1 << std::endl;
        if (edge.mV0 >= mData.mData.mS2 || edge.mV1 >= mData.mData.mS2)
            std::cout << "  BAD POINT IN EDGE!" << std::endl;
        i++;
    }

    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Race>::print()
{
    static const char *sAttributeNames[8] =
    {
        "Strength", "Intelligence", "Willpower", "Agility",
        "Speed", "Endurance", "Personality", "Luck"
    };

    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
    std::cout << "  Flags: " << raceFlags(mData.mData.mFlags) << std::endl;

    for (int i=0; i<2; ++i)
    {
        bool male = i==0;

        std::cout << (male ? "  Male:" : "  Female:") << std::endl;

        for (int j=0; j<8; ++j)
            std::cout << "    " << sAttributeNames[j] << ": "
                << mData.mData.mAttributeValues[j].getValue (male) << std::endl;

        std::cout << "    Height: " << mData.mData.mHeight.getValue (male) << std::endl;
        std::cout << "    Weight: " << mData.mData.mWeight.getValue (male) << std::endl;
    }

    for (int i = 0; i != 7; i++)
        // Not all races have 7 skills.
        if (mData.mData.mBonus[i].mSkill != -1)
            std::cout << "  Skill: "
                      << skillLabel(mData.mData.mBonus[i].mSkill)
                      << " (" << mData.mData.mBonus[i].mSkill << ") = "
                      << mData.mData.mBonus[i].mBonus << std::endl;

    for (const std::string &power : mData.mPowers.mList)
        std::cout << "  Power: " << power << std::endl;

    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Region>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;

    std::cout << "  Weather:" << std::endl;
    std::cout << "    Clear: " << (int)mData.mData.mClear << std::endl;
    std::cout << "    Cloudy: " << (int)mData.mData.mCloudy << std::endl;
    std::cout << "    Foggy: " << (int)mData.mData.mFoggy << std::endl;
    std::cout << "    Overcast: " << (int)mData.mData.mOvercast << std::endl;
    std::cout << "    Rain: " << (int)mData.mData.mOvercast << std::endl;
    std::cout << "    Thunder: " << (int)mData.mData.mThunder << std::endl;
    std::cout << "    Ash: " << (int)mData.mData.mAsh << std::endl;
    std::cout << "    Blight: " << (int)mData.mData.mBlight << std::endl;
    std::cout << "    UnknownA: " << (int)mData.mData.mA << std::endl;
    std::cout << "    UnknownB: " << (int)mData.mData.mB << std::endl;
    std::cout << "  Map Color: " << mData.mMapColor << std::endl;
    if (!mData.mSleepList.empty())
        std::cout << "  Sleep List: " << mData.mSleepList << std::endl;
    for (const ESM::Region::SoundRef &soundref : mData.mSoundList)
        std::cout << "  Sound: " << (int)soundref.mChance << " = " << soundref.mSound << std::endl;
}

template<>
void Record<ESM::Script>::print()
{
    std::cout << "  Name: " << mData.mId << std::endl;

    std::cout << "  Num Shorts: " << mData.mData.mNumShorts << std::endl;
    std::cout << "  Num Longs: " << mData.mData.mNumLongs << std::endl;
    std::cout << "  Num Floats: " << mData.mData.mNumFloats << std::endl;
    std::cout << "  Script Data Size: " << mData.mData.mScriptDataSize << std::endl;
    std::cout << "  Table Size: " << mData.mData.mStringTableSize << std::endl;

    for (const std::string &variable : mData.mVarNames)
        std::cout << "  Variable: " << variable << std::endl;

    std::cout << "  ByteCode: ";
    for (const unsigned char &byte : mData.mScriptData)
        std::cout << Misc::StringUtils::format("%02X", (int)(byte));
    std::cout << std::endl;

    if (mPrintPlain)
    {
        std::cout << "  Script:" << std::endl;
        std::cout << "START--------------------------------------" << std::endl;
        std::cout << mData.mScriptText << std::endl;
        std::cout << "END----------------------------------------" << std::endl;
    }
    else
    {
        std::cout << "  Script: [skipped]" << std::endl;
    }

    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Skill>::print()
{
    std::cout << "  ID: " << skillLabel(mData.mIndex)
              << " (" << mData.mIndex << ")" << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
    std::cout << "  Governing Attribute: " << attributeLabel(mData.mData.mAttribute)
              << " (" << mData.mData.mAttribute << ")" << std::endl;
    std::cout << "  Specialization: " << specializationLabel(mData.mData.mSpecialization)
              << " (" << mData.mData.mSpecialization << ")" << std::endl;
    for (int i = 0; i != 4; i++)
        std::cout << "  UseValue[" << i << "]:" << mData.mData.mUseValue[i] << std::endl;
}

template<>
void Record<ESM::SoundGenerator>::print()
{
    if (!mData.mCreature.empty())
        std::cout << "  Creature: " << mData.mCreature << std::endl;
    std::cout << "  Sound: " << mData.mSound << std::endl;
    std::cout << "  Type: " << soundTypeLabel(mData.mType)
              << " (" << mData.mType << ")" << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Sound>::print()
{
    std::cout << "  Sound: " << mData.mSound << std::endl;
    std::cout << "  Volume: " << (int)mData.mData.mVolume << std::endl;
    if (mData.mData.mMinRange != 0 && mData.mData.mMaxRange != 0)
        std::cout << "  Range: " << (int)mData.mData.mMinRange << " - "
                  << (int)mData.mData.mMaxRange << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Spell>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Type: " << spellTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Flags: " << spellFlags(mData.mData.mFlags) << std::endl;
    std::cout << "  Cost: " << mData.mData.mCost << std::endl;
    printEffectList(mData.mEffects);
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::StartScript>::print()
{
    std::cout << "  Start Script: " << mData.mId << std::endl;
    std::cout << "  Start Data: " << mData.mData << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<>
void Record<ESM::Static>::print()
{
    std::cout << "  Model: " << mData.mModel << std::endl;
}

template<>
void Record<ESM::Weapon>::print()
{
    // No names on VFX bolts
    if (!mData.mName.empty())
        std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Model: " << mData.mModel << std::endl;
    // No icons on VFX bolts or magic bolts
    if (!mData.mIcon.empty())
        std::cout << "  Icon: " << mData.mIcon << std::endl;
    if (!mData.mScript.empty())
        std::cout << "  Script: " << mData.mScript << std::endl;
    if (!mData.mEnchant.empty())
        std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
    std::cout << "  Type: " << weaponTypeLabel(mData.mData.mType)
              << " (" << mData.mData.mType << ")" << std::endl;
    std::cout << "  Flags: " << weaponFlags(mData.mData.mFlags) << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
    std::cout << "  Health: " << mData.mData.mHealth << std::endl;
    std::cout << "  Speed: " << mData.mData.mSpeed << std::endl;
    std::cout << "  Reach: " << mData.mData.mReach << std::endl;
    std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
    if (mData.mData.mChop[0] != 0 && mData.mData.mChop[1] != 0)
        std::cout << "  Chop: " << (int)mData.mData.mChop[0] << "-"
                  << (int)mData.mData.mChop[1] << std::endl;
    if (mData.mData.mSlash[0] != 0 && mData.mData.mSlash[1] != 0)
        std::cout << "  Slash: " << (int)mData.mData.mSlash[0] << "-"
                  << (int)mData.mData.mSlash[1] << std::endl;
    if (mData.mData.mThrust[0] != 0 && mData.mData.mThrust[1] != 0)
        std::cout << "  Thrust: " << (int)mData.mData.mThrust[0] << "-"
                  << (int)mData.mData.mThrust[1] << std::endl;
    std::cout << "  Deleted: " << mIsDeleted << std::endl;
}

template<> 
std::string Record<ESM::Cell>::getId() const
{
    return mData.mName;
}

template<> 
std::string Record<ESM::Land>::getId() const
{
    return std::string(); // No ID for Land record
}

template<> 
std::string Record<ESM::MagicEffect>::getId() const
{
    return std::string(); // No ID for MagicEffect record
}

template<> 
std::string Record<ESM::Pathgrid>::getId() const
{
    return std::string(); // No ID for Pathgrid record
}

template<> 
std::string Record<ESM::Skill>::getId() const
{
    return std::string(); // No ID for Skill record
}

} // end namespace
