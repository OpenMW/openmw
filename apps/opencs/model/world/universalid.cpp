#include "universalid.hpp"

#include <algorithm>
#include <compare>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace
{
    struct TypeData
    {
        CSMWorld::UniversalId::Class mClass;
        CSMWorld::UniversalId::Type mType;
        std::string_view mName;
        std::string_view mIcon;
    };

    constexpr TypeData sNoArg[] = {
        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, "-", ":placeholder" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Globals, "Global Variables",
            ":global-variable" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Gmsts, "Game Settings", ":gmst" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Skills, "Skills", ":skill" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Classes, "Classes", ":class" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Factions, "Factions", ":faction" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Races, "Races", ":race" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Sounds, "Sounds", ":sound" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Scripts, "Scripts", ":script" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Regions, "Regions", ":region" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Birthsigns, "Birthsigns", ":birthsign" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Spells, "Spells", ":spell" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Topics, "Topics", ":dialogue-topics" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Journals, "Journals",
            ":journal-topics" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_TopicInfos, "Topic Infos",
            ":dialogue-info" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_JournalInfos, "Journal Infos",
            ":journal-topic-infos" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Cells, "Cells", ":cell" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Enchantments, "Enchantments",
            ":enchantment" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_BodyParts, "Body Parts", ":body-part" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Referenceables, "Objects", ":object" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_References, "Instances", ":instance" },
        { CSMWorld::UniversalId::Class_NonRecord, CSMWorld::UniversalId::Type_RegionMap, "Region Map", ":region-map" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Filters, "Filters", ":filter" },
        { CSMWorld::UniversalId::Class_ResourceList, CSMWorld::UniversalId::Type_Meshes, "Meshes", ":resources-mesh" },
        { CSMWorld::UniversalId::Class_ResourceList, CSMWorld::UniversalId::Type_Icons, "Icons", ":resources-icon" },
        { CSMWorld::UniversalId::Class_ResourceList, CSMWorld::UniversalId::Type_Musics, "Music Files",
            ":resources-music" },
        { CSMWorld::UniversalId::Class_ResourceList, CSMWorld::UniversalId::Type_SoundsRes, "Sound Files",
            ":resources-sound" },
        { CSMWorld::UniversalId::Class_ResourceList, CSMWorld::UniversalId::Type_Textures, "Textures",
            ":resources-texture" },
        { CSMWorld::UniversalId::Class_ResourceList, CSMWorld::UniversalId::Type_Videos, "Videos", ":resources-video" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_DebugProfiles, "Debug Profiles",
            ":debug-profile" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_SelectionGroup, "Selection Groups", "" },
        { CSMWorld::UniversalId::Class_Transient, CSMWorld::UniversalId::Type_RunLog, "Run Log", ":run-log" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_SoundGens, "Sound Generators",
            ":sound-generator" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_MagicEffects, "Magic Effects",
            ":magic-effect" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Lands, "Lands", ":land-heightmap" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_LandTextures, "Land Textures",
            ":land-texture" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Pathgrids, "Pathgrids", ":pathgrid" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_StartScripts, "Start Scripts",
            ":start-script" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_MetaDatas, "Metadata", ":metadata" },
    };

    constexpr TypeData sIdArg[] = {
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Global, "Global Variable",
            ":global-variable" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Gmst, "Game Setting", ":gmst" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Skill, "Skill", ":skill" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Class, "Class", ":class" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Faction, "Faction", ":faction" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Race, "Race", ":race" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Sound, "Sound", ":sound" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Script, "Script", ":script" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Region, "Region", ":region" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Birthsign, "Birthsign", ":birthsign" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Spell, "Spell", ":spell" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Topic, "Topic", ":dialogue-topics" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Journal, "Journal", ":journal-topics" },
        { CSMWorld::UniversalId::Class_SubRecord, CSMWorld::UniversalId::Type_TopicInfo, "TopicInfo",
            ":dialogue-info" },
        { CSMWorld::UniversalId::Class_SubRecord, CSMWorld::UniversalId::Type_JournalInfo, "JournalInfo",
            ":journal-topic-infos" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Cell, "Cell", ":cell" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Cell_Missing, "Cell", ":cell" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Referenceable, "Object", ":object" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Activator, "Activator", ":activator" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Potion, "Potion", ":potion" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Apparatus, "Apparatus", ":apparatus" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Armor, "Armor", ":armor" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Book, "Book", ":book" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Clothing, "Clothing", ":clothing" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Container, "Container", ":container" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Creature, "Creature", ":creature" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Door, "Door", ":door" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Ingredient, "Ingredient", ":ingredient" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_CreatureLevelledList,
            "Creature Levelled List", ":levelled-creature" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_ItemLevelledList, "Item Levelled List",
            ":levelled-item" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Light, "Light", ":light" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Lockpick, "Lockpick", ":lockpick" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Miscellaneous, "Miscellaneous",
            ":miscellaneous" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Npc, "NPC", ":npc" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Probe, "Probe", ":probe" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Repair, "Repair", ":repair" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Static, "Static", ":static" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Weapon, "Weapon", ":weapon" },
        { CSMWorld::UniversalId::Class_SubRecord, CSMWorld::UniversalId::Type_Reference, "Instance", ":instance" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Filter, "Filter", ":filter" },
        { CSMWorld::UniversalId::Class_Collection, CSMWorld::UniversalId::Type_Scene, "Scene", ":scene" },
        { CSMWorld::UniversalId::Class_Collection, CSMWorld::UniversalId::Type_Preview, "Preview", ":edit-preview" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Enchantment, "Enchantment", ":enchantment" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_BodyPart, "Body Part", ":body-part" },
        { CSMWorld::UniversalId::Class_Resource, CSMWorld::UniversalId::Type_Mesh, "Mesh", ":resources-mesh" },
        { CSMWorld::UniversalId::Class_Resource, CSMWorld::UniversalId::Type_Icon, "Icon", ":resources-icon" },
        { CSMWorld::UniversalId::Class_Resource, CSMWorld::UniversalId::Type_Music, "Music", ":resources-music" },
        { CSMWorld::UniversalId::Class_Resource, CSMWorld::UniversalId::Type_SoundRes, "Sound File",
            ":resources-sound" },
        { CSMWorld::UniversalId::Class_Resource, CSMWorld::UniversalId::Type_Texture, "Texture", ":resources-texture" },
        { CSMWorld::UniversalId::Class_Resource, CSMWorld::UniversalId::Type_Video, "Video", ":resources-video" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_DebugProfile, "Debug Profile",
            ":debug-profile" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_SoundGen, "Sound Generator",
            ":sound-generator" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_MagicEffect, "Magic Effect",
            ":magic-effect" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Land, "Land", ":land-heightmap" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_LandTexture, "Land Texture",
            ":land-texture" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Pathgrid, "Pathgrid", ":pathgrid" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_StartScript, "Start Script",
            ":start-script" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_MetaData, "Metadata", ":metadata" },
    };

    constexpr TypeData sIndexArg[] = {
        { CSMWorld::UniversalId::Class_Transient, CSMWorld::UniversalId::Type_VerificationResults,
            "Verification Results", ":menu-verify" },
        { CSMWorld::UniversalId::Class_Transient, CSMWorld::UniversalId::Type_LoadErrorLog, "Load Error Log",
            ":error-log" },
        { CSMWorld::UniversalId::Class_Transient, CSMWorld::UniversalId::Type_Search, "Global Search", ":menu-search" },
    };

    struct WriteToStream
    {
        std::ostream& mStream;

        void operator()(std::monostate /*value*/) const {}

        template <class T>
        void operator()(const T& value) const
        {
            mStream << ": " << value;
        }

        void operator()(const ESM::RefId& value) const { mStream << ": " << value.toString(); }
    };

    struct GetTypeData
    {
        std::span<const TypeData> operator()(std::monostate /*value*/) const { return sNoArg; }

        std::span<const TypeData> operator()(int /*value*/) const { return sIndexArg; }

        template <class T>
        std::span<const TypeData> operator()(const T& /*value*/) const
        {
            return sIdArg;
        }
    };

    std::string toString(CSMWorld::UniversalId::ArgumentType value)
    {
        switch (value)
        {
            case CSMWorld::UniversalId::ArgumentType_None:
                return "None";
            case CSMWorld::UniversalId::ArgumentType_Id:
                return "Id";
            case CSMWorld::UniversalId::ArgumentType_Index:
                return "Index";
            case CSMWorld::UniversalId::ArgumentType_RefId:
                return "RefId";
        }

        return std::to_string(value);
    }

    CSMWorld::UniversalId::Class getClassByType(CSMWorld::UniversalId::Type type)
    {
        if (const auto it
            = std::find_if(std::begin(sIdArg), std::end(sIdArg), [&](const TypeData& v) { return v.mType == type; });
            it != std::end(sIdArg))
            return it->mClass;
        if (const auto it = std::find_if(
                std::begin(sIndexArg), std::end(sIndexArg), [&](const TypeData& v) { return v.mType == type; });
            it != std::end(sIndexArg))
            return it->mClass;
        if (const auto it
            = std::find_if(std::begin(sNoArg), std::end(sNoArg), [&](const TypeData& v) { return v.mType == type; });
            it != std::end(sNoArg))
            return it->mClass;
        throw std::logic_error("invalid UniversalId type: " + std::to_string(type));
    }
}

