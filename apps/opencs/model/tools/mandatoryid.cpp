
#include "mandatoryid.hpp"

#include "../world/idcollection.hpp"

CSMTools::MandatoryIdStage::MandatoryIdStage (const CSMWorld::IdCollectionBase& idCollection,
    const CSMWorld::UniversalId& collectionId, const std::vector<std::string>& ids)
: mIdCollection (idCollection), mCollectionId (collectionId), mIds (ids)
{}

int CSMTools::MandatoryIdStage::setup()
{
    return mIds.size();
}

void CSMTools::MandatoryIdStage::perform (int stage, std::vector<std::string>& messages)
{
    if (mIdCollection.searchId (mIds.at (stage))==-1 ||
        mIdCollection.getRecord (mIds.at (stage)).isDeleted())
        messages.push_back (mCollectionId.toString() + "|Missing mandatory record: " + mIds.at (stage));
}