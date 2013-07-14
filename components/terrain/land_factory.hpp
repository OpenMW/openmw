#ifndef TERRAIN_LAND_FACTORY_H
#define TERRAIN_LAND_FACTORY_H

namespace Terrain
{
  enum LandInfoGridType
    {
      LGT_Quadratic
    };

  enum LandInfoDataType
    {
      LDT_Float
    };

  struct LandInfo
  {
    // Type information
    LandInfoGridType grid;
    LandInfoDataType data;

    // Landscape size and number of vertices. Note that xsize and
    // ysize may be negative, signaling a flipped landscape in that
    // direction.
    float xsize, ysize;
    int numx, numy;

    // World offset along the same x/y axes. Whether these are set or
    // used depends on the client implementation.
    float xoffset, yoffset;
  };

  /*
    Factory class that provides streams to land data cells. Each
    "cell" has a unique integer coordinate in the plane.
  */
  struct LandFactory
  {
    // True if this factory has any data for the given grid cell.
    virtual bool has(int x, int y) = 0;
  };
}
#endif