CSMWorld::UniversalId::UniversalId(const std::string& universalId)
    : mValue(std::monostate{})
{
    std::string::size_type index = universalId.find(':');

    if (index != std::string::npos)
    {
        std::string type = universalId.substr(0, index);

        for (const TypeData& value : sIdArg)
            if (type == value.mName)
            {
                mType = value.mType;
                mClass = value.mClass;
                mValue = universalId.substr(index + 2);
                return;
            }

        for (const TypeData& value : sIndexArg)
            if (type == value.mName)
            {
                mType = value.mType;
                mClass = value.mClass;

                std::istringstream stream(universalId.substr(index + 2));

                int index = 0;
                if (stream >> index)
                {
                    mValue = index;
                    return;
                }

                break;
            }
    }
    else
    {
        for (const TypeData& value : sIndexArg)
            if (universalId == value.mName)
            {
                mType = value.mType;
                mClass = value.mClass;
                return;
            }
    }

    throw std::runtime_error("invalid UniversalId: " + universalId);
}

CSMWorld::UniversalId::UniversalId(Type type)
    : mType(type)
    , mValue(std::monostate{})
{
    for (const TypeData& value : sNoArg)
        if (type == value.mType)
        {
            mClass = value.mClass;
            return;
        }

    for (const TypeData& value : sIdArg)
        if (type == value.mType)
        {
            mValue = std::string();
            mClass = value.mClass;
            return;
        }

    for (const TypeData& value : sIndexArg)
        if (type == value.mType)
        {
            mValue = int{};
            mClass = value.mClass;
            return;
        }

    throw std::logic_error("invalid argument-less UniversalId type");
}

