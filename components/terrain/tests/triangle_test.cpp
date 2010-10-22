#include <iostream>
using namespace std;

#include "../triangulator.hpp"

const int X = 4;
const int Y = 4;

typedef Terrain::Triangulator<short,X,Y> Triangles4x4;

int main()
{
  Triangles4x4 t;

  cout << "Cell types:\n";
  for(int y=0;y<Y;y++)
    {
      for(int x=0;x<X;x++)
        {
          if(t.cellType(x,y)) cout << "/ ";
          else cout << "\\ ";
        }
      cout << endl;
    }
  cout << endl;

  cout << "Full index list:\n";
  for(int i=0; i<X*Y*3; i++)
    cout << t.getData()[i] << endl;

  return 0;
}

/* Code we might add later:

    // Get the vertex indices belonging to a given triangle
    void getTriangle(int trinum, Index &p1, Index &p2, Index &p3)
    {
      assert(trinum >= 0 && trinum < TriNum);
      trinum *= 3;

      p1 = array[trinum++];
      p2 = array[trinum++];
      p3 = array[trinum];
    }

    /*
      Get height interpolation weights for a given grid square. The
      input is the grid square number (x,y) and the relative position
      within that square (xrel,yrel = [0.0..1.0].) The weights are
      returned as three vertex index + weight factor pairs.

      A more user-friendly version for HeightMap structs is given
      below.
    * /
    void getWeights(int x, int y, float xrel, float yrel,
                    Index &p1, float w1,
                    Index &p2, float w2,
                    Index &p3, float w3)
    {
      // Find cell index
      int index = y*SizeX + x;

      // First triangle in cell
      index *= 2;

      // The rest depends on how the cell is triangulated
      if(cellType(x,y))
        {
        }
      else
        {
          // Cell is divided as \ from 0,0 to 1,1
          if(xrel < yrel)
            {
              // Bottom left triangle.

              // Order is (0,0),(1,1),(0,1).
              getTriangle(index, p1,p2,p3);

              
            }
          else
            {
              // Top right triangle

              // Order is (0,0),(1,0),(1,1).
              getTriangle(index+1, p1,p2,p3);
            }
        }
    }

 */
