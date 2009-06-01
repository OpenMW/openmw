/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2009  Nicolay Korslund
  WWW: http://openmw.sourceforge.net/

  This file (cpp_archive.cpp) is part of the OpenMW package.

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

// Info about the entire quad. TODO: Some of this (such as the texture
// scale and probably the width and radius) can be generated at
// loadtime and is common for all quads on the same level.
struct QuadInfo
{
  // Basic info
  int cellX, cellY;
  int level;

  // Bounding box info
  float minHeight, maxHeight;
  float worldWidth;
  float boundingRadius;

  // Texture scale for this quad
  float texScale;

  // True if we should make the given child
  bool hasChild[4];

  // Number of mesh segments in this quad
  int meshNum;

  // Location of this quad in the main archive file. The size includes
  // everything related to this quad, including mesh data, alpha maps,
  // etc.
  size_t offset, size;
};

struct MeshInfo;

const static int CACHE_MAGIC = 0x345AF815;

struct ArchiveHeader
{
  // "Magic" number to make sure we're actually reading an archive
  // file
  int magic;

  // Total number of quads in the archive
  int quads;

  // Level of the 'root' quad. There will only be one quad on this
  // level.
  int rootLevel;

  // Size of the alpha maps, in pixels along one side.
  int alphaSize;

  // Number of strings in the string table
  int stringNum;

  // Size of the string buffer
  size_t stringSize;
};

// This class handles the cached terrain data.
class TerrainArchive
{
public:
  MeshInfo *curMesh;
  QuadInfo *curQuad;

  QuadInfo *rootQuad;

  TerrainArchive()
    : mappedPtr(0),
      mappedSize(0),
      mmf(0),
      curMesh(0),
      curQuad(0),
      rootQuad(0) {}

  void openFile(const std::string &name)
  {
    mmf = new MmFile(name);

    // Read the index file first
    std::ifstream ifile((name+".index").c_str(), std::ios::binary);

    ArchiveHeader head;
    ifile.read((char*)&head, sizeof(head));

    // Sanity check
    assert(head.magic == CACHE_MAGIC);
    assert(head.quads > 0 && head.quads < 8192);

    // Store header info
    alphaSize = head.alphaSize;

    // Read all the quads
    quadList = new QuadInfo[head.quads];
    ifile.read((char*)quadList, sizeof(QuadInfo)*head.quads);

    // Create an index of all the quads
    for(int qn = 0; qn < head.quads; qn++)
      {
        int x = quadList[qn].cellX;
        int y = quadList[qn].cellY;
        int l = quadList[qn].level;

        assert(l >= 1);

        quadMap[l][x][y] = qn;

        // Store the root quad
        if(l == head.rootLevel)
          {
            assert(rootQuad == NULL);
            rootQuad = &quadList[qn];
          }
        else
          assert(l < head.rootLevel);
      }

    // Make sure the root was set
    assert(rootQuad != NULL);

    // Next read the string table
    stringBuf = new char[head.stringSize];
    stringOffsets = new int[head.stringNum];
    stringNum = head.stringNum;

    // First read the main string buffer
    ifile.read(stringBuf, head.stringSize);

    // Then read the string offsets
    ifile.read((char*)stringOffsets, stringNum*sizeof(int));

    // Read the vertex buffer data
    int bufNum = head.rootLevel;
    assert(bufNum == 7);
    vertBufData.resize(bufNum);
    indexBufData.resize(bufNum);

    // Fill the buffers. Start at level 1.
    for(int i=1;i<bufNum;i++)
      {
        int size;
        void *ptr;

        // Vertex buffer
        ifile.read((char*)size, sizeof(int));
        ptr = malloc(size);
        vertBufData[i] = ptr;
        ifile.read((char*)ptr, size);

        // Index buffer
        ifile.read((char*)size, sizeof(int));
        ptr = malloc(size);
        indexBufData[i] = ptr;
        ifile.read((char*)ptr, size);
      }
  }

