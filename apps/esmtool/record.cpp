#include "record.hpp"
#include "labels.hpp"

#include <format>
#include <iostream>
#include <numeric>
#include <sstream>

#include <osg/Math>

#include <components/esm3/cellstate.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/misc/strings/conversion.hpp>

namespace
{

    void printAIPackage(const ESM::AIPackage& p)
    {
        std::cout << std::format("  AI Type: {} (0x{:08X})\n", aiTypeLabel(p.mType), std::uint32_t(p.mType));
        if (p.mType == ESM::AI_Wander)
        {
            std::cout << "    Distance: " << p.mWander.mDistance << std::endl;
            std::cout << "    Duration: " << p.mWander.mDuration << std::endl;
            std::cout << "    Time of Day: " << (int)p.mWander.mTimeOfDay << std::endl;
            if (p.mWander.mShouldRepeat != 1)
                std::cout << "    Should repeat: " << static_cast<bool>(p.mWander.mShouldRepeat != 0) << std::endl;

            std::cout << "    Idle: ";
            for (int i = 0; i != 8; i++)
                std::cout << (int)p.mWander.mIdle[i] << " ";
            std::cout << std::endl;
        }
        else if (p.mType == ESM::AI_Travel)
        {
            std::cout << "    Travel Coordinates: (" << p.mTravel.mX << "," << p.mTravel.mY << "," << p.mTravel.mZ
                      << ")" << std::endl;
            std::cout << "    Should repeat: " << static_cast<bool>(p.mTravel.mShouldRepeat != 0) << std::endl;
        }
        else if (p.mType == ESM::AI_Follow || p.mType == ESM::AI_Escort)
        {
            std::cout << "    Follow Coordinates: (" << p.mTarget.mX << "," << p.mTarget.mY << "," << p.mTarget.mZ
                      << ")" << std::endl;
            std::cout << "    Duration: " << p.mTarget.mDuration << std::endl;
            std::cout << "    Target ID: " << p.mTarget.mId.toString() << std::endl;
            std::cout << "    Should repeat: " << static_cast<bool>(p.mTarget.mShouldRepeat != 0) << std::endl;
        }
        else if (p.mType == ESM::AI_Activate)
        {
            std::cout << "    Name: " << p.mActivate.mName.toString() << std::endl;
            std::cout << "    Should repeat: " << static_cast<bool>(p.mActivate.mShouldRepeat != 0) << std::endl;
        }
        else
        {
            std::cout << std::format("    BadPackage: 0x{:08X}\n", std::uint32_t(p.mType));
        }

        if (!p.mCellName.empty())
            std::cout << "    Cell Name: " << p.mCellName << std::endl;
    }

    std::string ruleString(const ESM::DialogueCondition& ss)
    {
        std::string_view typeStr = "INVALID";
        std::string_view funcStr;

        switch (ss.mFunction)
        {
            case ESM::DialogueCondition::Function_Global:
                typeStr = "Global";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_Local:
                typeStr = "Local";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_Journal:
                typeStr = "Journal";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_Item:
                typeStr = "Item count";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_Dead:
                typeStr = "Dead";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_NotId:
                typeStr = "Not ID";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_NotFaction:
                typeStr = "Not Faction";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_NotClass:
                typeStr = "Not Class";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_NotRace:
                typeStr = "Not Race";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_NotCell:
                typeStr = "Not Cell";
                funcStr = ss.mVariable;
                break;
            case ESM::DialogueCondition::Function_NotLocal:
                typeStr = "Not Local";
                funcStr = ss.mVariable;
                break;
            default:
                typeStr = "Function";
                funcStr = ruleFunction(ss.mFunction);
                break;
        }

        std::string_view operStr = "??";
        switch (ss.mComparison)
        {
            case ESM::DialogueCondition::Comp_Eq:
                operStr = "==";
                break;
            case ESM::DialogueCondition::Comp_Ne:
                operStr = "!=";
                break;
            case ESM::DialogueCondition::Comp_Gt:
                operStr = "> ";
                break;
            case ESM::DialogueCondition::Comp_Ge:
                operStr = ">=";
                break;
            case ESM::DialogueCondition::Comp_Ls:
                operStr = "< ";
                break;
            case ESM::DialogueCondition::Comp_Le:
                operStr = "<=";
                break;
            default:
                break;
        }

        std::ostringstream stream;
        std::visit([&](auto value) { stream << value; }, ss.mValue);

        std::string result = std::format("{:<12} {:<32} {:2} {}", typeStr, funcStr, operStr, stream.str());
        return result;
    }

    void printEffectList(const ESM::EffectList& effects)
    {
        int i = 0;
        for (const ESM::IndexedENAMstruct& effect : effects.mList)
        {
            std::cout << "  Effect[" << i << "]: " << magicEffectLabel(ESM::MagicEffect::refIdToIndex(effect.mData.mEffectID)) << " ("
                      << effect.mData.mEffectID << ")" << std::endl;
            if (effect.mData.mSkill != -1)
                std::cout << "    Skill: " << skillLabel(effect.mData.mSkill) << " (" << (int)effect.mData.mSkill << ")"
                          << std::endl;
            if (effect.mData.mAttribute != -1)
                std::cout << "    Attribute: " << attributeLabel(effect.mData.mAttribute) << " ("
                          << (int)effect.mData.mAttribute << ")" << std::endl;
            std::cout << "    Range: " << rangeTypeLabel(effect.mData.mRange) << " (" << effect.mData.mRange << ")"
                      << std::endl;
            // Area is always zero if range type is "Self"
            if (effect.mData.mRange != ESM::RT_Self)
                std::cout << "    Area: " << effect.mData.mArea << std::endl;
            std::cout << "    Duration: " << effect.mData.mDuration << std::endl;
            std::cout << "    Magnitude: " << effect.mData.mMagnMin << "-" << effect.mData.mMagnMax << std::endl;
            i++;
        }
    }

