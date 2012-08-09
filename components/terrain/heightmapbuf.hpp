#ifndef TERRAIN_HEIGHTMAPBUF_H
#define TERRAIN_HEIGHTMAPBUF_H

/*
  A HeightMap implementation that stores heigths in a buffer.
 */

#include "heightmap.hpp"
#include "land_factory.hpp"
#include <vector>
#include <cassert>

namespace Terrain
{
  class HeightMapBuffer : public HeightMap
  {
    std::vector<float> buf;

    float beginX, sizeX, endX;
    float beginY, sizeY, endY;

    int numX, numY;

  public:
    void load(LandDataPtr data, const LandInfo &info)
    {
      // We don't support other kinds of grid data yet.
      assert(info.grid == LGT_Quadratic);
      assert(info.data == LDT_Float);

      // Set up internal data
      beginX = info.xoffset;
      sizeX = info.xsize;
      endX = beginX+sizeX;
      numX = info.numx;

      beginY = info.yoffset;
      sizeY = info.ysize;
      endY = beginY+sizeY;
      numY = info.numy;

      // Prepare the buffer and load it
      buf.resize(numX*numY);

      data.read(&buf[0], buf.size()*sizeof(float));
    }

    // Functions inherited from HeightMap:

    float getHeight(int x, int y)
    {
      assert(x>=0 && x<numX);
      assert(y>=0 && y<numY);
      return getHeight(x + y*numX);
    }

    float getHeight(int index)
    {
      assert(index >= 0 && index < buf.size());
      return buf[index];
    }

    float getMinX() { return beginX; }
    float getMaxX() { return endX; }
    float getMinY() { return beginY; }
    float getMaxY() { return endY; }

    int getNumX() { return numX; }
    int getNumY() { return numY; }
  };
}
#endif