  // Get info about a given quad from the index.
  QuadInfo *getQuad(int X, int Y, int level)
  {
    // FIXME: First check if the quad exists. This function should
    // FAIL if that is not the case.
    int ind = quadMap[level][X][Y];
    QuadInfo *res = &quadList[ind];
    assert(res);
    return res;
  }

  // Maps the terrain and material info for a given quad into
  // memory. This is typically called right before the meshes are
  // created.
  void mapQuad(QuadInfo *info)
  {
    assert(info);

    // Store the quad for later
    curQuad = info;

    doMap(info->offset, info->size);
  }

  // Get the info struct for a given segment. Remembers the MeshInfo
  // for all later calls.
  MeshInfo *getMeshInfo(int segNum);

  float *getVertexBuffer(int level)
  {
    assert(level>=1 && level<vertBufData.size());
    return (float*)vertBufData[level];
  }

  unsigned short *getIndexBuffer(int level, int &size)
  {
    assert(level>=1 && level<indexBufData.size());
    return (unsigned short*)indexBufData[level];
  }

private:
  // All quad headers (from the index) are stored in this list
  QuadInfo *quadList;

  // A map of all quads. Contain indices to the above array. Indexed
  // by [level][X][Y].
  std::map<int,std::map<int,std::map<int,int> > > quadMap;

  // These contain pregenerated mesh data that is common for all
  // meshes on a given level.
  std::vector<void*> vertBufData;
  std::vector<void*> indexBufData;

  // Used for the mmapped file
  MmFile *mmf;
  uint8_t *mappedPtr;
  size_t mappedSize;

  // Stores the string table
  char *stringBuf;
  int *stringOffsets;
  int stringNum;

  // Texture size of the alpha maps
  int alphaSize;

  const char *getString(int index)
  {
    assert(index >= 0);
    assert(index < stringNum);

    return stringBuf + stringOffsets[index];
  }

  void doMap(size_t offset, size_t size)
  {
    assert(mmf != NULL);
    mappedPtr = (uint8_t*)mmf->map(offset, size);
    mappedSize = size;
    assert(mappedPtr != NULL);
  }

  // Get the pointer to a given buffer in the mapped window. The
  // offset is relative to the start of the window, and the size must
  // fit inside the window.
  void *getRelPtr(size_t offset, size_t size)
  {
    assert(mappedPtr != NULL);

    // Don't overstep the mapped data
    assert(offset + size <= mappedSize);

    return mappedPtr + offset;
  }

  // Copy a given buffer from the file. The buffer might be a
  // compressed stream, so it's important that the buffers are written
  // the same as they are read. (Ie. you can't write a buffer as one
  // operation and read it as two, or vice versa. Also, buffers cannot
  // overlap.) The offset is relative to the current mapped file
  // window.
  void copy(void *dest, size_t offset, size_t inSize)
  {
    const void *source = getRelPtr(offset, inSize);

    // Just copy it for now
    memcpy(dest, source, inSize);
  }

  friend struct MeshInfo;
  friend struct AlphaInfo;
} g_archive;

// Info about an alpha map belonging to a mesh
struct AlphaInfo
{
  size_t bufSize, bufOffset;

  // The texture name for this layer. The actual string is stored in
  // the archive's string buffer.
  int texName;
  int alphaName;

  // Fill the alpha texture buffer
  void fillAlphaBuffer(uint8_t *abuf) const
  {
    g_archive.copy(abuf, bufOffset, bufSize);
  }

  // Get the texture for this alpha layer
  const char *getTexName() const
  {
    return g_archive.getString(texName);
  }

  // Get the material name to give the alpha texture
  const char *getAlphaName() const
  {
    return g_archive.getString(alphaName);
  }
};

// Info about each submesh
struct MeshInfo
{
  // Bounding box info
  float minHeight, maxHeight;
  float worldWidth;

  // Vertex and index numbers
  int vertRows, vertCols;
  int indexCount;

  // Scene node position (relative to the parent node)
  float x, y;

