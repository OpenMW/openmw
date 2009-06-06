module terrain.esmland;

import esm.loadltex;
import esm.loadcell;
import util.regions;
import esm.filereader;

const int LAND_NUM_VERTS = 65*65;

MWLand mwland;

// Interface to the ESM landscape data
struct MWLand
{
  RegionManager reg;

  // These structs/types represent the way data is actually stored in
  // the ESM files.

  // Heightmap
  align(1)
  struct VHGT
  {
    float heightOffset;
    byte  heightData[LAND_NUM_VERTS];
    short unknown1;
    char unknown2;
  }

  // Normals
  typedef byte[LAND_NUM_VERTS*3] VNML;

  // Land textures. This is organized in 4x4 buffers of 4x4 squares
  // each. This is how the original engine splits up the cell meshes,
  // and it's probably a good idea for us to do the same.
  typedef short[4][4][4][4] VTEX;

  static assert(VHGT.sizeof == 4232);
  static assert(VNML.sizeof == 12675);
  static assert(VTEX.sizeof == 512);

  // Landscape data for one cell
  struct LandData
  {
    VHGT vhgt;
    VNML normals;
  }

  // Texture data for one cell
  struct LTEXData
  {
    // TODO: Store the source file here too, so we can get the list
    // from the right file. The source file is the same as the one we
    // load the landscape from in loadCell().
    VTEX vtex;

    // Get the texture x2,y2 from the sub map x1,x2
    char[] getTexture(int x1, int y1, int x2, int y2)
    {
      // Get the texture index relative to the current esm/esp file
      short texID = vtex[y1][x1][y2][x2];

      // Hack, will only work for Morrowind.esm. Fix this later.
      auto tl = landTextures.files["Morrowind.esm"];

      // Return the 'new' texture name. This name has automatically
      // been converted to .dds if the .tga file was not found.
      return tl[texID].getNewName();
    }

    // Get a texture from the 16x16 grid in one cell
    char[] getTexture(int x, int y)
    {
      return getTexture(x/4,y/4,x%4,y%4);
    }
  }

  // Get the maximum absolute coordinate value in any direction
  int getMaxCoord()
  { return cells.maxXY; }

  // Does the given cell exist and does it have land data?
  bool hasData(int x, int y)
  {
    // Does the cell exist?
    if(!cells.hasExt(x,y))
      return false;

    // And does it have terrain data?
    auto ex = cells.getExt(x,y);
    return ex.hasLand();
  }

  LandData *getLandData(int x, int y)
  {
    loadCell(x, y);
    return &currentLand;
  }

  LTEXData *getLTEXData(int x, int y)
  {
    loadCell(x, y);
    return &currentLtex;
  }

  private:

  int currentX = -1234;
  int currentY = 4321;

  LandData currentLand;
  LTEXData currentLtex;

  // Make sure the given cell is loaded
  void loadCell(int x, int y)
  {
    // If the right cell is already loaded, don't do anything
    if(x == currentX && y == currentY)
      return;

    assert(hasData(x,y));

    currentX = x;
    currentY = y;

    // Get the file context for the terrain data. This can be used to
    // skip to the right part of the ESM file.
    auto cont = cells.getExt(x,y).land.context;

    // We should use an existing region later, or at least delete this
    // once we're done with the gen process.
    if(reg is null)
      reg = new RegionManager();

    // Open the ESM at this cell
    esFile.restoreContext(cont, reg);

    // Store the data
    esFile.readHNExact(currentLand.normals.ptr,
                       currentLand.normals.length, "VNML");
    esFile.readHNExact(&currentLand.vhgt, VHGT.sizeof, "VHGT");

    // These aren't used yet
    if(esFile.isNextSub("WNAM")) esFile.skipHSubSize(81);
    if(esFile.isNextSub("VCLR")) esFile.skipHSubSize(12675);

    esFile.readHNExact(&currentLtex.vtex, VTEX.sizeof, "VTEX");
  }
}
