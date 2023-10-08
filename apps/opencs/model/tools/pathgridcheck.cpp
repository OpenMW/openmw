#include "pathgridcheck.hpp"

#include <algorithm>
#include <memory>
#include <sstream>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm3/loadpgrd.hpp>

#include "../prefs/state.hpp"

#include "../world/idcollection.hpp"
#include "../world/pathgrid.hpp"
#include "../world/subcellcollection.hpp"
#include "../world/universalid.hpp"

CSMTools::PathgridCheckStage::PathgridCheckStage(const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids)
    : mPathgrids(pathgrids)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::PathgridCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mPathgrids.getSize();
}

void CSMTools::PathgridCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Pathgrid>& record = mPathgrids.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const CSMWorld::Pathgrid& pathgrid = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Pathgrid, pathgrid.mId);

    // check the number of pathgrid points
    if (pathgrid.mData.mPoints < pathgrid.mPoints.size())
        messages.add(id, "Less points than expected", "", CSMDoc::Message::Severity_Error);
    else if (pathgrid.mData.mPoints > pathgrid.mPoints.size())
        messages.add(id, "More points than expected", "", CSMDoc::Message::Severity_Error);

    std::vector<CSMTools::Point> pointList(pathgrid.mPoints.size());
    std::vector<size_t> duplList;

    for (const auto& edge : pathgrid.mEdges)
    {
        if (edge.mV0 < pathgrid.mPoints.size())
        {
            auto& point = pointList[edge.mV0];
            point.mConnectionNum++;
            // first check for duplicate edges
            size_t j = 0;
            for (; j < point.mOtherIndex.size(); ++j)
            {
                if (point.mOtherIndex[j] == edge.mV1)
                {
                    std::ostringstream ss;
                    ss << "Duplicate edge between points #" << edge.mV0 << " and #" << edge.mV1;
                    messages.add(id, ss.str(), {}, CSMDoc::Message::Severity_Error);
                    break;
                }
            }

            // only add if not a duplicate
            if (j == point.mOtherIndex.size())
                point.mOtherIndex.push_back(edge.mV1);
        }
        else
        {
            std::ostringstream ss;
            ss << "An edge is connected to a non-existent point #" << edge.mV0;
            messages.add(id, ss.str(), {}, CSMDoc::Message::Severity_Error);
        }
    }

    for (size_t i = 0; i < pathgrid.mPoints.size(); ++i)
    {
        // check that edges are bidirectional
        bool foundReverse = false;
        for (const auto& otherIndex : pointList[i].mOtherIndex)
        {
            for (const auto& other : pointList[otherIndex].mOtherIndex)
            {
                if (other == i)
                {
                    foundReverse = true;
                    break;
                }
            }

            if (!foundReverse)
            {
                std::ostringstream ss;
                ss << "Missing edge between points #" << i << " and #" << otherIndex;
                messages.add(id, ss.str(), {}, CSMDoc::Message::Severity_Error);
            }
        }

        // check duplicate points
        // FIXME: how to do this efficiently?
        for (size_t j = 0; j != i; ++j)
        {
            if (pathgrid.mPoints[i].mX == pathgrid.mPoints[j].mX && pathgrid.mPoints[i].mY == pathgrid.mPoints[j].mY
                && pathgrid.mPoints[i].mZ == pathgrid.mPoints[j].mZ)
            {
                auto it = std::find(duplList.begin(), duplList.end(), i);
                if (it == duplList.end())
                {
                    std::ostringstream ss;
                    ss << "Point #" << i << " duplicates point #" << j << " (" << pathgrid.mPoints[i].mX << ", "
                       << pathgrid.mPoints[i].mY << ", " << pathgrid.mPoints[i].mZ << ")";
                    messages.add(id, ss.str(), {}, CSMDoc::Message::Severity_Warning);

                    duplList.push_back(i);
                    break;
                }
            }
        }
    }

    // check pathgrid points that are not connected to anything
    for (size_t i = 0; i < pointList.size(); ++i)
    {
        if (pointList[i].mConnectionNum == 0)
        {
            std::ostringstream ss;
            ss << "Point #" << i << " (" << pathgrid.mPoints[i].mX << ", " << pathgrid.mPoints[i].mY << ", "
               << pathgrid.mPoints[i].mZ << ") is disconnected from other points";
            messages.add(id, ss.str(), {}, CSMDoc::Message::Severity_Warning);
        }
    }

    // TODO: check whether there are disconnected graphs
}
