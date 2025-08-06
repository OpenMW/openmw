#include "actoradapter.hpp"

#include <QModelIndex>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <apps/opencs/model/prefs/state.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/mappings.hpp>
#include <components/settings/settings.hpp>

#include "data.hpp"

namespace CSMWorld
{
    const ESM::RefId& ActorAdapter::RaceData::getId() const
    {
        return mId;
    }

    bool ActorAdapter::RaceData::isBeast() const
    {
        return mIsBeast;
    }

    ActorAdapter::RaceData::RaceData()
    {
        mIsBeast = false;
    }

    bool ActorAdapter::RaceData::handlesPart(ESM::PartReferenceType type) const
    {
        switch (type)
        {
            case ESM::PRT_Skirt:
            case ESM::PRT_Shield:
            case ESM::PRT_RPauldron:
            case ESM::PRT_LPauldron:
            case ESM::PRT_Weapon:
                return false;
            default:
                return true;
        }
    }

    const ESM::RefId& ActorAdapter::RaceData::getFemalePart(ESM::PartReferenceType index) const
    {
        return mFemaleParts[ESM::getMeshPart(index)];
    }

    const ESM::RefId& ActorAdapter::RaceData::getMalePart(ESM::PartReferenceType index) const
    {
        return mMaleParts[ESM::getMeshPart(index)];
    }

    const osg::Vec2f& ActorAdapter::RaceData::getGenderWeightHeight(bool isFemale)
    {
        return isFemale ? mWeightsHeights.mFemaleWeightHeight : mWeightsHeights.mMaleWeightHeight;
    }

    bool ActorAdapter::RaceData::hasDependency(const ESM::RefId& id) const
    {
        return mDependencies.find(id) != mDependencies.end();
    }

    void ActorAdapter::RaceData::setFemalePart(ESM::BodyPart::MeshPart index, const ESM::RefId& partId)
    {
        mFemaleParts[index] = partId;
        addOtherDependency(partId);
    }

    void ActorAdapter::RaceData::setMalePart(ESM::BodyPart::MeshPart index, const ESM::RefId& partId)
    {
        mMaleParts[index] = partId;
        addOtherDependency(partId);
    }

    void ActorAdapter::RaceData::addOtherDependency(const ESM::RefId& id)
    {
        if (!id.empty())
            mDependencies.emplace(id);
    }

    void ActorAdapter::RaceData::reset_data(const ESM::RefId& id, const WeightsHeights& raceStats, bool isBeast)
    {
        mId = id;
        mIsBeast = isBeast;
        mWeightsHeights = raceStats;
        for (auto& str : mFemaleParts)
            str = ESM::RefId();
        for (auto& str : mMaleParts)
            str = ESM::RefId();
        mDependencies.clear();

        // Mark self as a dependency
        addOtherDependency(id);
    }

    ActorAdapter::ActorData::ActorData()
    {
        mCreature = false;
        mFemale = false;
    }

    const ESM::RefId& ActorAdapter::ActorData::getId() const
    {
        return mId;
    }

    bool ActorAdapter::ActorData::isCreature() const
    {
        return mCreature;
    }

    bool ActorAdapter::ActorData::isFemale() const
    {
        return mFemale;
    }

    std::string ActorAdapter::ActorData::getSkeleton() const
    {
        if (mCreature || !mSkeletonOverride.empty())
            return "meshes\\" + mSkeletonOverride;

        bool beast = mRaceData ? mRaceData->isBeast() : false;

        if (beast)
            return CSMPrefs::get()["Models"]["baseanimkna"].toString();
        else if (mFemale)
            return CSMPrefs::get()["Models"]["baseanimfemale"].toString();
        else
            return CSMPrefs::get()["Models"]["baseanim"].toString();
    }

    ESM::RefId ActorAdapter::ActorData::getPart(ESM::PartReferenceType index) const
    {
        auto it = mParts.find(index);
        if (it == mParts.end())
        {
            if (mRaceData && mRaceData->handlesPart(index))
            {
                if (mFemale)
                {
                    // Note: we should use male parts for females as fallback
                    if (const ESM::RefId femalePart = mRaceData->getFemalePart(index); !femalePart.empty())
                        return femalePart;
                }

                return mRaceData->getMalePart(index);
            }

            return {};
        }

        return it->second.first;
    }

