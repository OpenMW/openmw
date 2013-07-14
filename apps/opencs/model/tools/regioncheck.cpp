
#include "regioncheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadregn.hpp>

#include "../world/universalid.hpp"

CSMTools::RegionCheckStage::RegionCheckStage (const CSMWorld::IdCollection<ESM::Region>& regions)
: mRegions (regions)
{}

int CSMTools::RegionCheckStage::setup()
{
    return mRegions.getSize();
}

void CSMTools::RegionCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    const ESM::Region& region = mRegions.getRecord (stage).get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Region, region.mId);

    // test for empty name
    if (region.mName.empty())
        messages.push_back (id.toString() + "|" + region.mId + " has an empty name");

    /// \todo test that the ID in mSleeplist exists

    /// \todo check data members that can't be edited in the table view
}