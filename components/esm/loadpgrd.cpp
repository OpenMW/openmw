#include "loadpgrd.hpp"

namespace ESM
{

void Pathgrid::load(ESMReader &esm)
{
    esm.getHNT(data, "DATA", 12);
    cell = esm.getHNString("NAME");

    // keep track of total connections so we can reserve edge vector size
    int edgeCount = 0;

    if (esm.isNextSub("PGRP"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        // Check that the sizes match up. Size = 16 * s2 (path points)
        if (size != static_cast<int> (sizeof(Point) * data.s2))
            esm.fail("Path point subrecord size mismatch");
        else
        {
            int pointCount = data.s2;
            points.reserve(pointCount);
            for (int i = 0; i < pointCount; ++i)
            {
                Point p;
                esm.getExact(&p, sizeof(Point));
                points.push_back(p);
                edgeCount += p.connectionNum;
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
            edges.reserve(edgeCount);
            for(PointList::const_iterator it = points.begin(); it != points.end(); it++, pointIndex++)
            {
                unsigned char connectionNum = (*it).connectionNum;
                for (int i = 0; i < connectionNum; ++i) {
                    Edge edge;
                    edge.v0 = pointIndex;
                    edge.v1 = *rawIt;
                    rawIt++;
                    edges.push_back(edge);
                }
            }
        }
    }
}
void Pathgrid::save(ESMWriter &esm)
{
    esm.writeHNT("DATA", data, 12);
    esm.writeHNString("NAME", cell);
    
    if (!points.empty())
    {
        esm.writeName("PGRP");
        for (PointList::iterator it = points.begin(); it != points.end(); ++it)
        {
            esm.writeT(*it);
        }
    }
    
    if (!edges.empty())
    {
        esm.writeName("PGRC");
        for (std::vector<Edge>::iterator it = edges.begin(); it != edges.end(); ++it)
        {
            esm.writeT(it->v1);
        }
    }
}

}
