#include "pathgridcheck.hpp"

#include <sstream>
#include <algorithm>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"
#include "../world/idcollection.hpp"
#include "../world/subcellcollection.hpp"
#include "../world/pathgrid.hpp"

CSMTools::PathgridCheckStage::PathgridCheckStage (const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids)
: mPathgrids (pathgrids)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::PathgridCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mPathgrids.getSize();
}

void CSMTools::PathgridCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Pathgrid>& record = mPathgrids.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const CSMWorld::Pathgrid& pathgrid = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Pathgrid, pathgrid.mId);

    // check the number of pathgrid points
    if (pathgrid.mData.mS2 < static_cast<int>(pathgrid.mPoints.size()))
        messages.add (id, "Less points than expected", "", CSMDoc::Message::Severity_Error);
    else if (pathgrid.mData.mS2 > static_cast<int>(pathgrid.mPoints.size()))
        messages.add (id, "More points than expected", "", CSMDoc::Message::Severity_Error);

    std::vector<CSMTools::Point> pointList(pathgrid.mPoints.size());
    std::vector<int> duplList;

    for (unsigned int i = 0; i < pathgrid.mEdges.size(); ++i)
    {
        if (pathgrid.mEdges[i].mV0 < static_cast<int>(pathgrid.mPoints.size()) && pathgrid.mEdges[i].mV0 >= 0)
        {
            pointList[pathgrid.mEdges[i].mV0].mConnectionNum++;
            // first check for duplicate edges
            unsigned int j = 0;
            for (; j < pointList[pathgrid.mEdges[i].mV0].mOtherIndex.size(); ++j)
            {
                if (pointList[pathgrid.mEdges[i].mV0].mOtherIndex[j] == pathgrid.mEdges[i].mV1)
                {
                    std::ostringstream ss;
                    ss << "Duplicate edge between points #" << pathgrid.mEdges[i].mV0 << " and #" << pathgrid.mEdges[i].mV1;
                    messages.add (id, ss.str(), "", CSMDoc::Message::Severity_Error);
                    break;
                }
            }

            // only add if not a duplicate
            if (j == pointList[pathgrid.mEdges[i].mV0].mOtherIndex.size())
                pointList[pathgrid.mEdges[i].mV0].mOtherIndex.push_back(pathgrid.mEdges[i].mV1);
        }
        else
        {
            std::ostringstream ss;
            ss << "An edge is connected to a non-existent point #" << pathgrid.mEdges[i].mV0;
            messages.add (id, ss.str(), "", CSMDoc::Message::Severity_Error);
        }
    }

    for (unsigned int i = 0; i < pathgrid.mPoints.size(); ++i)
    {
        // check that edges are bidirectional
        bool foundReverse = false;
        for (unsigned int j = 0; j < pointList[i].mOtherIndex.size(); ++j)
        {
            for (unsigned int k = 0; k < pointList[pointList[i].mOtherIndex[j]].mOtherIndex.size(); ++k)
            {
                if (pointList[pointList[i].mOtherIndex[j]].mOtherIndex[k] == static_cast<int>(i))
                {
                    foundReverse = true;
                    break;
                }
            }

            if (!foundReverse)
            {
                std::ostringstream ss;
                ss << "Missing edge between points #" << i << " and #" << pointList[i].mOtherIndex[j];
                messages.add (id, ss.str(), "", CSMDoc::Message::Severity_Error);
            }
        }

        // check duplicate points
        // FIXME: how to do this efficiently?
        for (unsigned int j = 0; j != i; ++j)
        {
            if (pathgrid.mPoints[i].mX == pathgrid.mPoints[j].mX &&
                pathgrid.mPoints[i].mY == pathgrid.mPoints[j].mY &&
                pathgrid.mPoints[i].mZ == pathgrid.mPoints[j].mZ)
            {
                std::vector<int>::const_iterator it = find(duplList.begin(), duplList.end(), static_cast<int>(i));
                if (it == duplList.end())
                {
                    std::ostringstream ss;
                    ss << "Point #" << i << " duplicates point #" << j
                    << " (" << pathgrid.mPoints[i].mX << ", " << pathgrid.mPoints[i].mY << ", " << pathgrid.mPoints[i].mZ << ")";
                    messages.add (id, ss.str(), "", CSMDoc::Message::Severity_Warning);

                    duplList.push_back(i);
                    break;
                }
            }
        }
    }

    // check pathgrid points that are not connected to anything
    for (unsigned int i = 0; i < pointList.size(); ++i)
    {
        if (pointList[i].mConnectionNum == 0)
        {
            std::ostringstream ss;
            ss << "Point #" << i << " ("
            << pathgrid.mPoints[i].mX << ", "
            << pathgrid.mPoints[i].mY << ", "
            << pathgrid.mPoints[i].mZ << ") is disconnected from other points";
            messages.add (id, ss.str(), "", CSMDoc::Message::Severity_Warning);
        }
    }

    // TODO: check whether there are disconnected graphs
}
