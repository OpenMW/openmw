#include "loadpgrd.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Pathgrid::load(ESMReader &esm)
{
    esm.getHNT(mData, "DATA", 12);
    mCell = esm.getHNString("NAME");

    // keep track of total connections so we can reserve edge vector size
    int edgeCount = 0;

    if (esm.isNextSub("PGRP"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        // Check that the sizes match up. Size = 16 * s2 (path points)
        if (size != static_cast<int> (sizeof(Point) * mData.mS2))
            esm.fail("Path point subrecord size mismatch");
        else
        {
            int pointCount = mData.mS2;
            mPoints.reserve(pointCount);
            for (int i = 0; i < pointCount; ++i)
            {
                Point p;
                esm.getExact(&p, sizeof(Point));
                mPoints.push_back(p);
                edgeCount += p.mConnectionNum;
            }
        }
    }

    if (esm.isNextSub("PGRC"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        if (size % sizeof(int) != 0)
            esm.fail("PGRC size not a multiple of 4");
        else
        {
            int rawConnNum = size / sizeof(int);
            std::vector<int> rawConnections;
            rawConnections.reserve(rawConnNum);
            for (int i = 0; i < rawConnNum; ++i)
            {
                int currentValue;
                esm.getT(currentValue);
                rawConnections.push_back(currentValue);
            }

            std::vector<int>::const_iterator rawIt = rawConnections.begin();
            int pointIndex = 0;
            mEdges.reserve(edgeCount);
            for(PointList::const_iterator it = mPoints.begin(); it != mPoints.end(); it++, pointIndex++)
            {
                unsigned char connectionNum = (*it).mConnectionNum;
                for (int i = 0; i < connectionNum; ++i) {
                    Edge edge;
                    edge.mV0 = pointIndex;
                    edge.mV1 = *rawIt;
                    rawIt++;
                    mEdges.push_back(edge);
                }
            }
        }
    }
}
void Pathgrid::save(ESMWriter &esm)
{
    esm.writeHNT("DATA", mData, 12);
    esm.writeHNCString("NAME", mCell);
    
    if (!mPoints.empty())
    {
        esm.startSubRecord("PGRP");
        for (PointList::iterator it = mPoints.begin(); it != mPoints.end(); ++it)
        {
            esm.writeT(*it);
        }
        esm.endRecord("PGRP");
    }
    
    if (!mEdges.empty())
    {
        esm.startSubRecord("PGRC");
        for (std::vector<Edge>::iterator it = mEdges.begin(); it != mEdges.end(); ++it)
        {
            esm.writeT(it->mV1);
        }
        esm.endRecord("PGRC");
    }
}

}