    void printTransport(const std::vector<ESM::Transport::Dest>& transport)
    {
        for (const ESM::Transport::Dest& dest : transport)
        {
            std::cout << std::format("  Destination Position: ({:12.3f}, {:12.3f}, {:12.3f})\n", dest.mPos.pos[0],
                dest.mPos.pos[1], dest.mPos.pos[2]);
            std::cout << std::format("  Destination Rotation: ({:12.3f}, {:12.3f}, {:12.3f})\n",
                osg::RadiansToDegrees(dest.mPos.rot[0]), osg::RadiansToDegrees(dest.mPos.rot[1]),
                osg::RadiansToDegrees(dest.mPos.rot[2]));
            if (!dest.mCellName.empty())
                std::cout << "  Destination Cell: " << dest.mCellName << std::endl;
        }
    }
}

namespace EsmTool
{
    void CellState::load(ESM::ESMReader& reader, bool& deleted)
    {
        mCellState.mId = reader.getCellId();
        mCellState.load(reader);
        if (mCellState.mHasFogOfWar)
            mFogState.load(reader);
        deleted = false;
        reader.skipRecord();
    }

    std::unique_ptr<RecordBase> RecordBase::create(const ESM::NAME type)
    {
        std::unique_ptr<RecordBase> record;

        switch (type.toInt())
        {
            case ESM::REC_ACTI:
            {
                record = std::make_unique<EsmTool::Record<ESM::Activator>>();
                break;
            }
            case ESM::REC_ALCH:
            {
                record = std::make_unique<EsmTool::Record<ESM::Potion>>();
                break;
            }
            case ESM::REC_APPA:
            {
                record = std::make_unique<EsmTool::Record<ESM::Apparatus>>();
                break;
            }
            case ESM::REC_ARMO:
            {
                record = std::make_unique<EsmTool::Record<ESM::Armor>>();
                break;
            }
            case ESM::REC_BODY:
            {
                record = std::make_unique<EsmTool::Record<ESM::BodyPart>>();
                break;
            }
            case ESM::REC_BOOK:
            {
                record = std::make_unique<EsmTool::Record<ESM::Book>>();
                break;
            }
            case ESM::REC_BSGN:
            {
                record = std::make_unique<EsmTool::Record<ESM::BirthSign>>();
                break;
            }
            case ESM::REC_CELL:
            {
                record = std::make_unique<EsmTool::Record<ESM::Cell>>();
                break;
            }
            case ESM::REC_CLAS:
            {
                record = std::make_unique<EsmTool::Record<ESM::Class>>();
                break;
            }
            case ESM::REC_CLOT:
            {
                record = std::make_unique<EsmTool::Record<ESM::Clothing>>();
                break;
            }
            case ESM::REC_CONT:
            {
                record = std::make_unique<EsmTool::Record<ESM::Container>>();
                break;
            }
            case ESM::REC_CREA:
            {
                record = std::make_unique<EsmTool::Record<ESM::Creature>>();
                break;
            }
            case ESM::REC_DIAL:
            {
                record = std::make_unique<EsmTool::Record<ESM::Dialogue>>();
                break;
            }
            case ESM::REC_DOOR:
            {
                record = std::make_unique<EsmTool::Record<ESM::Door>>();
                break;
            }
            case ESM::REC_ENCH:
            {
                record = std::make_unique<EsmTool::Record<ESM::Enchantment>>();
                break;
            }
            case ESM::REC_FACT:
            {
                record = std::make_unique<EsmTool::Record<ESM::Faction>>();
                break;
            }
            case ESM::REC_GLOB:
            {
                record = std::make_unique<EsmTool::Record<ESM::Global>>();
                break;
            }
            case ESM::REC_GMST:
            {
                record = std::make_unique<EsmTool::Record<ESM::GameSetting>>();
                break;
            }
            case ESM::REC_INFO:
            {
                record = std::make_unique<EsmTool::Record<ESM::DialInfo>>();
                break;
            }
            case ESM::REC_INGR:
            {
                record = std::make_unique<EsmTool::Record<ESM::Ingredient>>();
                break;
            }
            case ESM::REC_LAND:
            {
                record = std::make_unique<EsmTool::Record<ESM::Land>>();
                break;
            }
            case ESM::REC_LEVI:
            {
                record = std::make_unique<EsmTool::Record<ESM::ItemLevList>>();
                break;
            }
            case ESM::REC_LEVC:
            {
                record = std::make_unique<EsmTool::Record<ESM::CreatureLevList>>();
                break;
            }
            case ESM::REC_LIGH:
            {
                record = std::make_unique<EsmTool::Record<ESM::Light>>();
                break;
            }
            case ESM::REC_LOCK:
            {
                record = std::make_unique<EsmTool::Record<ESM::Lockpick>>();
                break;
            }
            case ESM::REC_LTEX:
            {
                record = std::make_unique<EsmTool::Record<ESM::LandTexture>>();
                break;
            }
            case ESM::REC_MISC:
            {
                record = std::make_unique<EsmTool::Record<ESM::Miscellaneous>>();
                break;
            }
            case ESM::REC_MGEF:
            {
                record = std::make_unique<EsmTool::Record<ESM::MagicEffect>>();
                break;
            }
            case ESM::REC_NPC_:
            {
                record = std::make_unique<EsmTool::Record<ESM::NPC>>();
                break;
            }
            case ESM::REC_PGRD:
            {
                record = std::make_unique<EsmTool::Record<ESM::Pathgrid>>();
                break;
            }
            case ESM::REC_PROB:
            {
                record = std::make_unique<EsmTool::Record<ESM::Probe>>();
                break;
            }
            case ESM::REC_RACE:
            {
                record = std::make_unique<EsmTool::Record<ESM::Race>>();
                break;
            }
            case ESM::REC_REGN:
            {
                record = std::make_unique<EsmTool::Record<ESM::Region>>();
                break;
            }
            case ESM::REC_REPA:
            {
                record = std::make_unique<EsmTool::Record<ESM::Repair>>();
                break;
            }
            case ESM::REC_SCPT:
            {
                record = std::make_unique<EsmTool::Record<ESM::Script>>();
                break;
            }
            case ESM::REC_SKIL:
            {
                record = std::make_unique<EsmTool::Record<ESM::Skill>>();
                break;
            }
            case ESM::REC_SNDG:
            {
                record = std::make_unique<EsmTool::Record<ESM::SoundGenerator>>();
                break;
            }
            case ESM::REC_SOUN:
            {
                record = std::make_unique<EsmTool::Record<ESM::Sound>>();
                break;
            }
            case ESM::REC_SPEL:
            {
                record = std::make_unique<EsmTool::Record<ESM::Spell>>();
                break;
            }
            case ESM::REC_STAT:
            {
                record = std::make_unique<EsmTool::Record<ESM::Static>>();
                break;
            }
            case ESM::REC_WEAP:
            {
                record = std::make_unique<EsmTool::Record<ESM::Weapon>>();
                break;
            }
            case ESM::REC_SSCR:
            {
                record = std::make_unique<EsmTool::Record<ESM::StartScript>>();
                break;
            }
            case ESM::REC_CSTA:
            {
                record = std::make_unique<EsmTool::Record<CellState>>();
                break;
            }
            default:
                break;
        }
        if (record)
        {
            record->mType = type;
        }
        return record;
    }

