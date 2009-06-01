/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2009  Jacob Essex, Nicolay Korslund
  WWW: http://openmw.sourceforge.net/

  This file (cpp_generator.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

*/

typedef uint8_t ubyte;
typedef uint16_t ushort16;

struct GenLevelResult
{
private:
  bool isAlpha;

public:
  QuadHolder quad;
  PixelBox image;
  bool hasMesh;

  GenLevelResult()
  {
    image.data = NULL;
    isAlpha = false;
    hasMesh = false;
  }

  ~GenLevelResult()
  {
    if(!isEmpty())
      {
        // This takes care of both normal image data and alpha maps.
        free(image.data);

        if(hasMesh)
          free(quad.meshes[0].vertexBuffer);
      }
  }

  ubyte *allocAlphas(int width, int texNum)
  {
    assert(isEmpty() || hasMesh);
    image = PixelBox(width, width, texNum, Ogre::PF_A8);

    image.data = calloc(width*width*texNum, 1);
    isAlpha = true;

    // Set up the alpha images. TODO: We have to split these over
    // several meshes, but for now pretend that we're using only
    // one. This is going to be a bit messy... Perhaps having only
    // separate buffers is good enough, without using the pixel box at
    // all.
    assert(quad.meshes.size() == 1);
    quad.meshes[0].alphas.resize(texNum);

    for(int i=0;i<texNum;i++)
      {
        ubyte *ptr = (ubyte*)image.data;
        quad.meshes[0].alphas[i].buffer = ptr + width*width*i;
        quad.meshes[0].alphas[i].info.bufSize = width*width;
      }

    return (ubyte*)image.data;
  }

  void allocMesh(int size)
  {
    quad.meshes.resize(1);

    MeshHolder &mh = quad.meshes[0];
    MeshInfo &mi = mh.info;

    mi.vertRows = size;
    mi.vertCols = size;
    // 1 height, 3 normal components = 4 bytes per vertex. The reader
    // algorithm (fillVertexBuffer) needs 1 extra byte so add that
    // too.
    mi.vertBufSize = 4 * size*size + 1;
    mh.vertexBuffer = (char*)malloc(mi.vertBufSize);

    hasMesh = true;
  }

  void allocImage(int width)
  {
    assert(isEmpty());

    image = PixelBox(width, width, 1, Ogre::PF_R8G8B8);
    image.data = calloc(width*width*4, 1);

    assert(!hasAlpha());
  }

  bool hasAlpha()
  {
    assert(isAlpha == (quad.info.level==1));
    return isAlpha;
  }

  bool isEmpty()
  {
    return (image.data == NULL) && !hasMesh;
  }

  int alphaNum()
  {
    assert(hasAlpha());
    assert(quad.meshes.size() == 1);
    return quad.meshes[0].alphas.size();
  }

  int getAlphaTex(int alpha)
  {
    return quad.meshes[0].alphas[alpha].info.texName;
  }

  int setAlphaTex(int alpha, int index)
  {
    quad.meshes[0].alphas[alpha].info.texName = index;
  }

  // Resize the image
  void resize(int newSize)
  {
    assert(!isEmpty());
    assert(!hasAlpha());

    // Make sure we're scaling down, since we never gain anything by
    // scaling up.
    assert(newSize < image.getWidth());

    // Create a new pixel box to hold the data
    PixelBox nbox = PixelBox(newSize, newSize, 1, Ogre::PF_R8G8B8);
    nbox.data = malloc(newSize*newSize*4);

    // Resize the image. The nearest neighbour filter makes sure
    // there is no blurring.
    Image::scale(image, nbox, Ogre::Image::FILTER_NEAREST);

    // Kill and replace the old buffer
    free(image.data);
    image = nbox;
  }

  // TODO: Handle file naming internally
  void save(std::string name)
  {
    assert(!isEmpty());
    assert(!hasAlpha());

    std::cout << "  Creating " << name << "\n";

    // Create an image from the pixelbox data
    Image img;
    img.loadDynamicImage((ubyte*)image.data,
                         image.getWidth(), image.getHeight(),
                         image.format);

    // and save it
    img.save(name);
  }
};

// Copy from one pixel box to another
void copyBox(PixelBox &dst, PixelBox &src)
{
  // Some sanity checks first
  assert(src.format == dst.format);
  assert(src.data != NULL);
  assert(dst.data != NULL);

  // Memory size of one pixel
  int pixSize = Ogre::PixelUtil::getNumElemBytes(src.format);

  int fskip = src.rowPitch * pixSize;
  int tskip = dst.rowPitch * pixSize;
  int rows = src.getHeight();
  int rowSize = src.getWidth()*pixSize;

  assert(rows == dst.getHeight());
  assert(src.getWidth() == dst.getWidth());
  assert(dst.left == 0);
  assert(dst.top == 0);

  // Source and destination pointers
  ubyte *from = (ubyte*)src.data;
  ubyte *to = (ubyte*)dst.data;

  for(;rows>0;rows--)
    {
      memcpy(to, from, rowSize);
      to += tskip;
      from += fskip;
    }
}

class Generator
{
  // ESM data holder (will disappear soon)
  MWLand mMWLand;

  int mCount;

  // Texture sizes for the various levels. For the most detailed level
  // (level 1), this give the size of the alpha texture rather than
  // the actual final texture (There is no final texture for this
  // level, only alpha splatting.)
  std::vector<int> texSizes;

  // Default textures
  std::vector<GenLevelResult> defaults;

  CacheWriter cache;

public:
  Generator() :
    mCount(0) {}

  inline void addLandData(Record* record, const std::string& source)
  { mMWLand.addLandData(record, source); }

  inline void addLandTextureData(Record* record, const std::string& source)
  { mMWLand.addLandTextureData(record, source); }

  void generate(const std::string &filename)
  {
    TRACE("generate");

    cache.openFile(filename);

    // Find the maxiumum distance from (0,0) in any direction
    int max = 0;
    max = std::max<int>(mMWLand.getMaxX(), max);
    max = std::max<int>(mMWLand.getMaxY(), max);
    max = std::max<int>(-mMWLand.getMinX(), max);
    max = std::max<int>(-mMWLand.getMinY(), max);

    // Round up to nearest binary
    int depth=1;
    while(max)
      {
        max >>= 1;
        depth++;
        assert(depth <= 8);
      }
    max = 1 << depth-1;

    // We already know the answers
    assert(max == 32);
    assert(depth == 6);

    // Set the texture sizes. TODO: These should be config options,
    // perhaps - or maybe a result of some higher-level detail setting.
    texSizes.resize(depth+1, 0);
    texSizes[6] = 1024;
    texSizes[5] = 512;
    texSizes[4] = 256;
    texSizes[3] = 256;
    texSizes[2] = 256;
    texSizes[1] = 64;

    // Set some general parameters for the runtime
    cache.setParams(depth+1, texSizes[1]);

    // Create some common data first
    std::cout << "Generating common data\n";
    genDefaults();
    genIndexData();

    std::cout << "Generating quad data\n";
    // Start at one level above the top, but don't generate a mesh for
    // it
    GenLevelResult gen;
    genLevel(depth+1, -max, -max, gen, false);
    std::cout << "Writing index file\n";
    cache.finish();
    std::cout << "Pregeneration done. Results written to "
              << filename << "\n";
  }

  // Generates the default texture images "2_default.png" etc
  void genDefaults()
  {
    TRACE("genDefaults");

    int size = texSizes.size()-1;
    defaults.resize(size);

    for(int i=1; i<size; i++)
      defaults[i].quad.info.level = i;

    // Sending NULL as the first parameter tells the function to only
    // render the default background.
    assert(size > 2);
    genLevel2Map(NULL, defaults[2]);

    for(int i=3; i<size; i++)
      mergeMaps(NULL, defaults[i]);
  }

  // Generates common mesh information that's stored in the .index
  // file. This includes the x/y coordinates of meshes on each level,
  // the u/v texture coordinates, and the triangle index data.
  void genIndexData()
  {
    // Generate mesh data for each level.

    /*
      TODO: This data is very easy to generate, and we haven't really
      tested whether it's worth it to pregenerate it rather than to
      just calculate it at runtime. Unlike the index buffer below
      (which is just a memcpy at runtime, and definitely worth
      pregenerating), we have to loop through all the vertices at
      runtime anyway in order to splice this with the height
      data. It's possible that the additional memory use, pluss the
      slowdown (CPU-cache-wise) of reading from two buffers instead of
      one, makes it worthwhile to generate this data at runtime
      instead. However I guess the differences will be very small
      either way.
    */
    for(int lev=1; lev<=6; lev++)
      {
        // Make a new buffer to store the data
        int size = 65*65*4*sizeof(float);
        float *vertPtr = (float*)malloc(size);

        // Find the vertex separation for this level. The vertices are
        // 128 units apart in each cell, and for each level above that
        // we double the distance. This gives 128 * 2^(lev-1) =
        // 64*2^lev.
        const int vertSep = 64 << lev;

        // Loop over all the vertices in the mesh.
        for(int y=0; y<65; y++)
          for(int x=0; x<65; x++)
            {
              // X and Y
              *vertPtr++ = x*vertSep;
              *vertPtr++ = y*vertSep;

              // U and V (texture coordinates)
              const float u = x/64.0;
              const float v = y/64.0;
              assert(u>=0&&v>=0);
              assert(u<=1&&v<=1);
    
              *vertPtr++ = u;
              *vertPtr++ = v;
            }

        // Store the buffer
        cache.addVertexBuffer(lev,vertPtr,size);
      }

    // Next up, triangle indices
    int size = 64*64*6*sizeof(ushort16);
    ushort16 *indPtr = (ushort16*)malloc(size);

    bool flag = false;
    int indNum = 0;
    for ( int y = 0; y < 64; y++ )
      {
        for ( int x = 0; x < 64; x++ )
          {
            const int line1 = y*65 + x;
            const int line2 = (y+1)*65 + x;

            if ( flag )
              {
                *indPtr++ = line1;
                *indPtr++ = line2;
                *indPtr++ = line1 + 1;

                *indPtr++ = line1 + 1;
                *indPtr++ = line2;
                *indPtr++ = line2 + 1;
              }
            else
              {
                *indPtr++ = line1;
                *indPtr++ = line2;
                *indPtr++ = line2 + 1;

                *indPtr++ = line1;
                *indPtr++ = line2 + 1;
                *indPtr++ = line1 + 1;
              }
            flag = !flag; //flip tris for next time
            indNum+=6;
          }
        flag = !flag; //flip tries for next row
      }
    assert(indNum*2==size);

    // The index buffers are the same for all levels
    cache.addIndexBuffer(1,indPtr,size);
    cache.addIndexBuffer(2,indPtr,size);
    cache.addIndexBuffer(3,indPtr,size);
    cache.addIndexBuffer(4,indPtr,size);
    cache.addIndexBuffer(5,indPtr,size);
    cache.addIndexBuffer(6,indPtr,size);
  }

  void genLevel(int depth, int X, int Y, GenLevelResult &result,
                bool makeData = true)
  {
    TRACE("genLevel");

    result.quad.info.cellX = X;
    result.quad.info.cellY = Y;
    result.quad.info.level = depth;

    assert(result.isEmpty());

    if(depth == 1)
      {
        assert(makeData);

        if(!mMWLand.hasData(X,Y))
          // Oops, there's no data for this cell. Skip it.
          return;

        // Level 1 (most detailed) is handled differently from the
        // other leves.

        // The mesh is generated in pieces rather than as one part.
        genLevel1Meshes(result);

        // We also generate alpha maps instead of the actual textures.
        genCellAlpha(result);

        if(!result.isEmpty())
          {
            // Store the information we just created
            assert(result.hasAlpha());
            cache.writeQuad(result.quad);
          }

        return;
      }
    assert(depth > 1);

    // Number of cells in each sub-quad (not in this quad)
    int cells = 1 << (depth-2);

    // Call the sub-levels and store the result
    GenLevelResult sub[4];
    genLevel(depth-1, X, Y, sub[0]);             // NW
    genLevel(depth-1, X+cells, Y, sub[1]);       // NE
    genLevel(depth-1, X, Y+cells, sub[2]);       // SW
    genLevel(depth-1, X+cells, Y+cells, sub[3]); // SE

    // Mark the sub-quads that have data
    bool anyUsed = false;
    for(int i=0;i<4;i++)
      {
        bool used = !sub[i].isEmpty();
        result.quad.info.hasChild[i] = used;
        anyUsed = anyUsed || used;
      }

    if(!anyUsed)
      {
        // If our children are empty, then we are also empty.
        assert(result.isEmpty());
        return;
      }

    if(makeData)
      {
        // For depth==2, generate a new texture from the alphas.
        if(depth == 2)
          // Create the texture from the alpha maps
          genLevel2Map(sub, result);
        else
          // Merge the images from the previous levels
          mergeMaps(sub, result);

        // Create the landscape mesh
        createMesh(sub, result);
      }

    // Store the result
    cache.writeQuad(result.quad);
  }

  // Generate mesh data for one cell
  void genLevel1Meshes(GenLevelResult &res)
  {
    TRACE("genLevel1Meshes");

    const int intervals = 64;

    // Constants
    const int vertNum = intervals+1;
    const int vertSep = 128;

    // Allocate the mesh buffer
    res.allocMesh(vertNum);

    int cellX = res.quad.info.cellX;
    int cellY = res.quad.info.cellY;
    assert(res.quad.info.level==1);

    MeshHolder &mh = res.quad.meshes[0];
    MeshInfo &mi = mh.info;

    mi.worldWidth = vertSep*intervals;
    assert(mi.worldWidth == 8192);

    const VHGT &verts = *mMWLand.getHeights(cellX,cellY);
    const std::vector<char> &normals = mMWLand.getNormals(cellX,cellY);

    mi.heightOffset = verts.heightOffset;

    float max=-1000000.0;
    float min=1000000.0;

    char *vertPtr = mh.vertexBuffer;

    // Loop over all the vertices in the mesh. TODO: Mix this with the
    // function in MWLand/ESM that decodes the original data, and use
    // that directly.
    float rowheight = mi.heightOffset;
    float height;
    for(int y=0; y<65; y++)
      for(int x=0; x<65; x++)
        {
          int offs=y*65+x;

          // The vertex data from the ESM
          char data = verts.heightData[offs];

          // Write the height byte
          *vertPtr++ = data;

          // Calculate the height here, even though we don't store
          // it. We use it to find the min and max values.
          if(x == 0)
            {
              // Set the height to the row height
              height = rowheight;

              // First value in each row adjusts the row height
              rowheight += data;
            }
          // Adjust the height from the previous value
          height += data;

          // Calculate the min and max
          max = std::max(max, height);
          min = std::min(min, height);

          // Store the normals
          for(int k=0; k<3; k++)
            *vertPtr++ = normals[offs*3+k];
        }

    // Make sure we wrote exactly the right amount of data
    assert((vertPtr-mh.vertexBuffer)+1 ==
           mi.vertBufSize);

    // Store the min/max values
    mi.minHeight = min * 8;
    mi.maxHeight = max * 8;
  }

  // Create the mesh for this level.
  void createMesh(GenLevelResult *sub, GenLevelResult &res)
  {
    TRACE("createMesh");

    // How much to shift various numbers to the left at this level
    // (ie. multiply by 2^shift)
    const int shift = res.quad.info.level - 1;
    assert(shift >= 1);

    // Constants
    const int intervals = 64;
    const int vertNum = intervals+1;
    const int vertSep = 128 << shift;

    // Allocate the result buffer
    res.allocMesh(vertNum);

    MeshHolder &mh = res.quad.meshes[0];
    MeshInfo &mi = mh.info;

    mi.worldWidth = vertSep*intervals;
    char *vertPtr = mh.vertexBuffer;

    // Get the height from the first cell
    float rowheight;
    if(sub[0].isEmpty())
      rowheight = 0.0;
    else
      rowheight = sub[0].quad.meshes[0].info.heightOffset;

    // This is also the offset for the entire mesh
    mi.heightOffset = rowheight;

    // Loop through each 'row' of submeshes
    for(int subY=0; subY<2; subY++)
      {
        // Loop through each row of vertices
        for(int row=0; row<65; row++)
          {
            // Loop through both sub meshes, left and right
            for(int subX=0; subX<2; subX++)
              {
                GenLevelResult *s = &sub[subX+2*subY];

                // Check if we have any data
                if(!s->isEmpty() && 0)
                  {
                    const MeshHolder &smh = s->quad.meshes[0];
                    char* inPtr = smh.vertexBuffer;

                    // Loop through each vertex in this mesh. We skip two
                    // at a time.
                    for(int v=0; v<64; v+=2)
                      {
                        // Handle the v=0 case

                        // Count the height from the two next vertices
                        int data = *inPtr++;
                        inPtr++;inPtr++;inPtr++; // Skip the first normal
                        data += *inPtr++;

                        // Divide by two, since the result needs to fit in
                        // one byte. We compensate for this when we regen
                        // the mesh at runtime.
                        data >>= 1;
                        assert(data < 128 && data >= -128);

                        *vertPtr++ = data;

                        // Copy over the normal
                        *vertPtr++ = *inPtr++;
                        *vertPtr++ = *inPtr++;
                        *vertPtr++ = *inPtr++;
                      }
                    // Store the last one here. It _should_ be the
                    // same as the first in the next section, if
                    // present.
                  }
                else
                  {
                    // No data in this mesh. Just write zeros.
                    for(int v=0; v<32; v++)
                      {
                        // Height
                        *vertPtr++ = 0;

                        // Normal, pointing straight upwards
                        *vertPtr++ = 0;
                        *vertPtr++ = 0;
                        *vertPtr++ = 0x7f;
                      }
                  }
              }
          }
      }
    assert(vertPtr == mh.vertexBuffer + mi.vertBufSize);
  }

    // About segments:

    /* NOTES for the gen-phase: Was:
    // This is pretty messy. Btw: 128*16 == 2048 ==
    // CELL_WIDTH/4
    // 65 points across one cell means 64 intervals, and 17 points

    // means 16=64/4 intervals. So IOW the number of verts when
    // dividing by D is (65-1)/D + 1 = 64/D+1, which means that D
    // should divide 64, that is, be a power of two < 64.

      addNewObject(Ogre::Vector3(x*16*128, 0, y*16*128), //pos
                   17, //size
                   false, //skirts
                   0.25f, float(x)/4.0f, float(y)/4.0f);//quad seg location
    */

    /* This was also declared in the original code, you'll need it
       when creating the cache data

    size_t vw = mWidth; // mWidth is 17 or 65
    if ( mUseSkirts ) vw += 2; // skirts are used for level 2 and up
    vertCount=vw*vw;
    */

  /**
   * @brief fills the vertex buffer with data
   * @todo I don't think tex co-ords are right
  void calculateVertexValues()
  {
    int start = 0;
    int end = mWidth;

    if ( mUseSkirts )
      {
        --start;
        ++end;
      }

    for ( int y = start; y < end; y++ )
      for ( int x = start; x < end; x++ )
        {
        if ( y < 0 || y > (mWidth-1) || x < 0 || x > (mWidth-1) )
          {
            // These are the skirt vertices. 'Skirts' are simply a
            // wall at the edges of the mesh that goes straight down,
            // cutting off the posibility that you might see 'gaps'
            // between the meshes. Or at least I think that's the
            // intention.

            assert(mUseSkirts);

            // 1st coordinate
            if ( x < 0 )
              *verts++ = 0;
            else if ( x > (mWidth-1) )
              *verts++ = (mWidth-1)*getVertexSeperation();
            else
              *verts++ = x*getVertexSeperation();

            // 2nd coordinate
            *verts++ = -4096; //2048 below base sea floor height

            // 3rd coordinate
            if ( y < 0 )
              *verts++ = 0;
            else if ( y > (mWidth-1) )
              *verts++ = (mWidth-1)*getVertexSeperation();
            else
              *verts++ = y*getVertexSeperation();

            // No normals
            for ( Ogre::uint i = 0; i < 3; i++ )
              *verts++ = 0;

            // It shouldn't matter if these go over 1
            float u = (float)(x) / (mWidth-1);
            float v = (float)(y) / (mWidth-1);
            *verts++ = u;
            *verts++ = v;
          }
        else // Covered already

  void calculateIndexValues()
  {
    size_t vw = mWidth-1;
    if ( mUseSkirts ) vw += 2;

    const size_t indexCount = (vw)*(vw)*6;

    //need to manage allocation if not null
    assert(mIndices==0);

    // buffer was created here

    bool flag = false;
    Ogre::uint indNum = 0;
    for ( Ogre::uint y = 0; y < (vw); y+=1 ) {
      for ( Ogre::uint x = 0; x < (vw); x+=1 ) {

        const int line1 = y * (vw+1) + x;
        const int line2 = (y + 1) * (vw+1) + x;

        if ( flag ) {
          *indices++ = line1;
          *indices++ = line2;
          *indices++ = line1 + 1;

          *indices++ = line1 + 1;
          *indices++ = line2;
          *indices++ = line2 + 1;
        } else {
          *indices++ = line1;
          *indices++ = line2;
          *indices++ = line2 + 1;

          *indices++ = line1;
          *indices++ = line2 + 1;
          *indices++ = line1 + 1;
        }
        flag = !flag; //flip tris for next time

        indNum+=6;
      }
      flag = !flag; //flip tries for next row
    }
    assert(indNum==indexCount);
    //return mIndices;
  }
   */

  // Generate the alpha splatting bitmap for one cell.
  void genCellAlpha(GenLevelResult &res)
  {
    TRACE("genCellAlpha");

    const int cellX = res.quad.info.cellX;
    const int cellY = res.quad.info.cellY;
    assert(res.quad.info.level == 1);

    // Messy older code. We'll fix this later and read ESM data
    // directly from D code.
    std::string source = mMWLand.getSource(cellX, cellY);

    // List of texture indices for this cell. A cell has 16x16 texture
    // squares.
    int ltex[16][16];

    // A map from the global texture index to the local index for this
    // cell.
    typedef std::map<int, int> TSet;
    TSet textures;

    int local = 0; // Local index

    // Loop through all the textures in the cell and get the indices
    bool isDef = true;
    for(int ty = 0; ty < 16; ty++)
      for(int tx = 0; tx < 16; tx++)
        {
          // More messy code, to get the texture file name
          short texID =
            mMWLand.getLTEXIndex(cellX,cellY, tx, ty);
          std::string texturePath = "_land_default.dds";
          if ( texID != 0 && mMWLand.hasLTEXRecord(source,--texID) )
            {
              texturePath = mMWLand.getLTEXRecord(source,texID);
              isDef = false;
            }

          // Store the final index
          int index = cache.addTexture(texturePath);
          ltex[ty][tx] = index;

          // Add the index to the map
          if(textures.find(index) == textures.end())
            textures[index] = local++;
        }
    assert(local == textures.size());

    // If we still only found default textures, exit now.
    if(isDef)
      return;

    const int imageRes = texSizes[1];
    const int dataSize = imageRes*imageRes;
    const int texNum = textures.size();

    // Number of alpha pixels per texture square
    const int pps = imageRes/16;

    // Make sure there are at least as many alpha pixels as there are
    // textures
    assert(imageRes >= 16);
    assert(imageRes%16 == 0);
    assert(pps >= 1);
    assert(texNum >= 1);

    // Allocate the alpha images
    ubyte *uptr = res.allocAlphas(imageRes, texNum);

    assert(res.hasAlpha() && !res.isEmpty());

    // Write the indices to the result list
    for(TSet::iterator it = textures.begin();
        it != textures.end(); it++)
      res.setAlphaTex(it->second, it->first);

    // Loop over all textures again. This time, do alpha splatting.
    for(int ty = 0; ty < 16; ty++)
      for(int tx = 0; tx < 16; tx++)
        {
          // Get the local index for this square
          int index = textures[ltex[ty][tx]];

          // Get the offset of this square
          long offs = index*dataSize + pps*(ty*imageRes + tx);

          // FIXME: Make real splatting later. This is just
          // placeholder code.

          // Set alphas to full for this square
          for(int y=0; y<pps; y++)
            for(int x=0; x<pps; x++)
              {
                long toffs = offs + imageRes*y + x;
                assert(toffs < dataSize*texNum);
                uptr[toffs] = 255;
              }
        }
  }

  struct LTEXIndices
  {
    LTEXIndices()
    {
      for(int i=0;i<4;i++)
        inds[i] = -1;
    }

    int inds[4];
  };

  typedef std::map<int, LTEXIndices> LTexMap;

  // Generate a texture for level 2 from four alpha maps generated in
  // level 1.
  void genLevel2Map(GenLevelResult *maps, GenLevelResult &res)
  {
    TRACE("genLevel2Map");
    const int fromSize = texSizes[1];
    const int toSize = texSizes[2];

    // Create an overview of which texture is used where. The 'key' is
    // the global texture index, the 'value' is the corresponding
    // local indices in each of the four submaps.
    LTexMap lmap;

    if(maps != NULL) // NULL means only render default
    for(int mi=0;mi<4;mi++)
      {
        if(maps[mi].isEmpty())
          continue;

        assert(maps[mi].hasAlpha() &&
               maps[mi].image.getWidth() == fromSize);

        for(int ltex=0;ltex<maps[mi].alphaNum();ltex++)
          {
            // Global index for this texture
            int gIndex = maps[mi].getAlphaTex(ltex);

            // Store it in the map
            lmap[gIndex].inds[mi] = ltex;
          }
      }

    const float scale = TEX_SCALE/2;

    const std::string materialName
      ("MAT"+Ogre::StringConverter::toString(mCount++));

    // Get a new material
    Ogre::MaterialPtr mp = Ogre::MaterialManager::getSingleton().
      create(materialName,
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    // Put the default texture in the bottom 'layer', so that we don't
    // end up seeing through the landscape.
    Ogre::Pass* np = mp->getTechnique(0)->getPass(0);
    np->setLightingEnabled(false);
    np->createTextureUnitState("_land_default.dds")
      ->setTextureScale(scale,scale);

    // List of resources created
    std::list<Ogre::ResourcePtr> createdResources;

    // Loop through all our textures
    if(maps != NULL)
    for(LTexMap::iterator it = lmap.begin();
        it != lmap.end(); it++)
      {
        int gIndex = it->first;
        int *inds = it->second.inds;

        const std::string tn(cache.getString(gIndex));
        if ( tn == "_land_default.dds" )
          continue;

        // Create alpha map for this texture
        std::string alphaName(materialName + "_A_" + tn);
        Ogre::TexturePtr texPtr = Ogre::TextureManager::getSingleton().
          createManual(alphaName,
                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                       Ogre::TEX_TYPE_2D,
                       2*fromSize,2*fromSize,
                       1,0, // depth, mipmaps
                       Ogre::PF_A8, // One-channel alpha
                       Ogre::TU_STATIC_WRITE_ONLY);
        
        createdResources.push_back(texPtr);

        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texPtr->getBuffer();
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);

        // Fill in the alpha values
        memset(pDest, 0, 4*fromSize*fromSize);
        for(int i=0;i<4;i++)
          {
            // Does this sub-image have this texture?
            if(inds[i] == -1) continue;

            assert(!maps[i].isEmpty());

            // Find the right sub-texture in the alpha map
            ubyte *from = ((ubyte*)maps[i].image.data) +
              (fromSize*fromSize)*inds[i];

            // Find the right destination pointer
            int x = i%2;
            int y = i/2;
            ubyte *to = pDest + x*fromSize + y*fromSize*fromSize*2;

            // Copy the rows one by one
            for(int row = 0; row < fromSize; row++)
              {
                assert(to+fromSize <= pDest + 4*fromSize*fromSize);
                memcpy(to, from, fromSize);
                to += 2*fromSize;
                from += fromSize;
              }
          }

        // More Ogre-barf
        pixelBuffer->unlock();
        np = mp->getTechnique(0)->createPass();
        np->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        np->setLightingEnabled(false);
        np->setDepthFunction(Ogre::CMPF_EQUAL);

        Ogre::TextureUnitState* tus = np->createTextureUnitState(alphaName);
        tus->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

        tus->setAlphaOperation(	Ogre::LBX_BLEND_TEXTURE_ALPHA,
                                Ogre::LBS_TEXTURE,
                                Ogre::LBS_TEXTURE);
        tus->setColourOperationEx(	Ogre::LBX_BLEND_DIFFUSE_ALPHA,
                                        Ogre::LBS_TEXTURE,
                                        Ogre::LBS_TEXTURE);
        tus->setIsAlpha(true);

        tus = np->createTextureUnitState(tn);
        tus->setColourOperationEx(	Ogre::LBX_BLEND_DIFFUSE_ALPHA,
                                        Ogre::LBS_TEXTURE,
                                        Ogre::LBS_CURRENT);

        tus->setTextureScale(scale, scale);
      }

    Ogre::TexturePtr tex1 = getRenderedTexture(mp,materialName + "_T",
                                               toSize,Ogre::PF_R8G8B8);

    // Create the result buffer
    res.allocImage(toSize);

    // Blit the texture over
    tex1->getBuffer()->blitToMemory(res.image);

    // Clean up
    Ogre::MaterialManager::getSingleton().remove(mp->getHandle());
    Ogre::TextureManager::getSingleton().remove(tex1->getHandle());
    const std::list<Ogre::ResourcePtr>::const_iterator iend = createdResources.end();
    for ( std::list<Ogre::ResourcePtr>::const_iterator itr = createdResources.begin();
          itr != iend;
          ++itr) {
      (*itr)->getCreator()->remove((*itr)->getHandle());
    }

    // Output file name. TODO: The result structure can do this for us
    // now, it knows both the level and the cell coords. Figure out
    // what to do in the default case though.
    int X = res.quad.info.cellX;
    int Y = res.quad.info.cellY;
    std::string outname =
      "2_" + Ogre::StringConverter::toString(X) +
      "_"  + Ogre::StringConverter::toString(Y) +".png";

    // Override for the default image
    if(maps == NULL)
      outname = "2_default.png";

    outname = g_cacheDir + outname;

    // Save result
    res.save(outname);
  }

  void mergeMaps(GenLevelResult *maps, GenLevelResult &res)
  {
    TRACE("mergeMaps");

    const int level = res.quad.info.level;

    assert(texSizes.size() > level);
    assert(level > 2);
    const int fromSize = texSizes[level-1];
    const int toSize = texSizes[level];

    // Create a new image buffer large enough to hold the four
    // sub textures
    res.allocImage(fromSize*2);

    // Add the four sub-textures
    for(int mi=0;mi<4;mi++)
      {
        PixelBox src;

        // Use default texture if no source is present
        if(maps == NULL || maps[mi].isEmpty())
          src = defaults[level-1].image;
        else
          src = maps[mi].image;

        // Find the sub-part of the destination buffer to write to
        int x = (mi%2) * fromSize;
        int y = (mi/2) * fromSize;
        PixelBox dst = res.image.getSubVolume(Box(x,y,x+fromSize,y+fromSize));

        // Copy the image to the box
        copyBox(dst, src);
      }

    // Resize image if necessary
    if(toSize != 2*fromSize)
      res.resize(toSize);

    const int X = res.quad.info.cellX;
    const int Y = res.quad.info.cellY;

    // Texture file name
    std::string outname =
      Ogre::StringConverter::toString(level) + "_";

    if(maps == NULL)
      outname += "default.png";
    else
      outname +=
        Ogre::StringConverter::toString(X) + "_"  +
        Ogre::StringConverter::toString(Y) + ".png";

    outname = g_cacheDir + outname;

    // Save the image
    res.save(outname);
  }

  // Renders a material into a texture
  Ogre::TexturePtr getRenderedTexture(Ogre::MaterialPtr mp, const std::string& name,
                                      int texSize, Ogre::PixelFormat tt)
  {
    TRACE("getRenderedTexture");

    Ogre::CompositorPtr cp = Ogre::CompositorManager::getSingleton().
      create("Rtt_Comp",
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    //output pass
    Ogre::CompositionTargetPass* ctp = cp->createTechnique()->getOutputTargetPass();
    Ogre::CompositionPass* cpass = ctp->createPass();
    cpass->setType(Ogre::CompositionPass::PT_RENDERQUAD);
    cpass->setMaterial(mp);

    //create a texture to write the texture to...
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().
      createManual(
                   name,
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                   Ogre::TEX_TYPE_2D,
                   texSize,
                   texSize,
                   0,
                   tt,
                   Ogre::TU_RENDERTARGET
                   );

    Ogre::RenderTexture* renderTexture = texture->getBuffer()->getRenderTarget();
    Ogre::Viewport* vp = renderTexture->addViewport(mCamera);

    Ogre::CompositorManager::getSingleton().addCompositor(vp, "Rtt_Comp");
    Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp,"Rtt_Comp", true);

    renderTexture->update();

    // Call the OGRE renderer.
    Ogre::Root::getSingleton().renderOneFrame();

    Ogre::CompositorManager::getSingleton().removeCompositor(vp, "Rtt_Comp");
    Ogre::CompositorManager::getSingleton().remove(cp->getHandle());

    renderTexture->removeAllViewports();

    return texture;
  }
};
