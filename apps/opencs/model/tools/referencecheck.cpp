#include "referencecheck.hpp"

#include "../prefs/state.hpp"

CSMTools::ReferenceCheckStage::ReferenceCheckStage(
    const CSMWorld::RefCollection& references,
    const CSMWorld::RefIdCollection& referencables,
    const CSMWorld::IdCollection<CSMWorld::Cell>& cells,
    const CSMWorld::IdCollection<ESM::Faction>& factions)
    :
    mReferences(references),
    mReferencables(referencables),
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
        messages.push_back(std::make_pair(id, " is an empty instance (not based on an object)"));
    } else {
        // Check for non existing referenced object
        if (mReferencables.searchId(cellRef.mRefID) == -1) {
            messages.push_back(std::make_pair(id, " is referencing non existing object " + cellRef.mRefID));
        } else {
            // Check if reference charge is valid for it's proper referenced type
            CSMWorld::RefIdData::LocalIndex localIndex = mDataSet.searchId(cellRef.mRefID);
            bool isLight = localIndex.second == CSMWorld::UniversalId::Type_Light;
            if ((isLight && cellRef.mChargeFloat < -1) || (!isLight && cellRef.mChargeInt < -1)) {
                std::string str = " has invalid charge ";
                if (localIndex.second == CSMWorld::UniversalId::Type_Light)
                    str += std::to_string(cellRef.mChargeFloat);
                else
                    str += std::to_string(cellRef.mChargeInt);
                messages.push_back(std::make_pair(id, id.getId() + str));
            }
        }
    }

    // If object have owner, check if that owner reference is valid
    if (!cellRef.mOwner.empty() && mReferencables.searchId(cellRef.mOwner) == -1)
        messages.push_back(std::make_pair(id, " has non existing owner " + cellRef.mOwner));

    // If object have creature soul trapped, check if that creature reference is valid
    if (!cellRef.mSoul.empty())
        if (mReferencables.searchId(cellRef.mSoul) == -1)
            messages.push_back(std::make_pair(id, " has non existing trapped soul " + cellRef.mSoul));

    bool hasFaction = !cellRef.mFaction.empty();

    // If object have faction, check if that faction reference is valid
    if (hasFaction)
        if (mFactions.searchId(cellRef.mFaction) == -1)
            messages.push_back(std::make_pair(id, " has non existing faction " + cellRef.mFaction));

    // Check item's faction rank
    if (hasFaction && cellRef.mFactionRank < -1)
        messages.push_back(std::make_pair(id, " has faction set but has invalid faction rank " + std::to_string(cellRef.mFactionRank)));
    else if (!hasFaction && cellRef.mFactionRank != -2)
        messages.push_back(std::make_pair(id, " has invalid faction rank " + std::to_string(cellRef.mFactionRank)));

    // If door have destination cell, check if that reference is valid
    if (!cellRef.mDestCell.empty())
        if (mCells.searchId(cellRef.mDestCell) == -1)
            messages.push_back(std::make_pair(id, " has non existing destination cell " + cellRef.mDestCell));

    // Check if scale isn't negative
    if (cellRef.mScale < 0)
    {
        std::string str = " has negative scale ";
        str += std::to_string(cellRef.mScale);
        messages.push_back(std::make_pair(id, id.getId() + str));
    }

    // Check if enchantement points aren't negative or are at full (-1)
    if (cellRef.mEnchantmentCharge < 0 && cellRef.mEnchantmentCharge != -1)
    {
        std::string str = " has negative enchantment points ";
        str += std::to_string(cellRef.mEnchantmentCharge);
        messages.push_back(std::make_pair(id, id.getId() + str));
    }

    // Check if gold value isn't negative
    if (cellRef.mGoldValue < 0)
    {
        std::string str = " has negative gold value ";
        str += cellRef.mGoldValue;
        messages.push_back(std::make_pair(id, id.getId() + str));
    }
}

int CSMTools::ReferenceCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mReferences.getSize();
}
