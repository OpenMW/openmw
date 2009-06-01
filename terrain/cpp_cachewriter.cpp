/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2009  Nicolay Korslund
  WWW: http://openmw.sourceforge.net/

  This file (cpp_cachewriter.cpp) is part of the OpenMW package.

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

// Helper structs
struct AlphaHolder
{
  AlphaInfo info;

  // Actual pixel buffer
  unsigned char *buffer;

  // Texture name and alpha material name to use
  //std::string texName, alphaName;
};

struct MeshHolder
{
  MeshInfo info;

  // Actual buffers
  char *vertexBuffer;

  // Texture name
  std::string texName;

  // Alpha maps (if any)
  std::vector<AlphaHolder> alphas;
};

// A struct that gathers all the relevant quad data in one place.
struct QuadHolder
{
  QuadInfo info;

  std::vector<MeshHolder> meshes;
};

class CacheWriter
{
public:
  // Opens the main archive file for output
  void openFile(const std::string &fname)
  {
    mainFile.open(fname.c_str(), std::ios::binary);
    iname = fname + ".index";
    fileOffset = 0;
    totalStringLength = 0;
  }

  void setParams(int mxLev, int alphSize)
  {
    maxLevel = mxLev;
    alphaSize = alphSize;

    vertBufData.resize(maxLevel);
    indexBufData.resize(maxLevel);
    vertBufSize.resize(maxLevel);
    indexBufSize.resize(maxLevel);
  }

  // Closes the main archive file and writes the index.
  void finish()
  {
    mainFile.close();

    // Write the index file
    std::ofstream ofile(iname.c_str(), std::ios::binary);

    // Header first
    ArchiveHeader head;
    head.magic = CACHE_MAGIC;
    head.quads = quadList.size();
    head.rootLevel = maxLevel;
    head.alphaSize = alphaSize;
    head.stringNum = stringList.size();
    head.stringSize = totalStringLength;
    ofile.write((char*)&head, sizeof(head));

    // Write the quads
    for(QuadList::iterator it = quadList.begin();
        it != quadList.end(); it++)
      {
        QuadInfo qi = *it;
        ofile.write((char*)&qi, sizeof(QuadInfo));
      }

    // String table next. We need to sort it in order of the indices
    // first.
    std::vector<std::string> strVector;
    strVector.resize(head.stringNum);

    for(StringList::iterator it = stringList.begin();
        it != stringList.end(); it++)
      {
        strVector[it->second] = it->first;
      }

    // Next, write the strings to file while we fill inn the offset
    // list
    std::vector<int> offsets;
    offsets.resize(head.stringNum);
    size_t curOffs = 0;

    for(int i=0; i<head.stringNum; i++)
      {
        // Add one byte for the zero terminator
        int len = strVector[i].length() + 1;
        const char *ptr = strVector[i].c_str();
        assert(ptr[len-1] == 0);

        ofile.write(ptr, len);

        // Store the offset
        offsets[i] = curOffs;
        curOffs += len;
      }
    // At the end the offset should match the buffer size we set in
    // the header.
    assert(curOffs == head.stringSize);

    // Finally, write the offset table itself
    for(int i=0; i<head.stringNum; i++)
      {
        int offs = offsets[i];
        ofile.write((char*)&offs, sizeof(int));
      }

    for(int i=1;i<maxLevel;i++)
      {
        int size;
        void *ptr;

        // Write vertex buffer
        size = vertBufSize[i];
        ptr = vertBufData[i];
        ofile.write((char*)&size, sizeof(int));
        ofile.write((char*)ptr, size);

        // Then the index buffer
        size = indexBufSize[i];
        ptr = indexBufData[i];
        ofile.write((char*)&size, sizeof(int));
        ofile.write((char*)ptr, size);
      }
  }

  // Add a common vertex buffer for a given level
  void addVertexBuffer(int level, void *ptr, int size)
  {
    assert(vertBufData.size() > level);

    vertBufData[level] = ptr;
    vertBufSize[level] = size;
  }

  // Add a common vertex buffer for a given level
  void addIndexBuffer(int level, void *ptr, int size)
  {
    assert(indexBufData.size() > level);

    indexBufData[level] = ptr;
    indexBufSize[level] = size;
  }