    const osg::Vec2f& ActorAdapter::ActorData::getRaceWeightHeight() const
    {
        return mRaceData->getGenderWeightHeight(isFemale());
    }

    bool ActorAdapter::ActorData::hasDependency(const ESM::RefId& id) const
    {
        return mDependencies.find(id) != mDependencies.end();
    }

    void ActorAdapter::ActorData::setPart(ESM::PartReferenceType index, const ESM::RefId& partId, int priority)
    {
        auto it = mParts.find(index);
        if (it != mParts.end())
        {
            if (it->second.second >= priority)
                return;
        }

        mParts[index] = std::make_pair(partId, priority);
        addOtherDependency(partId);
    }

    void ActorAdapter::ActorData::addOtherDependency(const ESM::RefId& id)
    {
        if (!id.empty())
            mDependencies.emplace(id);
    }

    void ActorAdapter::ActorData::reset_data(
        const ESM::RefId& id, const std::string& skeleton, bool isCreature, bool isFemale, RaceDataPtr raceData)
    {
        mId = id;
        mCreature = isCreature;
        mFemale = isFemale;
        mSkeletonOverride = skeleton;
        mRaceData = raceData;
        mParts.clear();
        mDependencies.clear();

        // Mark self and race as a dependency
        addOtherDependency(id);
        if (raceData)
            addOtherDependency(raceData->getId());
    }

    ActorAdapter::ActorAdapter(Data& data)
        : mReferenceables(data.getReferenceables())
        , mRaces(data.getRaces())
        , mBodyParts(data.getBodyParts())
    {
        // Setup qt slots and signals
        QAbstractItemModel* refModel = data.getTableModel(UniversalId::Type_Referenceable);
        connect(refModel, &QAbstractItemModel::rowsInserted, this, &ActorAdapter::handleReferenceablesInserted);
        connect(refModel, &QAbstractItemModel::dataChanged, this, &ActorAdapter::handleReferenceableChanged);
        connect(refModel, &QAbstractItemModel::rowsAboutToBeRemoved, this,
            &ActorAdapter::handleReferenceablesAboutToBeRemoved);

        QAbstractItemModel* raceModel = data.getTableModel(UniversalId::Type_Race);
        connect(raceModel, &QAbstractItemModel::rowsInserted, this, &ActorAdapter::handleRacesAboutToBeRemoved);
        connect(raceModel, &QAbstractItemModel::dataChanged, this, &ActorAdapter::handleRaceChanged);
        connect(raceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ActorAdapter::handleRacesAboutToBeRemoved);

        QAbstractItemModel* partModel = data.getTableModel(UniversalId::Type_BodyPart);
        connect(partModel, &QAbstractItemModel::rowsInserted, this, &ActorAdapter::handleBodyPartsInserted);
        connect(partModel, &QAbstractItemModel::dataChanged, this, &ActorAdapter::handleBodyPartChanged);
        connect(
            partModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ActorAdapter::handleBodyPartsAboutToBeRemoved);
    }

    ActorAdapter::ActorDataPtr ActorAdapter::getActorData(const ESM::RefId& id)
    {
        // Return cached actor data if it exists
        ActorDataPtr data = mCachedActors.get(id);
        if (data)
        {
            return data;
        }

        // Create the actor data
        data = std::make_shared<ActorData>();
        setupActor(id, data);
        mCachedActors.insert(id, data);
        return data;
    }

    void ActorAdapter::handleReferenceablesInserted(const QModelIndex& parent, int start, int end)
    {
        // Only rows added at the top level are pertinent. Others are caught by dataChanged handler.
        if (!parent.isValid())
        {
            for (int row = start; row <= end; ++row)
            {
                auto refId = mReferenceables.getId(row);
                markDirtyDependency(refId);
            }
        }

        // Update affected
        updateDirty();
    }

    void ActorAdapter::handleReferenceableChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        int start = getHighestIndex(topLeft).row();
        int end = getHighestIndex(botRight).row();

