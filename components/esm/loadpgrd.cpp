#include "loadpgrd.hpp"

namespace ESM
{

void Pathgrid::load(ESMReader &esm)
{
    esm.getHNT(data, "DATA", 12);
    cell = esm.getHNString("NAME");
//    std::cout << "loading PGRD for " <<
//                 cell << " x=" << data.x << " y=" << data.y <<
//                 " " << data.s1
//                 << std::endl;

    // Check that the sizes match up. Size = 16 * s2 (path points)
    if (esm.isNextSub("PGRP"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        //std::cout << "PGRP size is " << size << std::endl;
        if (size != sizeof(Point) * data.s2)
            esm.fail("Path grid table size mismatch");
        else
        {
            int pointCount = data.s2;
            //std::cout << "Path grid points count is " << data.s2 << std::endl;
            points.reserve(pointCount);
            for (int i = 0; i < pointCount; ++i)
            {
                Point p;
                esm.getExact(&p, sizeof(Point));
                points.push_back(p);
            }
//            for (int i = 0; i < pointCount; ++i)
//            {
//                std::cout << i << "'s point: " << points[i].x;
//                std::cout << " " << points[i].y;
//                std::cout << " " << points[i].z;
//                std::cout << std::endl;
//            }
        }
    }

    if (esm.isNextSub("PGRC"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        //std::cout << "PGRC size is " << size << std::endl;
        if (size % sizeof(int) != 0)
            esm.fail("PGRC size not a multiple of 8");
        else
        {
            int edgeCount = size / sizeof(int) - 1;
            //std::cout << "Path grid edge count is " << edgeCount << std::endl;
            edges.reserve(edgeCount);
            int prevValue;
            esm.getT(prevValue);
            for (int i = 0; i < edgeCount; ++i)
            {
                int nextValue;
                esm.getT(nextValue);
                Edge e;
                e.v0 = prevValue;
                e.v1 = nextValue;
                edges.push_back(e);
                prevValue = nextValue;
            }
//            for (int i = 0; i < edgeCount; ++i)
//            {
//                std::cout << i << "'s edge: " << edges[i].v0 << " "
//                << edges[i].v1 << std::endl;
//            }
        }
    }
}

}
