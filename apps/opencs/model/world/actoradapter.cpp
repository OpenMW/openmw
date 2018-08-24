#include "actoradapter.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/mappings.hpp>

#include "data.hpp"

namespace CSMWorld
{
    ActorAdapter::ActorAdapter(CSMWorld::Data& data)
        : mReferenceables(data.getReferenceables())
        , mRaces(data.getRaces())
        , mBodyParts(data.getBodyParts())
    {
        connect(data.getTableModel(UniversalId::Type_Referenceable), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
                this, SLOT(handleReferenceableChanged(const QModelIndex&, const QModelIndex&)));
        connect(data.getTableModel(UniversalId::Type_Race), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
                this, SLOT(handleRaceChanged(const QModelIndex&, const QModelIndex&)));
        connect(data.getTableModel(UniversalId::Type_BodyPart), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
                this, SLOT(handleBodyPartChanged(const QModelIndex&, const QModelIndex&)));
    }

    const ActorAdapter::ActorPartMap* ActorAdapter::getActorPartMap(const std::string& refId)
    {
        auto it = mActorPartMaps.find(refId);
        if (it != mActorPartMaps.end())
        {
            return &it->second;
        }
        else
        {
            updateActor(refId);
            it = mActorPartMaps.find(refId);
            if (it != mActorPartMaps.end())
                return &it->second;
        }

        return nullptr;
    }

    void ActorAdapter::handleReferenceableChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        // Setup
        const int TypeColumn = mReferenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
        int rowStart = getHighestIndex(topLeft).row();
        int rowEnd = getHighestIndex(botRight).row();