        // A change to record status (ex. Deleted) returns an invalid botRight
        if (end == -1)
            end = start;

        // Handle each record
        for (int row = start; row <= end; ++row)
        {
            auto refId = mReferenceables.getId(row);
            markDirtyDependency(refId);
        }

        // Update affected
        updateDirty();
    }

    void ActorAdapter::handleReferenceablesAboutToBeRemoved(const QModelIndex& parent, int start, int end)
    {
        // Only rows at the top are pertinent.
        if (!parent.isValid())
        {
            for (int row = start; row <= end; ++row)
            {
                auto refId = mReferenceables.getId(row);
                markDirtyDependency(refId);
            }
        }
    }

    void ActorAdapter::handleReferenceablesRemoved(const QModelIndex& parent, int start, int end)
    {
        // Changes specified in handleReferenceablesAboutToBeRemoved
        updateDirty();
    }

    void ActorAdapter::handleRacesInserted(const QModelIndex& parent, int start, int end)
    {
        // Only rows added at the top are pertinent.
        if (!parent.isValid())
        {
            for (int row = start; row <= end; ++row)
            {
                auto raceId = mReferenceables.getId(row);
                markDirtyDependency(raceId);
            }
        }

        // Update affected
        updateDirty();
    }

    void ActorAdapter::handleRaceChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        int start = getHighestIndex(topLeft).row();
        int end = getHighestIndex(botRight).row();

        // A change to record status (ex. Deleted) returns an invalid botRight
        if (end == -1)
            end = start;

        for (int row = start; row <= end; ++row)
        {
            auto raceId = mRaces.getId(row);
            markDirtyDependency(raceId);
        }

        // Update affected
        updateDirty();
    }

    void ActorAdapter::handleRacesAboutToBeRemoved(const QModelIndex& parent, int start, int end)
    {
        // Only changes at the top are pertinent.
        if (!parent.isValid())
        {
            for (int row = start; row <= end; ++row)
            {
                auto raceId = mRaces.getId(row);
                markDirtyDependency(raceId);
            }
        }
    }

    void ActorAdapter::handleRacesRemoved(const QModelIndex& parent, int start, int end)
    {
        // Changes specified in handleRacesAboutToBeRemoved
        updateDirty();
    }

    void ActorAdapter::handleBodyPartsInserted(const QModelIndex& parent, int start, int end)
    {
        // Only rows added at the top are pertinent.
        if (!parent.isValid())
        {
            for (int row = start; row <= end; ++row)
            {
                // Race specified by part may need update
                auto& record = mBodyParts.getRecord(row);
                if (!record.isDeleted())
                {
                    markDirtyDependency(record.get().mRace);
                }

                auto partId = mBodyParts.getId(row);
                markDirtyDependency(partId);
            }
        }

        // Update affected
        updateDirty();
    }

    void ActorAdapter::handleBodyPartChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
    {
        int start = getHighestIndex(topLeft).row();
        int end = getHighestIndex(botRight).row();

        // A change to record status (ex. Deleted) returns an invalid botRight
        if (end == -1)
            end = start;

        for (int row = start; row <= end; ++row)
        {
            // Race specified by part may need update
            auto& record = mBodyParts.getRecord(row);
            if (!record.isDeleted())
            {
                markDirtyDependency(record.get().mRace);
            }

            // Update entries with a tracked dependency
            auto partId = mBodyParts.getId(row);
            markDirtyDependency(partId);
        }

        // Update affected
        updateDirty();
    }

    void ActorAdapter::handleBodyPartsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
    {
        // Only changes at the top are pertinent.
        if (!parent.isValid())
        {
            for (int row = start; row <= end; ++row)
            {
                auto partId = mBodyParts.getId(row);
                markDirtyDependency(partId);
            }
        }
    }

    void ActorAdapter::handleBodyPartsRemoved(const QModelIndex& parent, int start, int end)
    {
        // Changes specified in handleBodyPartsAboutToBeRemoved
        updateDirty();
    }

    QModelIndex ActorAdapter::getHighestIndex(QModelIndex index) const
    {
        while (index.parent().isValid())
            index = index.parent();
        return index;
    }

    ActorAdapter::RaceDataPtr ActorAdapter::getRaceData(const ESM::RefId& id)
    {
        // Return cached race data if it exists
        RaceDataPtr data = mCachedRaces.get(id);
        if (data)
            return data;

        // Create the race data
        data = std::make_shared<RaceData>();
        setupRace(id, data);
        mCachedRaces.insert(id, data);
        return data;
    }

    void ActorAdapter::setupActor(const ESM::RefId& id, ActorDataPtr data)
    {
        int index = mReferenceables.searchId(id);
        if (index == -1)
        {
            // Record does not exist
            data->reset_data(id);
            emit actorChanged(id);
            return;
        }

        auto& record = mReferenceables.getRecord(index);
        if (record.isDeleted())
        {
            // Record is deleted and therefore not accessible
            data->reset_data(id);
            emit actorChanged(id);
            return;
        }

        const int typeColumn = mReferenceables.findColumnIndex(Columns::ColumnId_RecordType);
        int type = mReferenceables.getData(index, typeColumn).toInt();
        if (type == UniversalId::Type_Creature)
        {
            // Valid creature record
            setupCreature(id, std::move(data));
            emit actorChanged(id);
        }
        else if (type == UniversalId::Type_Npc)
        {
            // Valid npc record
            setupNpc(id, std::move(data));
            emit actorChanged(id);
        }
        else
        {
            // Wrong record type
            data->reset_data(id);
            emit actorChanged(id);
        }
    }

    void ActorAdapter::setupRace(const ESM::RefId& id, RaceDataPtr data)
    {
        int index = mRaces.searchId(id);
        if (index == -1)
        {
            // Record does not exist
            data->reset_data(id);
            return;
        }

        auto& raceRecord = mRaces.getRecord(index);
        if (raceRecord.isDeleted())
        {
            // Record is deleted, so not accessible
            data->reset_data(id);
            return;
        }

        auto& race = raceRecord.get();

        WeightsHeights scaleStats = { osg::Vec2f(race.mData.mMaleWeight, race.mData.mMaleHeight),
            osg::Vec2f(race.mData.mFemaleWeight, race.mData.mFemaleHeight) };

        data->reset_data(id, scaleStats, race.mData.mFlags & ESM::Race::Beast);

        // Setup body parts
        for (int i = 0; i < mBodyParts.getSize(); ++i)
        {
            auto& partRecord = mBodyParts.getRecord(i);

            if (partRecord.isDeleted())
            {
                // Record is deleted, so not accessible.
                continue;
            }

            auto& part = partRecord.get();
            if (part.mRace == id && part.mData.mType == ESM::BodyPart::MT_Skin && !ESM::isFirstPersonBodyPart(part))
            {
                auto type = (ESM::BodyPart::MeshPart)part.mData.mPart;
                bool female = part.mData.mFlags & ESM::BodyPart::BPF_Female;
                if (female)
                    data->setFemalePart(type, part.mId);
                else
                    data->setMalePart(type, part.mId);
            }
        }
    }

    void ActorAdapter::setupNpc(const ESM::RefId& id, ActorDataPtr data)
    {
        // Common setup, record is known to exist and is not deleted
        int index = mReferenceables.searchId(id);
        auto& npc = dynamic_cast<const Record<ESM::NPC>&>(mReferenceables.getRecord(index)).get();

        RaceDataPtr raceData = getRaceData(npc.mRace);
        data->reset_data(id, "", false, !npc.isMale(), std::move(raceData));

        // Add head and hair
        data->setPart(ESM::PRT_Head, npc.mHead, 0);
        data->setPart(ESM::PRT_Hair, npc.mHair, 0);

        // Add inventory items
        for (auto& item : npc.mInventory.mList)
        {
            if (item.mCount <= 0)
                continue;
            auto itemId = item.mItem;
            addNpcItem(itemId, data);
        }
    }

    void ActorAdapter::addNpcItem(const ESM::RefId& itemId, ActorDataPtr data)
    {
        int index = mReferenceables.searchId(itemId);
        if (index == -1)
        {
            // Item does not exist yet
            data->addOtherDependency(itemId);
            return;
        }

        auto& record = mReferenceables.getRecord(index);
        if (record.isDeleted())
        {
            // Item cannot be accessed yet
            data->addOtherDependency(itemId);
            return;
        }

        // Convenience function to add a parts list to actor data
        auto addParts = [&](const std::vector<ESM::PartReference>& list, int priority) {
            for (auto& part : list)
            {
                ESM::RefId partId;
                auto partType = (ESM::PartReferenceType)part.mPart;

                if (data->isFemale())
                    partId = part.mFemale;
                if (partId.empty())
                    partId = part.mMale;

                data->setPart(partType, partId, priority);

                // An another vanilla quirk: hide hairs if an item replaces Head part
                if (partType == ESM::PRT_Head)
                    data->setPart(ESM::PRT_Hair, ESM::RefId(), priority);
            }
        };

        const int typeColumn = mReferenceables.findColumnIndex(Columns::ColumnId_RecordType);
        int type = mReferenceables.getData(index, typeColumn).toInt();
        if (type == UniversalId::Type_Armor)
        {
            auto& armor = dynamic_cast<const Record<ESM::Armor>&>(record).get();
            addParts(armor.mParts.mParts, 1);

            // Changing parts could affect what is picked for rendering
            data->addOtherDependency(itemId);
        }
        else if (type == UniversalId::Type_Clothing)
        {
            auto& clothing = dynamic_cast<const Record<ESM::Clothing>&>(record).get();

            std::vector<ESM::PartReferenceType> parts;
            if (clothing.mData.mType == ESM::Clothing::Robe)
            {
                parts = { ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg, ESM::PRT_RUpperarm,
                    ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee, ESM::PRT_RForearm, ESM::PRT_LForearm,
                    ESM::PRT_Cuirass };
            }
            else if (clothing.mData.mType == ESM::Clothing::Skirt)
            {
                parts = { ESM::PRT_Groin, ESM::PRT_RLeg, ESM::PRT_LLeg };
            }

            std::vector<ESM::PartReference> reservedList;
            for (const auto& p : parts)
            {
                ESM::PartReference pr;
                pr.mPart = p;
                reservedList.emplace_back(pr);
            }

            int priority = parts.size();
            addParts(clothing.mParts.mParts, priority);
            addParts(reservedList, priority);

            // Changing parts could affect what is picked for rendering
            data->addOtherDependency(itemId);
        }
    }

    void ActorAdapter::setupCreature(const ESM::RefId& id, ActorDataPtr data)
    {
        // Record is known to exist and is not deleted
        int index = mReferenceables.searchId(id);
        auto& creature = dynamic_cast<const Record<ESM::Creature>&>(mReferenceables.getRecord(index)).get();

        data->reset_data(id, creature.mModel, true);
    }

    void ActorAdapter::markDirtyDependency(const ESM::RefId& dep)
    {
        for (auto raceIt : mCachedRaces)
        {
            if (raceIt->hasDependency(dep))
                mDirtyRaces.emplace(raceIt->getId());
        }
        for (auto actorIt : mCachedActors)
        {
            if (actorIt->hasDependency(dep))
                mDirtyActors.emplace(actorIt->getId());
        }
    }

    void ActorAdapter::updateDirty()
    {
        // Handle races before actors, since actors are dependent on race
        for (auto& race : mDirtyRaces)
        {
            RaceDataPtr data = mCachedRaces.get(race);
            if (data)
            {
                setupRace(race, std::move(data));
                // Race was changed. Need to mark actor dependencies as dirty.
                // Cannot use markDirtyDependency because that would invalidate
                // the current iterator.
                for (auto actorIt : mCachedActors)
                {
                    if (actorIt->hasDependency(race))
                        mDirtyActors.emplace(actorIt->getId());
                }
            }
        }
        mDirtyRaces.clear();

        for (auto& actor : mDirtyActors)
        {
            ActorDataPtr data = mCachedActors.get(actor);
            if (data)
            {
                setupActor(actor, std::move(data));
            }
        }
        mDirtyActors.clear();
    }
}
