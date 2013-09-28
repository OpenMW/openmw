#ifndef TERRAIN_TRIANGULATOR_H
#define TERRAIN_TRIANGULATOR_H

/*
  The triangulator is a simple math helper class, used for dividing a
  regular square grid into alternating set of triangles. It divides a
  grid like this:

  +----+----+
  |    |    | 
  |    |    |
  +----+----+
  |    |    |
  |    |    |
  +----+----+

  into this:

  +----+----+
  | \ 2|3 / |
  |1 \ | / 4|
  +----+----+
  |5 / | \ 8| 
  | / 6|7 \ |
  +----+----+

  Since the triangulation information is typically the same for all
  terrains of the same size, once instance can usually be shared.
*/

#include <cassert>

namespace Terrain
{
  // Index number type, number of grid cells (not vertices) in X and Y
  // directions.
  template <typename Index, int SizeX, int SizeY>
  class Triangulator
  {
    // Number of triangles
    static const int TriNum = SizeX * SizeY * 2;

    // 3 indices per triangle
    Index array[TriNum * 3];

  public:
    // Get raw triangle data pointer. Typically used for creating
    // meshes.
    const Index *getData() { return array; }

    // Return whether a given cell is divided as / (true) or \
    // (false).
    static bool cellType(int x, int y)
    {
      assert(x >= 0 && x < SizeX);
      assert(y >= 0 && y < SizeY);

      bool even = (x & 1) == 1;
      if((y & 1) == 1) even = !even;
      return even;
    }

    // Constructor sets up the index buffer
    Triangulator()
    {
      int index = 0;
      for ( int y = 0; y < SizeX; y++ )
        for ( int x = 0; x < SizeY; x++ )
          {
            // Get vertex indices
            Index line1 = y*(SizeX+1) + x;
            Index line2 = line1 + SizeX+1;

            if(cellType(x,y))
              {
                // Top left
                array[index++] = line1;
                array[index++] = line1 + 1;
                array[index++] = line2;

                // Bottom right
                array[index++] = line2;
                array[index++] = line1 + 1;
                array[index++] = line2 + 1;
              }
            else
              {
                // Bottom left
                array[index++] = line1;
                array[index++] = line2 + 1;
                array[index++] = line2;

                // Top right
                array[index++] = line1;
                array[index++] = line1 + 1;
                array[index++] = line2 + 1;
              }
          }
      assert(index == TriNum*3);
    }
  };
} // Namespace

#endif
