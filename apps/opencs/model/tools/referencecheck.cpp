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

    // Check reference id
    if (cellRef.mRefID.empty())
        messages.add(id, "Instance is not based on an object", "", CSMDoc::Message::Severity_Error);
    else 
    {
        // Check for non existing referenced object
        if (mObjects.searchId(cellRef.mRefID) == -1)
            messages.add(id, "Instance of a non-existent object '" + cellRef.mRefID + "'", "", CSMDoc::Message::Severity_Error);
        else 
        {
            // Check if reference charge is valid for it's proper referenced type
            CSMWorld::RefIdData::LocalIndex localIndex = mDataSet.searchId(cellRef.mRefID);
            bool isLight = localIndex.second == CSMWorld::UniversalId::Type_Light;
            if ((isLight && cellRef.mChargeFloat < -1) || (!isLight && cellRef.mChargeInt < -1))
                messages.add(id, "Invalid charge", "", CSMDoc::Message::Severity_Error);
        }
    }

    // If object have owner, check if that owner reference is valid
    if (!cellRef.mOwner.empty() && mObjects.searchId(cellRef.mOwner) == -1)
        messages.add(id, "Owner object '" + cellRef.mOwner + "' does not exist", "", CSMDoc::Message::Severity_Error);

    // If object have creature soul trapped, check if that creature reference is valid
    if (!cellRef.mSoul.empty())
        if (mObjects.searchId(cellRef.mSoul) == -1)
            messages.add(id, "Trapped soul object '" + cellRef.mSoul + "' does not exist", "", CSMDoc::Message::Severity_Error);

    if (cellRef.mFaction.empty())
    {
        if (cellRef.mFactionRank != -2)
            messages.add(id, "Reference without a faction has a faction rank", "", CSMDoc::Message::Severity_Error);
    }
    else
    {
        if (mFactions.searchId(cellRef.mFaction) == -1)
            messages.add(id, "Faction '" + cellRef.mFaction + "' does not exist", "", CSMDoc::Message::Severity_Error);
        else if (cellRef.mFactionRank < -1)
            messages.add(id, "Invalid faction rank", "", CSMDoc::Message::Severity_Error);
    }

    if (!cellRef.mDestCell.empty() && mCells.searchId(cellRef.mDestCell) == -1)
        messages.add(id, "Destination cell '" + cellRef.mDestCell + "' does not exist", "", CSMDoc::Message::Severity_Error);

    if (cellRef.mScale < 0)
        messages.add(id, "Negative scale", "", CSMDoc::Message::Severity_Error);

    // Check if enchantement points aren't negative or are at full (-1)
    if (cellRef.mEnchantmentCharge < -1)
        messages.add(id, "Negative number of enchantment points", "", CSMDoc::Message::Severity_Error);

    // Check if gold value isn't negative
    if (cellRef.mGoldValue < 0)
        messages.add(id, "Negative gold value", "", CSMDoc::Message::Severity_Error);
}

int CSMTools::ReferenceCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mReferences.getSize();
}
