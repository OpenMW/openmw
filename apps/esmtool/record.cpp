#include "record.hpp"

#include <iostream>

namespace EsmTool {

RecordBase *
RecordBase::create(int type)
{
    RecordBase *record = 0;

    switch (type) {
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
        record = new EsmTool::Record<ESM::Tool>;
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
    std::cout << "  Mesh: " << mData.mModel << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
}

template<>
void Record<ESM::Potion>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::Armor>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Mesh: " << mData.mModel << std::endl;
    std::cout << "  Icon: " << mData.mIcon << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
    std::cout << "  Type: " << mData.mData.mType << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
}

template<>
void Record<ESM::Apparatus>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::BodyPart>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Mesh: " << mData.mModel << std::endl;
}

template<>
void Record<ESM::Book>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Mesh: " << mData.mModel << std::endl;
}

template<>
void Record<ESM::BirthSign>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Texture: " << mData.mTexture << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
}

template<>
void Record<ESM::Cell>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Region: " << mData.mRegion << std::endl;
}

template<>
void Record<ESM::Class>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Description: " << mData.mDescription << std::endl;
}

template<>
void Record<ESM::Clothing>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::Container>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::Creature>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::Dialogue>::print()
{
    // nothing to print
}

template<>
void Record<ESM::Door>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Mesh: " << mData.mModel << std::endl;
    std::cout << "  Script: " << mData.mScript << std::endl;
    std::cout << "  OpenSound: " << mData.mOpenSound << std::endl;
    std::cout << "  CloseSound: " << mData.mCloseSound << std::endl;
}

template<>
void Record<ESM::Enchantment>::print()
{
    // nothing to print
}

template<>
void Record<ESM::Faction>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Attr1: " << mData.mData.mAttribute1 << std::endl;
    std::cout << "  Attr2: " << mData.mData.mAttribute2 << std::endl;
    std::cout << "  Hidden: " << mData.mData.mIsHidden << std::endl;
}

template<>
void Record<ESM::Global>::print()
{
    // nothing to print
}

template<>
void Record<ESM::GameSetting>::print()
{
    std::cout << "  Value: ";
    switch (mData.mType) {
    case ESM::VT_String:
        std::cout << "'" << mData.mStr << "' (std::string)";
        break;

    case ESM::VT_Float:
        std::cout << mData.mF << " (float)";
        break;

    case ESM::VT_Int:
        std::cout << mData.mI << " (int)";
        break;

    default:
        std::cout << "unknown type";
    }
    std::cout << "\n  Dirty: " << mData.mDirty << std::endl;
}

template<>
void Record<ESM::DialInfo>::print()
{
    std::cout << "  Id: " << mData.mId << std::endl;
    std::cout << "  Text: " << mData.mResponse << std::endl;
}

template<>
void Record<ESM::Ingredient>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
}

template<>
void Record<ESM::Land>::print()
{
    std::cout << "  Coords: [" << mData.mX << "," << mData.mY << "]" << std::endl;
}

template<>
void Record<ESM::CreatureLevList>::print()
{
    std::cout << "  Number of items: " << mData.mList.size() << std::endl;
}

template<>
void Record<ESM::ItemLevList>::print()
{
    std::cout << "  Number of items: " << mData.mList.size() << std::endl;
}

template<>
void Record<ESM::Light>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
}

template<>
void Record<ESM::Tool>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
}

template<>
void Record<ESM::Probe>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
}

template<>
void Record<ESM::Repair>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
}

template<>
void Record<ESM::LandTexture>::print()
{
    std::cout << "  Id: " << mData.mId << std::endl;
    std::cout << "  Texture: " << mData.mTexture << std::endl;
}

template<>
void Record<ESM::MagicEffect>::print()
{
    std::cout << "  Index: " << mData.mIndex << std::endl;

    const char *text = "Positive";
    if (mData.mData.mFlags & ESM::MagicEffect::Negative) {
        text = "Negative";
    }
    std::cout << "  " << text << std::endl;
}

template<>
void Record<ESM::Miscellaneous>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
}

template<>
void Record<ESM::NPC>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Race: " << mData.mRace << std::endl;
}

template<>
void Record<ESM::Pathgrid>::print()
{
    std::cout << "  Cell: " << mData.mCell << std::endl;
    std::cout << "  Point count: " << mData.mPoints.size() << std::endl;
    std::cout << "  Edge count: " << mData.mEdges.size() << std::endl;
}

template<>
void Record<ESM::Race>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Length: " << mData.mData.mHeight.mMale << "m " << mData.mData.mHeight.mFemale << "f" << std::endl;
}

template<>
void Record<ESM::Region>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::Script>::print()
{
    std::cout << "  Name: " << mData.mData.mName.toString() << std::endl;
}

template<>
void Record<ESM::Skill>::print()
{
    std::cout << "  ID: " << mData.mIndex << std::endl;

    const char *spec = 0;
    int specId = mData.mData.mSpecialization;
    if (specId == 0) {
        spec = "Combat";
    } else if (specId == 1) {
        spec = "Magic";
    } else {
        spec = "Stealth";
    }
    std::cout << "  Type: " << spec << std::endl;
}

template<>
void Record<ESM::SoundGenerator>::print()
{
    std::cout << "  Creature: " << mData.mCreature << std::endl;
    std::cout << "  Sound: " << mData.mSound << std::endl;
}

template<>
void Record<ESM::Sound>::print()
{
    std::cout << "  Sound: " << mData.mSound << std::endl;
    std::cout << "  Volume: " << mData.mData.mVolume << std::endl;
}

template<>
void Record<ESM::Spell>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
}

template<>
void Record<ESM::StartScript>::print()
{
    std::cout << "Start script: " << mData.mScript << std::endl;
}

template<>
void Record<ESM::Static>::print()
{
    std::cout << "  Model: " << mData.mModel << std::endl;
}

template<>
void Record<ESM::Weapon>::print()
{
    std::cout << "  Name: " << mData.mName << std::endl;
    std::cout << "  Chop: " << mData.mData.mChop[0] << "-" << mData.mData.mChop[1] << std::endl;
    std::cout << "  Slash: " << mData.mData.mSlash[0] << "-" << mData.mData.mSlash[1] << std::endl;
    std::cout << "  Thrust: " << mData.mData.mThrust[0] << "-" << mData.mData.mThrust[1] << std::endl;
    std::cout << "  Value: " << mData.mData.mValue << std::endl;
}

template<>
void Record<ESM::CellRef>::print()
{
    std::cout << "    Refnum: " << mData.mRefnum << std::endl;
    std::cout << "    ID: '" << mData.mRefID << "'\n";
    std::cout << "    Owner: '" << mData.mOwner << "'\n";
    std::cout << "    INTV: " << mData.mIntv << "   NAM9: " << mData.mIntv << std::endl;
}

} // end namespace