        // Handle each record
        for (int row = rowStart; row <= rowEnd; ++row)
        {
            int type = mReferenceables.getData(row, TypeColumn).toInt();
            if (type == CSMWorld::UniversalId::Type_Creature || type == CSMWorld::UniversalId::Type_Npc)
            {
                // Update the cached npc or creature
                std::string refId = mReferenceables.getId(row);
                if (mActorPartMaps.find(refId) != mActorPartMaps.end())
                    updateActor(refId);
            }
            else if (type == CSMWorld::UniversalId::Type_Armor)
            {
                // TODO update everything?
                // store all items referenced when creating map and check against that here
            }
            else if (type == CSMWorld::UniversalId::Type_Clothing)
            {
                // TODO update everything?
            }
        }
    }

    void ActorAdapter::handleRaceChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        int rowStart = getHighestIndex(topLeft).row();
        int rowEnd = getHighestIndex(botRight).row();
        for (int row = rowStart; row <= rowEnd; ++row)
        {
            std::string raceId = mRaces.getId(row);
            updateNpcsWithRace(raceId);
        }
    }

    void ActorAdapter::handleBodyPartChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        // TODO
        Log(Debug::Info) << "Body Part Changed (" << topLeft.row() << ", " << topLeft.column() << ") (" << botRight.row() << ", " << botRight.column() << ")";
    }

    QModelIndex ActorAdapter::getHighestIndex(QModelIndex index) const
    {
        while (index.parent().isValid())
            index = index.parent();
        return index;
    }

    ActorAdapter::RacePartMap& ActorAdapter::getOrCreateRacePartMap(const std::string& raceId, bool isFemale)
    {
        auto key = std::make_pair(raceId, isFemale);
        auto it = mRacePartMaps.find(key);
        if (it != mRacePartMaps.end())
        {
            return it->second;
        }
        else
        {
            // Create and find result
            updateRaceParts(raceId);
            return mRacePartMaps.find(key)->second;
        }
    }

    void ActorAdapter::updateRaceParts(const std::string& raceId)
    {
        // Convenience function to determine if part is for 1st person view
        auto is1stPersonPart = [](std::string name) {
            return name.size() >= 4 && name.find(".1st", name.size() - 4) != std::string::npos;
        };

        RacePartMap maleMap, femaleMap;
        for (int i = 0; i < mBodyParts.getSize(); ++i)
        {
            auto& record = mBodyParts.getRecord(i);
            if (!record.isDeleted() && record.get().mRace == raceId && record.get().mData.mType == ESM::BodyPart::MT_Skin && !is1stPersonPart(record.get().mId))
            {
                auto& part = record.get();
                auto type = (ESM::BodyPart::MeshPart) part.mData.mPart;
                // Note: Prefer the first part encountered for duplicates. emplace() does not overwrite
                if (part.mData.mFlags & ESM::BodyPart::BPF_Female)
                    femaleMap.emplace(type, part.mId);
                else
                    maleMap.emplace(type, part.mId);
            }
        }

        mRacePartMaps[std::make_pair(raceId, true)] = femaleMap;
        mRacePartMaps[std::make_pair(raceId, false)] = maleMap;
    }

    void ActorAdapter::updateActor(const std::string& refId)
    {
        int index = mReferenceables.searchId(refId);
        if (index != -1)
        {
            int typeColumn = mReferenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
            int recordType = mReferenceables.getData(index, typeColumn).toInt();
            if (recordType == CSMWorld::UniversalId::Type_Creature)
                updateCreature(refId);
            else if (recordType == CSMWorld::UniversalId::Type_Npc)
                updateNpc(refId);
        }
    }

    void ActorAdapter::updateNpc(const std::string& refId)
    {
        auto& record = mReferenceables.getRecord(refId);
        if (record.isDeleted())
        {
            mActorPartMaps.erase(refId);
            return;
        }

        auto& npc = dynamic_cast<const Record<ESM::NPC>&>(record).get();
        auto& femaleRacePartMap = getOrCreateRacePartMap(npc.mRace, true);
        auto& maleRacePartMap = getOrCreateRacePartMap(npc.mRace, false);

        ActorPartMap npcMap;

        // Look at the npc's inventory first
        for (auto& item : npc.mInventory.mList)
        {
            if (item.mCount > 0)
            {
                std::string itemId = item.mItem.toString();
                // Handle armor, weapons, and clothing
                int index = mReferenceables.searchId(itemId);
                if (index != -1 && !mReferenceables.getRecord(index).isDeleted())
                {
                    auto& itemRecord = mReferenceables.getRecord(index);

                    int typeColumn = mReferenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
                    int recordType = mReferenceables.getData(index, typeColumn).toInt();
                    if (recordType == CSMWorld::UniversalId::Type_Armor)
                    {
                        auto& armor = dynamic_cast<const Record<ESM::Armor>&>(itemRecord).get();
                        for (auto& part : armor.mParts.mParts)
                        {
                            std::string bodyPartId;
                            if (!npc.isMale())
                                bodyPartId = part.mFemale;
                            if (bodyPartId.empty())
                                bodyPartId = part.mMale;

                            if (!bodyPartId.empty())
                                npcMap.emplace(static_cast<ESM::PartReferenceType>(part.mPart), bodyPartId);
                        }
                    }
                    else if (recordType == CSMWorld::UniversalId::Type_Clothing)
                    {
                        auto& clothing = dynamic_cast<const Record<ESM::Clothing>&>(itemRecord).get();
                        for (auto& part : clothing.mParts.mParts)
                        {
                            std::string bodyPartId;
                            if (!npc.isMale())
                                bodyPartId = part.mFemale;
                            if (bodyPartId.empty())
                                bodyPartId = part.mMale;

                            if (!bodyPartId.empty())
                                npcMap.emplace(static_cast<ESM::PartReferenceType>(part.mPart), bodyPartId);
                        }
                    }
                }
            }
        }

        // Fill in the rest with body parts
        for (int i = 0; i < ESM::PRT_Count; ++i)
        {
            auto type = static_cast<ESM::PartReferenceType>(i);
            if (npcMap.find(type) == npcMap.end())
            {
                switch (type)
                {
                    case ESM::PRT_Head:
                        npcMap.emplace(type, npc.mHead);
                        break;
                    case ESM::PRT_Hair:
                        npcMap.emplace(type, npc.mHair);
                        break;
                    case ESM::PRT_Skirt:
                    case ESM::PRT_Shield:
                    case ESM::PRT_RPauldron:
                    case ESM::PRT_LPauldron:
                    case ESM::PRT_Weapon:
                        // No body part associated
                        break;
                    default:
                    {
                        std::string bodyPartId;
                        // Check female map if applicable
                        if (!npc.isMale())
                        {
                            auto partIt = femaleRacePartMap.find(ESM::getMeshPart(type));
                            if (partIt != femaleRacePartMap.end())
                                bodyPartId = partIt->second;
                        }

                        // Check male map next
                        if (bodyPartId.empty() || npc.isMale())
                        {
                            auto partIt = maleRacePartMap.find(ESM::getMeshPart(type));
                            if (partIt != maleRacePartMap.end())
                                bodyPartId = partIt->second;
                        }

                        // Add to map
                        if (!bodyPartId.empty())
                        {
                            npcMap.emplace(type, bodyPartId);
                        }
                    }
                }
            }
        }

        mActorPartMaps[refId] = npcMap;
        emit actorChanged(refId);
    }

    void ActorAdapter::updateCreature(const std::string& refId)
    {
        emit actorChanged(refId);
    }

    void ActorAdapter::updateNpcsWithRace(const std::string& raceId)
    {
        for (auto it : mActorPartMaps)
        {
            auto& refId = it.first;
            auto& npc = dynamic_cast<const Record<ESM::NPC>&>(mReferenceables.getRecord(refId)).get();
            if (npc.mRace == raceId)
            {
                updateNpc(refId);
            }
        }
    }
}
