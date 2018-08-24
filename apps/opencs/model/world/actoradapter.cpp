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

    const ActorAdapter::ActorPartMap* ActorAdapter::getActorParts(const std::string& refId, bool create)
    {
        auto it = mCachedActors.find(refId);
        if (it != mCachedActors.end())
        {
            return &it->second.parts;
        }
        else if (create)
        {
            updateActor(refId);
            return getActorParts(refId, false);
        }
        else
        {
            return nullptr;
        }
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
                if (mCachedActors.find(refId) != mCachedActors.end())
                    updateActor(refId);
            }
            else if (type == CSMWorld::UniversalId::Type_Armor || type == CSMWorld::UniversalId::Type_Clothing)
            {
                std::string refId = mReferenceables.getId(row);
                updateActorsWithDependency(refId);
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
            updateActorsWithDependency(raceId);
        }
    }

    void ActorAdapter::handleBodyPartChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        int rowStart = getHighestIndex(topLeft).row();
        int rowEnd = getHighestIndex(botRight).row();
        for (int row = rowStart; row <= rowEnd; ++row)
        {
            // Manually update race specified by part
            auto& record = mBodyParts.getRecord(row);
            if (!record.isDeleted())
            {
                updateRace(record.get().mRace);
            }

            // Update entries with a tracked dependency
            std::string partId = mBodyParts.getId(row);
            updateRacesWithDependency(partId);
            updateActorsWithDependency(partId);
        }
    }

    QModelIndex ActorAdapter::getHighestIndex(QModelIndex index) const
    {
        while (index.parent().isValid())
            index = index.parent();
        return index;
    }

    bool ActorAdapter::is1stPersonPart(const std::string& name) const
    {
        return name.size() >= 4 && name.find(".1st", name.size() - 4) != std::string::npos;
    }

    ActorAdapter::RaceData& ActorAdapter::getRaceData(const std::string& raceId)
    {
        auto it = mCachedRaces.find(raceId);
        if (it != mCachedRaces.end())
        {
            return it->second;
        }
        else
        {
            // Create and find result
            updateRace(raceId);
            return mCachedRaces.find(raceId)->second;
        }
    }

    void ActorAdapter::updateRace(const std::string& raceId)
    {
        // Retrieve or create cache entry
        auto raceDataIt = mCachedRaces.find(raceId);
        if (raceDataIt == mCachedRaces.end())
        {
            auto result = mCachedRaces.emplace(raceId, RaceData());
            raceDataIt = result.first;
        }

        auto& raceData = raceDataIt->second;
        raceData.femaleParts.clear();
        raceData.maleParts.clear();
        raceData.dependencies.clear();

        // Construct entry
        for (int i = 0; i < mBodyParts.getSize(); ++i)
        {
            auto& record = mBodyParts.getRecord(i);
            if (!record.isDeleted() && record.get().mRace == raceId)
            {
                auto& part = record.get();

                // Part could affect race data
                raceData.dependencies.emplace(part.mId, true);

                // Add base types
                if (part.mData.mType == ESM::BodyPart::MT_Skin && !is1stPersonPart(part.mId))
                {
                    auto type = (ESM::BodyPart::MeshPart) part.mData.mPart;
                    // Note: Prefer the first part encountered for duplicates. emplace() does not overwrite
                    if (part.mData.mFlags & ESM::BodyPart::BPF_Female)
                        raceData.femaleParts.emplace(type, part.mId);
                    else
                        raceData.maleParts.emplace(type, part.mId);
                }
            }
        }

        updateActorsWithDependency(raceId);
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

        // Retrieve record if possible
        if (record.isDeleted())
        {
            mCachedActors.erase(refId);
            emit actorChanged(refId);
            return;
        }
        auto& npc = dynamic_cast<const Record<ESM::NPC>&>(record).get();

        // Create holder for cached data
        auto actorIt = mCachedActors.find(refId);
        if (actorIt == mCachedActors.end())
        {
            auto result = mCachedActors.emplace(refId, ActorData());
            actorIt = result.first;
        }
        auto& actorData = actorIt->second;

        // Reset old data
        actorData.parts.clear();
        actorData.dependencies.clear();

        // Look at the npc's inventory first
        for (auto& item : npc.mInventory.mList)
        {
            if (item.mCount > 0)
            {
                std::string itemId = item.mItem.toString();
                // Handle armor and clothing
                int index = mReferenceables.searchId(itemId);
                if (index != -1 && !mReferenceables.getRecord(index).isDeleted())
                {
                    auto& itemRecord = mReferenceables.getRecord(index);

                    int typeColumn = mReferenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
                    int recordType = mReferenceables.getData(index, typeColumn).toInt();
                    if (recordType == CSMWorld::UniversalId::Type_Armor)
                    {
                        // Changes here could affect the actor
                        actorData.dependencies.emplace(itemId, true);

                        // Add any parts if there is room
                        auto& armor = dynamic_cast<const Record<ESM::Armor>&>(itemRecord).get();
                        for (auto& part : armor.mParts.mParts)
                        {
                            std::string bodyPartId;
                            if (!npc.isMale())
                                bodyPartId = part.mFemale;
                            if (bodyPartId.empty())
                                bodyPartId = part.mMale;

                            if (!bodyPartId.empty())
                            {
                                actorData.parts.emplace(static_cast<ESM::PartReferenceType>(part.mPart), bodyPartId);
                                actorData.dependencies.emplace(bodyPartId, true);
                            }
                        }
                    }
                    else if (recordType == CSMWorld::UniversalId::Type_Clothing)
                    {
                        // Changes here could affect the actor
                        actorData.dependencies.emplace(itemId, true);

                        // Add any parts if there is room
                        auto& clothing = dynamic_cast<const Record<ESM::Clothing>&>(itemRecord).get();
                        for (auto& part : clothing.mParts.mParts)
                        {
                            std::string bodyPartId;
                            if (!npc.isMale())
                                bodyPartId = part.mFemale;
                            if (bodyPartId.empty())
                                bodyPartId = part.mMale;

                            if (!bodyPartId.empty())
                            {
                                actorData.parts.emplace(static_cast<ESM::PartReferenceType>(part.mPart), bodyPartId);
                                actorData.dependencies.emplace(bodyPartId, true);
                            }
                        }
                    }
                }
            }
        }

        // Lookup cached race parts
        auto& raceData = getRaceData(npc.mRace);

        // Changes to race could affect the actor
        actorData.dependencies.emplace(npc.mRace, true);

        // Fill in the rest with race specific body parts
        for (int i = 0; i < ESM::PRT_Count; ++i)
        {
            auto type = static_cast<ESM::PartReferenceType>(i);
            if (actorData.parts.find(type) == actorData.parts.end())
            {
                switch (type)
                {
                    case ESM::PRT_Head:
                        actorData.parts.emplace(type, npc.mHead);
                        actorData.dependencies.emplace(npc.mHead, true);
                        break;
                    case ESM::PRT_Hair:
                        actorData.parts.emplace(type, npc.mHair);
                        actorData.dependencies.emplace(npc.mHair, true);
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
                            auto partIt = raceData.femaleParts.find(ESM::getMeshPart(type));
                            if (partIt != raceData.femaleParts.end())
                                bodyPartId = partIt->second;
                        }

                        // Check male map next
                        if (bodyPartId.empty() || npc.isMale())
                        {
                            auto partIt = raceData.maleParts.find(ESM::getMeshPart(type));
                            if (partIt != raceData.maleParts.end())
                                bodyPartId = partIt->second;
                        }

                        // Add to map
                        if (!bodyPartId.empty())
                        {
                            actorData.parts.emplace(type, bodyPartId);
                            actorData.dependencies.emplace(bodyPartId, true);
                        }
                    }
                }
            }
        }

        // Signal change to actor
        emit actorChanged(refId);
    }

    void ActorAdapter::updateCreature(const std::string& refId)
    {
        // Signal change to actor
        emit actorChanged(refId);
    }

    void ActorAdapter::updateActorsWithDependency(const std::string& id)
    {
        for (auto it : mCachedActors)
        {
            auto& deps = it.second.dependencies;
            if (deps.find(id) != deps.end())
                updateActor(it.first);
        }
    }

    void ActorAdapter::updateRacesWithDependency(const std::string& id)
    {
        for (auto it : mCachedRaces)
        {
            auto& deps = it.second.dependencies;
            if (deps.find(id) != deps.end())
                updateRace(it.first);
        }
    }
}
