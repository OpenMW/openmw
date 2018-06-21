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
    if (bodyPart.mData.mPart > 14 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has out of range part value." ));

    if (bodyPart.mData.mFlags > 3 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has out of range flags value." ));

    if (bodyPart.mData.mType > 2 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has out of range type value." ));

    // Check MODL

    if ( bodyPart.mModel.empty() )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has no model." ));
    else if ( mMeshes.searchId( bodyPart.mModel ) == -1 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has invalid model." ));

    // Check FNAM

    if ( bodyPart.mRace.empty() )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has no race." ));
    else if ( mRaces.searchId( bodyPart.mRace ) == -1 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has invalid race." ));
}