CSMWorld::UniversalId::UniversalId(Type type, const std::string& id)
    : mType(type)
    , mValue(id)
{
    for (const TypeData& value : sIdArg)
        if (type == value.mType)
        {
            mClass = value.mClass;
            return;
        }
    throw std::logic_error("invalid ID argument UniversalId type: " + std::to_string(type));
}

CSMWorld::UniversalId::UniversalId(Type type, ESM::RefId id)
    : mType(type)
    , mValue(id)
{
    for (const TypeData& value : sIdArg)
        if (type == value.mType)
        {
            mClass = value.mClass;
            return;
        }
    throw std::logic_error("invalid RefId argument UniversalId type: " + std::to_string(type));
}

CSMWorld::UniversalId::UniversalId(Type type, const UniversalId& id)
    : mClass(getClassByType(type))
    , mType(type)
    , mValue(id.mValue)
{
}

CSMWorld::UniversalId::UniversalId(Type type, int index)
    : mType(type)
    , mValue(index)
{
    for (const TypeData& value : sIndexArg)
        if (type == value.mType)
        {
            mClass = value.mClass;
            return;
        }

    throw std::logic_error("invalid index argument UniversalId type: " + std::to_string(type));
}

CSMWorld::UniversalId::Class CSMWorld::UniversalId::getClass() const
{
    return mClass;
}