    template <>
    void Record<ESM::Activator>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Potion>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Icon: " << mData.mIcon << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        std::cout << "  Flags: " << potionFlags(mData.mData.mFlags) << std::endl;
        printEffectList(mData.mEffects);
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Armor>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Icon: " << mData.mIcon << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        if (!mData.mEnchant.empty())
            std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
        std::cout << "  Type: " << armorTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")" << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        std::cout << "  Health: " << mData.mData.mHealth << std::endl;
        std::cout << "  Armor: " << mData.mData.mArmor << std::endl;
        std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
        for (const ESM::PartReference& part : mData.mParts.mParts)
        {
            std::cout << "  Body Part: " << bodyPartLabel(part.mPart) << " (" << (int)(part.mPart) << ")" << std::endl;
            std::cout << "    Male Name: " << part.mMale << std::endl;
            if (!part.mFemale.empty())
                std::cout << "    Female Name: " << part.mFemale << std::endl;
        }

        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Apparatus>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Icon: " << mData.mIcon << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Type: " << apparatusTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")"
                  << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        std::cout << "  Quality: " << mData.mData.mQuality << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::BodyPart>::print()
    {
        std::cout << "  Race: " << mData.mRace << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Type: " << meshTypeLabel(mData.mData.mType) << " (" << (int)mData.mData.mType << ")"
                  << std::endl;
        std::cout << "  Flags: " << bodyPartFlags(mData.mData.mFlags) << std::endl;
        std::cout << "  Part: " << meshPartLabel(mData.mData.mPart) << " (" << (int)mData.mData.mPart << ")"
                  << std::endl;
        std::cout << "  Vampire: " << (int)mData.mData.mVampire << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
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

    template <>
    void Record<ESM::BirthSign>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Texture: " << mData.mTexture << std::endl;
        std::cout << "  Description: " << mData.mDescription << std::endl;
        for (const auto& power : mData.mPowers.mList)
            std::cout << "  Power: " << power << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Cell>::print()
    {
        // None of the cells have names...
        if (!mData.mName.empty())
            std::cout << "  Name: " << mData.mName << std::endl;
        if (!mData.mRegion.empty())
            std::cout << "  Region: " << mData.mRegion << std::endl;
        std::cout << "  Flags: " << cellFlags(mData.mData.mFlags) << std::endl;

        std::cout << "  Coordinates: "
                  << " (" << mData.getGridX() << "," << mData.getGridY() << ")" << std::endl;

        if (mData.mData.mFlags & ESM::Cell::Interior && !(mData.mData.mFlags & ESM::Cell::QuasiEx))
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
            std::cout << std::format("  Map Color: 0x{:08X}\n", mData.mMapColor);
        std::cout << "  RefId counter: " << mData.mRefNumCounter << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Class>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Description: " << mData.mDescription << std::endl;
        std::cout << "  Playable: " << mData.mData.mIsPlayable << std::endl;
        std::cout << std::format("  AI Services: 0x{:08X}\n", mData.mData.mServices);
        for (size_t i = 0; i < mData.mData.mAttribute.size(); ++i)
            std::cout << "  Attribute" << (i + 1) << ": " << attributeLabel(mData.mData.mAttribute[i]) << " ("
                      << mData.mData.mAttribute[i] << ")" << std::endl;
        std::cout << "  Specialization: " << specializationLabel(mData.mData.mSpecialization) << " ("
                  << mData.mData.mSpecialization << ")" << std::endl;
        for (const auto& skills : mData.mData.mSkills)
            std::cout << "  Minor Skill: " << skillLabel(skills[0]) << " (" << skills[0] << ")" << std::endl;
        for (const auto& skills : mData.mData.mSkills)
            std::cout << "  Major Skill: " << skillLabel(skills[1]) << " (" << skills[1] << ")" << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Clothing>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Icon: " << mData.mIcon << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        if (!mData.mEnchant.empty())
            std::cout << "  Enchantment: " << mData.mEnchant << std::endl;
        std::cout << "  Type: " << clothingTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")"
                  << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
        for (const ESM::PartReference& part : mData.mParts.mParts)
        {
            std::cout << "  Body Part: " << bodyPartLabel(part.mPart) << " (" << (int)(part.mPart) << ")" << std::endl;
            std::cout << "    Male Name: " << part.mMale << std::endl;
            if (!part.mFemale.empty())
                std::cout << "    Female Name: " << part.mFemale << std::endl;
        }
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Container>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Flags: " << containerFlags(mData.mFlags) << std::endl;
        std::cout << "  Weight: " << mData.mWeight << std::endl;
        for (const ESM::ContItem& item : mData.mInventory.mList)
            std::cout << std::format("  Inventory: Count: {:4d} Item: ", item.mCount) << item.mItem << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Creature>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Flags: " << creatureFlags((int)mData.mFlags) << std::endl;
        std::cout << "  Blood Type: " << mData.mBloodType + 1 << std::endl;
        std::cout << "  Original: " << mData.mOriginal << std::endl;
        std::cout << "  Scale: " << mData.mScale << std::endl;

        std::cout << "  Type: " << creatureTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")"
                  << std::endl;
        std::cout << "  Level: " << mData.mData.mLevel << std::endl;

        std::cout << "  Attributes:" << std::endl;
        for (size_t i = 0; i < mData.mData.mAttributes.size(); ++i)
            std::cout << "    " << ESM::Attribute::indexToRefId(static_cast<int>(i)) << ": "
                      << mData.mData.mAttributes[i] << std::endl;

        std::cout << "  Health: " << mData.mData.mHealth << std::endl;
        std::cout << "  Magicka: " << mData.mData.mMana << std::endl;
        std::cout << "  Fatigue: " << mData.mData.mFatigue << std::endl;
        std::cout << "  Soul: " << mData.mData.mSoul << std::endl;
        std::cout << "  Combat: " << mData.mData.mCombat << std::endl;
        std::cout << "  Magic: " << mData.mData.mMagic << std::endl;
        std::cout << "  Stealth: " << mData.mData.mStealth << std::endl;
        std::cout << "  Attack1: " << mData.mData.mAttack[0] << "-" << mData.mData.mAttack[1] << std::endl;
        std::cout << "  Attack2: " << mData.mData.mAttack[2] << "-" << mData.mData.mAttack[3] << std::endl;
        std::cout << "  Attack3: " << mData.mData.mAttack[4] << "-" << mData.mData.mAttack[5] << std::endl;
        std::cout << "  Gold: " << mData.mData.mGold << std::endl;

        for (const ESM::ContItem& item : mData.mInventory.mList)
            std::cout << std::format("  Inventory: Count: {:4d} Item: ", item.mCount) << item.mItem << std::endl;

        for (const auto& spell : mData.mSpells.mList)
            std::cout << "  Spell: " << spell << std::endl;

        printTransport(mData.getTransport());

        std::cout << "  Artificial Intelligence: " << std::endl;
        std::cout << "    AI Hello:" << (int)mData.mAiData.mHello << std::endl;
        std::cout << "    AI Fight:" << (int)mData.mAiData.mFight << std::endl;
        std::cout << "    AI Flee:" << (int)mData.mAiData.mFlee << std::endl;
        std::cout << "    AI Alarm:" << (int)mData.mAiData.mAlarm << std::endl;
        std::cout << std::format("    AI Services:0x{:08X}\n", mData.mAiData.mServices);

        for (const ESM::AIPackage& package : mData.mAiPackage.mList)
            printAIPackage(package);
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Dialogue>::print()
    {
        std::cout << "  StringId: " << mData.mStringId << std::endl;
        std::cout << "  Type: " << dialogTypeLabel(mData.mType) << " (" << (int)mData.mType << ")" << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
        // Sadly, there are no DialInfos, because the loader dumps as it
        // loads, rather than loading and then dumping. :-( Anyone mind if
        // I change this?
        for (const ESM::DialInfo& info : mData.mInfo)
            std::cout << "INFO!" << info.mId << std::endl;
    }

    template <>
    void Record<ESM::Door>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  OpenSound: " << mData.mOpenSound << std::endl;
        std::cout << "  CloseSound: " << mData.mCloseSound << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Enchantment>::print()
    {
        std::cout << "  Type: " << enchantTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")" << std::endl;
        std::cout << "  Cost: " << mData.mData.mCost << std::endl;
        std::cout << "  Charge: " << mData.mData.mCharge << std::endl;
        std::cout << "  Flags: " << enchantmentFlags(mData.mData.mFlags) << std::endl;
        printEffectList(mData.mEffects);
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Faction>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Hidden: " << mData.mData.mIsHidden << std::endl;
        for (size_t i = 0; i < mData.mData.mAttribute.size(); ++i)
            std::cout << "  Attribute" << (i + 1) << ": " << attributeLabel(mData.mData.mAttribute[i]) << " ("
                      << mData.mData.mAttribute[i] << ")" << std::endl;
        for (int skill : mData.mData.mSkills)
            if (skill != -1)
                std::cout << "  Skill: " << skillLabel(skill) << " (" << skill << ")" << std::endl;
        for (size_t i = 0; i != mData.mData.mRankData.size(); i++)
            if (!mData.mRanks[i].empty())
            {
                std::cout << "  Rank: " << mData.mRanks[i] << std::endl;
                std::cout << "    Attribute1 Requirement: " << mData.mData.mRankData[i].mAttribute1 << std::endl;
                std::cout << "    Attribute2 Requirement: " << mData.mData.mRankData[i].mAttribute2 << std::endl;
                std::cout << "    One Skill at Level: " << mData.mData.mRankData[i].mPrimarySkill << std::endl;
                std::cout << "    Two Skills at Level: " << mData.mData.mRankData[i].mFavouredSkill << std::endl;
                std::cout << "    Faction Reputation: " << mData.mData.mRankData[i].mFactReputation << std::endl;
            }
        for (const auto& reaction : mData.mReactions)
            std::cout << "  Reaction: " << reaction.second << " = " << reaction.first << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Global>::print()
    {
        std::cout << "  " << mData.mValue << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::GameSetting>::print()
    {
        std::cout << "  " << mData.mValue << std::endl;
    }

    template <>
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
            std::cout << "  Gender: " << static_cast<int>(mData.mData.mGender) << std::endl;
        if (!mData.mSound.empty())
            std::cout << "  Sound File: " << mData.mSound << std::endl;

        std::cout << "  Quest Status: " << questStatusLabel(mData.mQuestStatus) << " (" << mData.mQuestStatus << ")"
                  << std::endl;
        std::cout << "  Type: " << dialogTypeLabel(mData.mData.mType) << std::endl;

        for (const auto& rule : mData.mSelects)
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

    template <>
    void Record<ESM::Ingredient>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Icon: " << mData.mIcon << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        for (int i = 0; i != 4; i++)
        {
            // A value of -1 means no effect
            if (mData.mData.mEffectID[i] == -1)
                continue;
            std::cout << "  Effect: " << magicEffectLabel(mData.mData.mEffectID[i]) << " (" << mData.mData.mEffectID[i]
                      << ")" << std::endl;
            std::cout << "  Skill: " << skillLabel(mData.mData.mSkills[i]) << " (" << mData.mData.mSkills[i] << ")"
                      << std::endl;
            std::cout << "  Attribute: " << attributeLabel(mData.mData.mAttributes[i]) << " ("
                      << mData.mData.mAttributes[i] << ")" << std::endl;
        }
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Land>::print()
    {
        std::cout << "  Coordinates: (" << mData.mX << "," << mData.mY << ")" << std::endl;
        std::cout << "  Flags: " << landFlags(mData.mFlags) << std::endl;
        std::cout << "  DataTypes: " << mData.mDataTypes << std::endl;

        if (const ESM::Land::LandData* data = mData.getLandData(mData.mDataTypes))
        {
            std::cout << "  MinHeight: " << data->mMinHeight << std::endl;
            std::cout << "  MaxHeight: " << data->mMaxHeight << std::endl;
            std::cout << "  DataLoaded: " << data->mDataLoaded << std::endl;
        }
        mData.unloadData();
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::CreatureLevList>::print()
    {
        std::cout << "  Chance for None: " << (int)mData.mChanceNone << std::endl;
        std::cout << "  Flags: " << creatureListFlags(mData.mFlags) << std::endl;
        std::cout << "  Number of items: " << mData.mList.size() << std::endl;
        for (const ESM::LevelledListBase::LevelItem& item : mData.mList)
            std::cout << "  Creature: Level: " << item.mLevel << " Creature: " << item.mId << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::ItemLevList>::print()
    {
        std::cout << "  Chance for None: " << (int)mData.mChanceNone << std::endl;
        std::cout << "  Flags: " << itemListFlags(mData.mFlags) << std::endl;
        std::cout << "  Number of items: " << mData.mList.size() << std::endl;
        for (const ESM::LevelledListBase::LevelItem& item : mData.mList)
            std::cout << "  Inventory: Level: " << item.mLevel << " Item: " << item.mId << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
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

    template <>
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

    template <>
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

    template <>
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

    template <>
    void Record<ESM::LandTexture>::print()
    {
        std::cout << "  Id: " << mData.mId << std::endl;
        std::cout << "  Index: " << mData.mIndex << std::endl;
        std::cout << "  Texture: " << mData.mTexture << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::MagicEffect>::print()
    {
        std::cout << "  Id: " << mData.mId << std::endl;
        std::cout << "  Index: " << magicEffectLabel(ESM::MagicEffect::refIdToIndex(mData.mId)) << " ("
                  << ESM::MagicEffect::refIdToIndex(mData.mId) << ")" << std::endl;
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
        std::cout << "  School: " << schoolLabel(ESM::MagicSchool::skillRefIdToIndex(mData.mData.mSchool)) << " ("
                  << mData.mData.mSchool << ")" << std::endl;
        std::cout << "  Base Cost: " << mData.mData.mBaseCost << std::endl;
        std::cout << "  Unknown 1: " << mData.mData.mUnknown1 << std::endl;
        std::cout << "  Speed: " << mData.mData.mSpeed << std::endl;
        std::cout << "  Unknown 2: " << mData.mData.mUnknown2 << std::endl;
        std::cout << "  RGB Color: "
                  << "(" << mData.mData.mRed << "," << mData.mData.mGreen << "," << mData.mData.mBlue << ")"
                  << std::endl;
    }

    template <>
    void Record<ESM::Miscellaneous>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Model: " << mData.mModel << std::endl;
        std::cout << "  Icon: " << mData.mIcon << std::endl;
        if (!mData.mScript.empty())
            std::cout << "  Script: " << mData.mScript << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        std::cout << "  Is Key: " << (mData.mData.mFlags & ESM::Miscellaneous::Key) << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
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
            std::cout << "  Blood Type: " << mData.mBloodType + 1 << std::endl;

        if (mData.mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
        {
            std::cout << "  Level: " << mData.mNpdt.mLevel << std::endl;
            std::cout << "  Reputation: " << (int)mData.mNpdt.mReputation << std::endl;
            std::cout << "  Disposition: " << (int)mData.mNpdt.mDisposition << std::endl;
            std::cout << "  Rank: " << (int)mData.mNpdt.mRank << std::endl;
            std::cout << "  Gold: " << mData.mNpdt.mGold << std::endl;
        }
        else
        {
            std::cout << "  Level: " << mData.mNpdt.mLevel << std::endl;
            std::cout << "  Reputation: " << (int)mData.mNpdt.mReputation << std::endl;
            std::cout << "  Disposition: " << (int)mData.mNpdt.mDisposition << std::endl;
            std::cout << "  Rank: " << (int)mData.mNpdt.mRank << std::endl;

            std::cout << "  Attributes:" << std::endl;
            for (size_t i = 0; i != mData.mNpdt.mAttributes.size(); i++)
                std::cout << "    " << attributeLabel(static_cast<int>(i)) << ": " << int(mData.mNpdt.mAttributes[i])
                          << std::endl;

            std::cout << "  Skills:" << std::endl;
            for (size_t i = 0; i != mData.mNpdt.mSkills.size(); i++)
                std::cout << "    " << skillLabel(static_cast<int>(i)) << ": " << int(mData.mNpdt.mSkills[i])
                          << std::endl;

            std::cout << "  Health: " << mData.mNpdt.mHealth << std::endl;
            std::cout << "  Magicka: " << mData.mNpdt.mMana << std::endl;
            std::cout << "  Fatigue: " << mData.mNpdt.mFatigue << std::endl;
            std::cout << "  Gold: " << mData.mNpdt.mGold << std::endl;
        }

        for (const ESM::ContItem& item : mData.mInventory.mList)
            std::cout << std::format("  Inventory: Count: {:4d} Item: ", item.mCount) << item.mItem << std::endl;

        for (const auto& spell : mData.mSpells.mList)
            std::cout << "  Spell: " << spell << std::endl;

        printTransport(mData.getTransport());

        std::cout << "  Artificial Intelligence: " << std::endl;
        std::cout << "    AI Hello:" << (int)mData.mAiData.mHello << std::endl;
        std::cout << "    AI Fight:" << (int)mData.mAiData.mFight << std::endl;
        std::cout << "    AI Flee:" << (int)mData.mAiData.mFlee << std::endl;
        std::cout << "    AI Alarm:" << (int)mData.mAiData.mAlarm << std::endl;
        std::cout << std::format("    AI Services:0x{:08X}\n", mData.mAiData.mServices);

        for (const ESM::AIPackage& package : mData.mAiPackage.mList)
            printAIPackage(package);

        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Pathgrid>::print()
    {
        std::cout << "  Cell: " << mData.mCell << std::endl;
        std::cout << "  Coordinates: (" << mData.mData.mX << "," << mData.mData.mY << ")" << std::endl;
        std::cout << "  Granularity: " << mData.mData.mGranularity << std::endl;
        if (mData.mData.mPoints != mData.mPoints.size())
            std::cout << "  Reported Point Count: " << mData.mData.mPoints << std::endl;
        std::cout << "  Point Count: " << mData.mPoints.size() << std::endl;
        std::cout << "  Edge Count: " << mData.mEdges.size() << std::endl;

        int i = 0;
        for (const ESM::Pathgrid::Point& point : mData.mPoints)
        {
            std::cout << "  Point[" << i << "]:" << std::endl;
            std::cout << "    Coordinates: (" << point.mX << "," << point.mY << "," << point.mZ << ")" << std::endl;
            std::cout << "    Auto-Generated: " << (int)point.mAutogenerated << std::endl;
            std::cout << "    Connections: " << (int)point.mConnectionNum << std::endl;
            i++;
        }

        i = 0;
        for (const ESM::Pathgrid::Edge& edge : mData.mEdges)
        {
            std::cout << "  Edge[" << i << "]: " << edge.mV0 << " -> " << edge.mV1 << std::endl;
            if (edge.mV0 >= mData.mData.mPoints || edge.mV1 >= mData.mData.mPoints)
                std::cout << "  BAD POINT IN EDGE!" << std::endl;
            i++;
        }

        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Race>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Description: " << mData.mDescription << std::endl;
        std::cout << "  Flags: " << raceFlags(mData.mData.mFlags) << std::endl;

        std::cout << "  Male:" << std::endl;
        for (int j = 0; j < ESM::Attribute::Length; ++j)
        {
            ESM::RefId id = ESM::Attribute::indexToRefId(j);
            std::cout << "    " << id << ": " << mData.mData.getAttribute(id, true) << std::endl;
        }
        std::cout << "    Height: " << mData.mData.mMaleHeight << std::endl;
        std::cout << "    Weight: " << mData.mData.mMaleWeight << std::endl;

        std::cout << "  Female:" << std::endl;
        for (int j = 0; j < ESM::Attribute::Length; ++j)
        {
            ESM::RefId id = ESM::Attribute::indexToRefId(j);
            std::cout << "    " << id << ": " << mData.mData.getAttribute(id, false) << std::endl;
        }
        std::cout << "    Height: " << mData.mData.mFemaleHeight << std::endl;
        std::cout << "    Weight: " << mData.mData.mFemaleWeight << std::endl;

        for (const auto& bonus : mData.mData.mBonus)
            // Not all races have 7 skills.
            if (bonus.mSkill != -1)
                std::cout << "  Skill: " << skillLabel(bonus.mSkill) << " (" << bonus.mSkill << ") = " << bonus.mBonus
                          << std::endl;

        for (const auto& power : mData.mPowers.mList)
            std::cout << "  Power: " << power << std::endl;

        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Region>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;

        std::cout << "  Weather:" << std::endl;
        std::array<std::string_view, 10> weathers
            = { "Clear", "Cloudy", "Fog", "Overcast", "Rain", "Thunder", "Ash", "Blight", "Snow", "Blizzard" };
        for (size_t i = 0; i < weathers.size(); ++i)
            std::cout << "    " << weathers[i] << ": " << static_cast<unsigned>(mData.mData.mProbabilities[i])
                      << std::endl;
        std::cout << "  Map Color: " << mData.mMapColor << std::endl;
        if (!mData.mSleepList.empty())
            std::cout << "  Sleep List: " << mData.mSleepList << std::endl;
        for (const ESM::Region::SoundRef& soundref : mData.mSoundList)
            std::cout << "  Sound: " << (int)soundref.mChance << " = " << soundref.mSound << std::endl;
    }

    template <>
    void Record<ESM::Script>::print()
    {
        std::cout << "  Name: " << mData.mId << std::endl;

        std::cout << "  Num Shorts: " << mData.mNumShorts << std::endl;
        std::cout << "  Num Longs: " << mData.mNumLongs << std::endl;
        std::cout << "  Num Floats: " << mData.mNumFloats << std::endl;
        std::cout << "  Script Data Size: " << mData.mScriptData.size() << std::endl;
        std::cout << "  Table Size: " << ESM::computeScriptStringTableSize(mData.mVarNames) << std::endl;

        for (const std::string& variable : mData.mVarNames)
            std::cout << "  Variable: " << variable << std::endl;

        std::cout << "  ByteCode: ";
        for (unsigned char byte : mData.mScriptData)
            std::cout << std::format("{:02X}", byte);
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

    template <>
    void Record<ESM::Skill>::print()
    {
        int index = ESM::Skill::refIdToIndex(mData.mId);
        std::cout << "  ID: " << skillLabel(index) << " (" << index << ")" << std::endl;
        std::cout << "  Description: " << mData.mDescription << std::endl;
        std::cout << "  Governing Attribute: " << attributeLabel(mData.mData.mAttribute) << " ("
                  << mData.mData.mAttribute << ")" << std::endl;
        std::cout << "  Specialization: " << specializationLabel(mData.mData.mSpecialization) << " ("
                  << mData.mData.mSpecialization << ")" << std::endl;
        for (int i = 0; i != 4; i++)
            std::cout << "  UseValue[" << i << "]:" << mData.mData.mUseValue[i] << std::endl;
    }

    template <>
    void Record<ESM::SoundGenerator>::print()
    {
        if (!mData.mCreature.empty())
            std::cout << "  Creature: " << mData.mCreature << std::endl;
        std::cout << "  Sound: " << mData.mSound << std::endl;
        std::cout << "  Type: " << soundTypeLabel(mData.mType) << " (" << mData.mType << ")" << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Sound>::print()
    {
        std::cout << "  Sound: " << mData.mSound << std::endl;
        std::cout << "  Volume: " << (int)mData.mData.mVolume << std::endl;
        if (mData.mData.mMinRange != 0 && mData.mData.mMaxRange != 0)
            std::cout << "  Range: " << (int)mData.mData.mMinRange << " - " << (int)mData.mData.mMaxRange << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Spell>::print()
    {
        std::cout << "  Name: " << mData.mName << std::endl;
        std::cout << "  Type: " << spellTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")" << std::endl;
        std::cout << "  Flags: " << spellFlags(mData.mData.mFlags) << std::endl;
        std::cout << "  Cost: " << mData.mData.mCost << std::endl;
        printEffectList(mData.mEffects);
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::StartScript>::print()
    {
        std::cout << "  Start Script: " << mData.mId << std::endl;
        std::cout << "  Start Data: " << mData.mData << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<ESM::Static>::print()
    {
        std::cout << "  Model: " << mData.mModel << std::endl;
    }

    template <>
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
        std::cout << "  Type: " << weaponTypeLabel(mData.mData.mType) << " (" << mData.mData.mType << ")" << std::endl;
        std::cout << "  Flags: " << weaponFlags(mData.mData.mFlags) << std::endl;
        std::cout << "  Weight: " << mData.mData.mWeight << std::endl;
        std::cout << "  Value: " << mData.mData.mValue << std::endl;
        std::cout << "  Health: " << mData.mData.mHealth << std::endl;
        std::cout << "  Speed: " << mData.mData.mSpeed << std::endl;
        std::cout << "  Reach: " << mData.mData.mReach << std::endl;
        std::cout << "  Enchantment Points: " << mData.mData.mEnchant << std::endl;
        if (mData.mData.mChop[0] != 0 && mData.mData.mChop[1] != 0)
            std::cout << "  Chop: " << (int)mData.mData.mChop[0] << "-" << (int)mData.mData.mChop[1] << std::endl;
        if (mData.mData.mSlash[0] != 0 && mData.mData.mSlash[1] != 0)
            std::cout << "  Slash: " << (int)mData.mData.mSlash[0] << "-" << (int)mData.mData.mSlash[1] << std::endl;
        if (mData.mData.mThrust[0] != 0 && mData.mData.mThrust[1] != 0)
            std::cout << "  Thrust: " << (int)mData.mData.mThrust[0] << "-" << (int)mData.mData.mThrust[1] << std::endl;
        std::cout << "  Deleted: " << mIsDeleted << std::endl;
    }

    template <>
    void Record<CellState>::print()
    {
        std::cout << "  Cell Id: \"" << mData.mCellState.mId.toString() << "\"" << std::endl;
        std::cout << "  Water Level: " << mData.mCellState.mWaterLevel << std::endl;
        std::cout << "  Has Fog Of War: " << mData.mCellState.mHasFogOfWar << std::endl;
        std::cout << "  Last Respawn:" << std::endl;
        std::cout << "    Day:" << mData.mCellState.mLastRespawn.mDay << std::endl;
        std::cout << "    Hour:" << mData.mCellState.mLastRespawn.mHour << std::endl;
        if (mData.mCellState.mHasFogOfWar)
        {
            std::cout << "  North Marker Angle: " << osg::RadiansToDegrees(mData.mFogState.mNorthMarkerAngle)
                      << std::endl;
            std::cout << "  Bounds:" << std::endl;
            std::cout << "    Min X: " << mData.mFogState.mBounds.mMinX << std::endl;
            std::cout << "    Min Y: " << mData.mFogState.mBounds.mMinY << std::endl;
            std::cout << "    Max X: " << mData.mFogState.mBounds.mMaxX << std::endl;
            std::cout << "    Max Y: " << mData.mFogState.mBounds.mMaxY << std::endl;
            for (const ESM::FogTexture& fogTexture : mData.mFogState.mFogTextures)
            {
                std::cout << "  Fog Texture:" << std::endl;
                std::cout << "    X: " << fogTexture.mX << std::endl;
                std::cout << "    Y: " << fogTexture.mY << std::endl;
                std::cout << "    Image Data: (" << fogTexture.mImageData.size() << ")" << std::endl;
            }
        }
    }

    template <>
    std::string Record<ESM::Cell>::getId() const
    {
        return std::string(); // No ID for Cell record
    }

    template <>
    std::string Record<ESM::Land>::getId() const
    {
        return std::string(); // No ID for Land record
    }

    template <>
    std::string Record<ESM::MagicEffect>::getId() const
    {
        return std::string(); // No ID for MagicEffect record
    }

    template <>
    std::string Record<ESM::Pathgrid>::getId() const
    {
        return std::string(); // No ID for Pathgrid record
    }

    template <>
    std::string Record<ESM::Skill>::getId() const
    {
        return std::string(); // No ID for Skill record
    }

    template <>
    std::string Record<CellState>::getId() const
    {
        return std::string(); // No ID for CellState record
    }

} // end namespace
