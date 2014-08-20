#include "bodypartcheck.hpp"

CSMTools::BodyPartCheckStage::BodyPartCheckStage(
        const CSMWorld::IdCollection<ESM::BodyPart> &bodyParts,
        const CSMWorld::Resources                   &meshes,
        const CSMWorld::IdCollection<ESM::Race>     &races ) :
    mBodyParts(bodyParts),
    mMeshes(meshes),
    mRaces(races)
{ }

int CSMTools::BodyPartCheckStage::setup()
{
    return mBodyParts.getSize();
}

void CSMTools::BodyPartCheckStage::perform ( int stage, Messages &messages )
{
    const CSMWorld::Record<ESM::BodyPart> &record = mBodyParts.getRecord(stage);

    if ( record.isDeleted() )
        return;

    const ESM::BodyPart &bodyPart = record.get();

    CSMWorld::UniversalId id( CSMWorld::UniversalId::Type_BodyPart, bodyPart.mId );

    // Check BYDT

    if ( bodyPart.mData.mPart < 0 || bodyPart.mData.mPart > 14 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has out of range \"Part\" property." ));

    if ( bodyPart.mData.mVampire < 0 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has negative \"Vampire\" property." ));

    if ( bodyPart.mData.mFlags < 0 || bodyPart.mData.mFlags > 2 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has out of range \"Flags\" property." ));

    if ( bodyPart.mData.mType < 0 || bodyPart.mData.mType > 2 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has out of range \"Type\" property." ));

    // Check MODL

    if ( bodyPart.mModel.empty() )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has no model." ));
    else if ( mMeshes.searchId( bodyPart.mModel ) == -1 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has unreferenced model." ));

    // Check FNAM

    if ( bodyPart.mRace.empty() )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has no race." ));
    else if ( mRaces.searchId( bodyPart.mRace ) == -1 )
        messages.push_back(std::make_pair( id, bodyPart.mId + " has unreferenced race." ));
}