CSMWorld::UniversalId::ArgumentType CSMWorld::UniversalId::getArgumentType() const
{
    return static_cast<CSMWorld::UniversalId::ArgumentType>(mValue.index());
}

CSMWorld::UniversalId::Type CSMWorld::UniversalId::getType() const
{
    return mType;
}

const std::string& CSMWorld::UniversalId::getId() const
{
    if (const std::string* result = std::get_if<std::string>(&mValue))
        return *result;

    if (const ESM::RefId* refId = std::get_if<ESM::RefId>(&mValue))
        if (const ESM::StringRefId* result = refId->getIf<ESM::StringRefId>())
            return result->getValue();

    throw std::logic_error("invalid access to ID of " + ::toString(getArgumentType()) + " UniversalId");
}

int CSMWorld::UniversalId::getIndex() const
{
    if (const int* result = std::get_if<int>(&mValue))
        return *result;

    throw std::logic_error("invalid access to index of " + ::toString(getArgumentType()) + " UniversalId");
}

ESM::RefId CSMWorld::UniversalId::getRefId() const
{
    if (const ESM::RefId* result = std::get_if<ESM::RefId>(&mValue))
        return *result;

    throw std::logic_error("invalid access to RefId of " + ::toString(getArgumentType()) + " UniversalId");
}

std::string CSMWorld::UniversalId::getTypeName() const
{
    const std::span<const TypeData> typeData = std::visit(GetTypeData{}, mValue);

    for (const TypeData& value : typeData)
        if (value.mType == mType)
            return std::string(value.mName);

    throw std::logic_error("failed to retrieve UniversalId type name");
}

std::string CSMWorld::UniversalId::toString() const
{
    std::ostringstream stream;

    stream << getTypeName();

    std::visit(WriteToStream{ stream }, mValue);

    return stream.str();
}

std::string CSMWorld::UniversalId::getIcon() const
{
    const std::span<const TypeData> typeData = std::visit(GetTypeData{}, mValue);

    for (const TypeData& value : typeData)
        if (value.mType == mType)
            return std::string(value.mIcon);

    throw std::logic_error("failed to retrieve UniversalId type icon");
}

std::vector<CSMWorld::UniversalId::Type> CSMWorld::UniversalId::listReferenceableTypes()
{
    std::vector<CSMWorld::UniversalId::Type> list;

    for (const TypeData& value : sIdArg)
        if (value.mClass == Class_RefRecord)
            list.push_back(value.mType);

    return list;
}

std::vector<CSMWorld::UniversalId::Type> CSMWorld::UniversalId::listTypes(int classes)
{
    std::vector<CSMWorld::UniversalId::Type> list;

    for (const TypeData& value : sNoArg)
        if (value.mClass & classes)
            list.push_back(value.mType);

    for (const TypeData& value : sIdArg)
        if (value.mClass & classes)
            list.push_back(value.mType);

    for (const TypeData& value : sIndexArg)
        if (value.mClass & classes)
            list.push_back(value.mType);

    return list;
}

CSMWorld::UniversalId::Type CSMWorld::UniversalId::getParentType(Type type)
{
    for (const TypeData& value : sIdArg)
        if (type == value.mType)
        {
            if (value.mClass == Class_RefRecord)
                return Type_Referenceables;

            if (value.mClass == Class_SubRecord || value.mClass == Class_Record || value.mClass == Class_Resource)
            {
                if (type == Type_Cell_Missing)
                    return Type_Cells;

                return static_cast<Type>(type - 1);
            }

            break;
        }

    return Type_None;
}

bool CSMWorld::operator==(const CSMWorld::UniversalId& left, const CSMWorld::UniversalId& right)
{
    return std::tie(left.mClass, left.mType, left.mValue) == std::tie(right.mClass, right.mType, right.mValue);
}

bool CSMWorld::operator<(const UniversalId& left, const UniversalId& right)
{
    return std::tie(left.mClass, left.mType, left.mValue) < std::tie(right.mClass, right.mType, right.mValue);
}
