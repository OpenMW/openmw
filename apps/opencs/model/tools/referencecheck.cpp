#include "referencecheck.hpp"

#include "../prefs/state.hpp"

CSMTools::ReferenceCheckStage::ReferenceCheckStage(
    const CSMWorld::RefCollection& references,
    const CSMWorld::RefIdCollection& referencables,
    const CSMWorld::IdCollection<CSMWorld::Cell>& cells,
    const CSMWorld::IdCollection<ESM::Faction>& factions)
    :
    mReferences(references),
    mObjects(referencables),
    mDataSet(referencables.getDataSet()),
    mCells(cells),
    mFactions(factions)
{
    mIgnoreBaseRecords = false;
}

void CSMTools::ReferenceCheckStage::perform(int stage, CSMDoc::Messages &messages)
{
    const CSMWorld::Record<CSMWorld::CellRef>& record = mReferences.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const CSMWorld::CellRef& cellRef = record.get();
    const CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Reference, cellRef.mId);

    // Check for empty reference id
    if (cellRef.mRefID.empty()) {
        messages.push_back(std::make_pair(id, "Instance is not based on an object"));
    } else {
        // Check for non existing referenced object
        if (mObjects.searchId(cellRef.mRefID) == -1) {
            messages.push_back(std::make_pair(id, "Instance of a non-existent object '" + cellRef.mRefID + "'"));
        } else {
            // Check if reference charge is valid for it's proper referenced type
            CSMWorld::RefIdData::LocalIndex localIndex = mDataSet.searchId(cellRef.mRefID);
            bool isLight = localIndex.second == CSMWorld::UniversalId::Type_Light;
            if ((isLight && cellRef.mChargeFloat < -1) || (!isLight && cellRef.mChargeInt < -1)) {
                std::string str = "Invalid charge: ";
                if (localIndex.second == CSMWorld::UniversalId::Type_Light)
                    str += std::to_string(cellRef.mChargeFloat);
                else
                    str += std::to_string(cellRef.mChargeInt);
                messages.push_back(std::make_pair(id, str));
            }
        }
    }

    // If object have owner, check if that owner reference is valid
    if (!cellRef.mOwner.empty() && mObjects.searchId(cellRef.mOwner) == -1)
        messages.push_back(std::make_pair(id, "Owner object '" + cellRef.mOwner + "' does not exist"));

    // If object have creature soul trapped, check if that creature reference is valid
    if (!cellRef.mSoul.empty())
        if (mObjects.searchId(cellRef.mSoul) == -1)
            messages.push_back(std::make_pair(id, "Trapped soul object '" + cellRef.mSoul + "' does not exist"));

    bool hasFaction = !cellRef.mFaction.empty();

    // If object have faction, check if that faction reference is valid
    if (hasFaction)
        if (mFactions.searchId(cellRef.mFaction) == -1)
            messages.push_back(std::make_pair(id, "Faction '" + cellRef.mFaction + "' does not exist"));

    // Check item's faction rank
    if ((hasFaction && cellRef.mFactionRank < -1) || (!hasFaction && cellRef.mFactionRank != -2))
        messages.push_back(std::make_pair(id, "Invalid faction rank " + std::to_string(cellRef.mFactionRank)));

    // If door have destination cell, check if that reference is valid
    if (!cellRef.mDestCell.empty())
        if (mCells.searchId(cellRef.mDestCell) == -1)
            messages.push_back(std::make_pair(id, "Destination cell '" + cellRef.mDestCell + "' does not exist"));

    // Check if scale isn't negative
    if (cellRef.mScale < 0)
    {
        std::string str = "Negative scale: ";
        str += std::to_string(cellRef.mScale);
        messages.push_back(std::make_pair(id, str));
    }

    // Check if enchantement points aren't negative or are at full (-1)
    if (cellRef.mEnchantmentCharge < 0 && cellRef.mEnchantmentCharge != -1)
    {
        std::string str = "Negative enchantment points: ";
        str += std::to_string(cellRef.mEnchantmentCharge);
        messages.push_back(std::make_pair(id, str));
    }

    // Check if gold value isn't negative
    if (cellRef.mGoldValue < 0)
    {
        std::string str = "Negative gold value: ";
        str += cellRef.mGoldValue;
        messages.push_back(std::make_pair(id, str));
    }
}

int CSMTools::ReferenceCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mReferences.getSize();
}
