#include "bodypartcheck.hpp"

#include "../prefs/state.hpp"

CSMTools::BodyPartCheckStage::BodyPartCheckStage(
        const CSMWorld::IdCollection<ESM::BodyPart> &bodyParts,
        const CSMWorld::Resources                   &meshes,
        const CSMWorld::IdCollection<ESM::Race>     &races ) :
    mBodyParts(bodyParts),
    mMeshes(meshes),
    mRaces(races)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::BodyPartCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mBodyParts.getSize();
}

void CSMTools::BodyPartCheckStage::perform (int stage, CSMDoc::Messages &messages)
{
    const CSMWorld::Record<ESM::BodyPart> &record = mBodyParts.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::BodyPart &bodyPart = record.get();

    CSMWorld::UniversalId id( CSMWorld::UniversalId::Type_BodyPart, bodyPart.mId );

    // Check BYDT
    if (bodyPart.mData.mPart >= ESM::BodyPart::MP_Count )
        messages.add(id, "Invalid part", "", CSMDoc::Message::Severity_Error);

    if (bodyPart.mData.mType > ESM::BodyPart::MT_Armor )
        messages.add(id, "Invalid type", "", CSMDoc::Message::Severity_Error);

    // Check MODL
    if ( bodyPart.mModel.empty() )
        messages.add(id, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if ( mMeshes.searchId( bodyPart.mModel ) == -1 )
        messages.add(id, "Model '" + bodyPart.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    // Check FNAM for skin body parts (for non-skin body parts it's meaningless)
    if ( bodyPart.mData.mType == ESM::BodyPart::MT_Skin )
    {
        if ( bodyPart.mRace.empty() )
            messages.add(id, "Race is missing", "", CSMDoc::Message::Severity_Error);
        else if ( mRaces.searchId( bodyPart.mRace ) == -1 )
            messages.add(id, "Race '" + bodyPart.mRace + "' does not exist", "", CSMDoc::Message::Severity_Error);
    }
}