  // Write a finished quad to the archive file. All the offsets and
  // numbers in the *Info structs are filled in automatically based on
  // the additional data in the Holder structs.
  void writeQuad(const QuadHolder &qh)
  {
    TRACE("writeQuad");

    // See util/outbuffer.h
    OutBuffer buf;

    // Write the MeshInfo's first
    int meshNum = qh.meshes.size();

    MeshInfo *meshes = buf.write<MeshInfo>(meshNum);

    // Then write the mesh data in approximately the order it's read
    for(int i=0; i<meshNum; i++)
      {
        assert(meshes != NULL);

        const MeshHolder &mh = qh.meshes[i];

        // Copy the basic data first
        meshes[i] = mh.info;

        // Set everything else except the offsets
        int alphaNum = mh.alphas.size();
        meshes[i].alphaNum = alphaNum;
        //meshes[i].texName = addString(mh.texName);

        // Write the vertex buffer
        meshes[i].vertBufOffset = buf.size();
        writeBuf(buf, mh.vertexBuffer, meshes[i].vertBufSize);

        // Next write the alpha maps, if any
        meshes[i].alphaOffset = buf.size();
        AlphaInfo *ai = buf.write<AlphaInfo>(alphaNum);

        // Loop through the alpha maps
        for(int k=0; k<alphaNum; k++)
          {
            AlphaHolder ah = mh.alphas[k];
            ai[k] = ah.info;

            // Convert the strings
            // KILLME
            //ai[k].texName = addString(ah.texName);
            //ai[k].alphaName = addString(ah.alphaName);
            
            // Write the alpha pixel buffer
            ai[k].bufOffset = buf.size();
            writeBuf(buf, ah.buffer, ai[k].bufSize);
          }
      }
    // The quad cache is done, write it to file
    mainFile << buf;

    // Finally set up the QuadInfo itself
    QuadInfo qi;

    // Basic info
    qi = qh.info;

    // Derived info
    qi.meshNum = meshNum;
    qi.size = buf.size();
    qi.offset = fileOffset;

    // Update the main offset
    fileOffset += qi.size;

    // Add the quad to the index list
    quadList.push_back(qi);

    std::cout << "end\n";
  }

  // Add a texture name as a string. Will convert .tga file names to
  // .dds as a convenience
  int addTexture(const std::string &orig)
  {
    size_t d = orig.find_last_of(".") + 1;
    return addString(orig.substr(0, d) + "dds");
  }

  // Convert a string to an index
  int addString(const std::string &str)
  {
    // Do we already have the string?
    StringList::iterator it = stringList.find(str);
    if(it != stringList.end())
      return it->second;

    // Nope, insert it
    int index = stringList.size();
    stringList[str] = index;
    stringLookup[index] = str;

    // Sum up the string lengths + 1 byte for the zero
    totalStringLength += str.length() + 1;

    return index;
  }

  const std::string &getString(int index)
  {
    const std::string &res = stringLookup[index];
    assert(stringList[res] == index);
    return res;
  }

private:
  // Write the given block of memory to 'buf', possibly compressing
  // the data.
  void writeBuf(OutBuffer &buf, const void *ptr, size_t size)
  {
    // Reserve the maximum bytes needed.
    void *toPtr = buf.reserve(size);

    // Store the data
    memcpy(toPtr, ptr, size);

    // Add the actual number of bytes stored
    buf.add(size);
  }

  std::vector<void*> vertBufData;
  std::vector<void*> indexBufData;
  std::vector<int> vertBufSize;
  std::vector<int> indexBufSize;

  // Variables that must be set during the gen phase
  int maxLevel;
  int alphaSize;

  // Contains a unique index for each string
  typedef std::map<std::string, int> StringList;
  StringList stringList;
  std::map<int, std::string> stringLookup;
  size_t totalStringLength;

  // List of all quads
  typedef std::list<QuadInfo> QuadList;
  QuadList quadList;

  // Output file
  std::ofstream mainFile;
  size_t fileOffset;

  // Index file name
  std::string iname;
};