  // Height offset to apply to all vertices
  float heightOffset;

  // Size and offset of the vertex buffer
  size_t vertBufSize, vertBufOffset;

  // Number and offset of AlphaInfo blocks
  int alphaNum;
  size_t alphaOffset;

  // Texture name. Index to the string table.
  int texName;

  // Fill the given vertex buffer
  void fillVertexBuffer(float *vbuf) const
  {
    //g_archive.copy(vbuf, vertBufOffset, vertBufSize);

    // The height map and normals from the archive
    char *hmap = (char*)g_archive.getRelPtr(vertBufOffset, vertBufSize);

    int level = getLevel();

    // The generic part, containing the x,y coordinates and the uv
    // maps.
    float *gmap = g_archive.getVertexBuffer(level);

    // Calculate the factor to multiply each height value with. The
    // heights are very limited in range as they are stored in a
    // single byte. Normal MW data uses a factor of 8, but we have to
    // double this for each successive level since we're splicing
    // several vertices together and need to allow larger differences
    // for each vertex. The formula is 8*2^(level-1).
    float scale = 4.0 * (1<<level);

    // Merge the two data sets together into the output buffer.
    float offset = heightOffset;
    for(int y=0; y<vertRows; y++)
      {
        // The offset for the entire row is determined by the first
        // height value. All the values in a row gives the height
        // relative to the previous value, and the first value in each
        // row is relative to the first value in the previous row.
        offset += *hmap;

        // This is the 'sliding offset' for this row. It's adjusted
        // for each vertex that's added, but only affects this row.
        float rowofs = offset;
        for(int x=0; x<vertCols; x++)
          {
            hmap++; // Skip the byte we just read

            // X and Y from the pregenerated buffer
            *vbuf++ = *gmap++;
            *vbuf++ = *gmap++;

            // The height is calculated from the current offset
            *vbuf++ = rowofs * scale;

            // Normal vector.
            // TODO: Normalize?
            *vbuf++ = *hmap++;
            *vbuf++ = *hmap++;
            *vbuf++ = *hmap++;

            // UV
            *vbuf++ = *gmap++;
            *vbuf++ = *gmap++;

            // Adjust the offset for the next vertex. On the last
            // iteration this will read past the current row, but
            // that's OK since rowofs is discarded afterwards.
            rowofs += *hmap;
          }
      }
  }

  // Fill the index buffer
  void fillIndexBuffer(unsigned short *ibuf) const
  {
    // The index buffer is pregenerated. It is identical for all
    // meshes on the same level, so just copy it over.
    int size;
    unsigned short *generic = g_archive.getIndexBuffer(getLevel(), size);
    memcpy(ibuf, generic, size);
  }

  int getLevel() const
  {
    assert(g_archive.curQuad);
    return g_archive.curQuad->level;
  }

  // Get an alpha map belonging to this mesh
  AlphaInfo *getAlphaInfo(int num) const
  {
    assert(num < alphaNum && num >= 0);
    assert(getLevel() == 1);
    AlphaInfo *res = (AlphaInfo*)g_archive.getRelPtr
      (alphaOffset, alphaNum*sizeof(AlphaInfo));
    res += num;
    return res;
  }

  // Get the size of the alpha textures (in pixels).
  int getAlphaSize() const
  { return g_archive.alphaSize; }

  // Get the texture and material name to use for this mesh.
  const char *getTexName() const
  { return g_archive.getString(texName); }

  float getTexScale() const
  { return g_archive.curQuad->texScale; }

  const char *getBackgroundTex() const
  { return "_land_default.dds"; }
};

MeshInfo *TerrainArchive::getMeshInfo(int segNum)
{
  assert(curQuad);
  assert(segNum < curQuad->meshNum);

  // The mesh headers are at the beginning of the mapped segment.
  curMesh = (MeshInfo*) getRelPtr(0, sizeof(MeshInfo)*curQuad->meshNum);
  curMesh += segNum;

  return curMesh;
}
