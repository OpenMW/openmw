#include "loadpgrd.hpp"

namespace ESM
{

PathGrid::~PathGrid() {
    if (points != NULL) {
        delete[] points;
        points = NULL;
    }
    if (edges != NULL) {
        delete[] edges;
        edges = NULL;
    }
}

void PathGrid::load(ESMReader &esm)
{
    esm.getHNT(data, "DATA", 12);
    cell = esm.getHNString("NAME");
    //std::cout << "loading PGRD for " << cell << " x=" << data.x << " y=" << data.y << std::endl;

    // Remember this file position
    context = esm.getContext();

    // Check that the sizes match up. Size = 16 * s2 (path points?)
    if (esm.isNextSub("PGRP"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        //std::cout << "PGRP size is " << size << std::endl;
        if (size != 16 * data.s2)
            esm.fail("Path grid table size mismatch");
        else
        {
            pointCount = data.s2;
            //std::cout << "Path grid points count is " << data.s2 << std::endl;
            points = new Point[pointCount];
            esm.getExact(points, size);
//            for (int i = 0; i < pointCount; ++i)
//            {
//                std::cout << i << "'s point: " << points[i].x;
//                std::cout << " " << points[i].y;
//                std::cout << " " << points[i].z;
//                std::cout << std::endl;
//            }
        }
    }

    // Size varies. Path grid chances? Connections? Multiples of 4
    // suggest either int or two shorts, or perhaps a float. Study
    // it later.
    if (esm.isNextSub("PGRC"))
    {
        esm.getSubHeader();
        int size = esm.getSubSize();
        //std::cout << "PGRC size is " << size << std::endl;
        if (size % 4 != 0)
            esm.fail("PGRC size not a multiple of 4");
        else
        {
            edgeCount = size / sizeof(Edge);
            //std::cout << "Path grid edge count is " << edgeCount << std::endl;
            edges = new Edge[edgeCount];
            esm.getExact(edges, size);
//            for (int i = 0; i < edgeCount; ++i)
//            {
//                std::cout << i << "'s edge: " << edges[i].v0 << " "
//                << edges[i].v1 << std::endl;
//            }
        }
    }
}

}
