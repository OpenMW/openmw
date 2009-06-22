/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2009  Nicolay Korslund
  WWW: http://openmw.sourceforge.net/

  This file (cachewriter.d) is part of the OpenMW package.

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

import terrain.archive;

import terrain.outbuffer;
import std.stdio, std.stream, std.string;
import terrain.myfile;
import std.math2;
import monster.util.string;
import monster.vm.dbg;

// Helper structs
struct AlphaHolder
{
  AlphaInfo info;

  // Actual pixel buffer
  ubyte[] buffer;
}

struct MeshHolder
{
  MeshInfo info;

  // Actual buffers
  byte[] vertexBuffer;

  // Alpha maps (if any)
  AlphaHolder alphas[];
}

// A struct that gathers all the relevant quad data in one place.
struct QuadHolder
{
  QuadInfo info;

  MeshHolder meshes[];
}

struct CacheWriter
{
  // Opens the main archive file for output
  void openFile(char[] fname)
  {
    mainFile = new File(fname, FileMode.OutNew);
    iname = fname ~ ".index";

    buf = new OutBuffer;
  }

  void setParams(int mxLev, int alphSize)
  {
    maxLevel = mxLev;
    alphaSize = alphSize;

    vertBuf.length = maxLevel;
  }

  // Closes the main archive file and writes the index.
  void finish()
  {
    mainFile.close();

    // Write the index file
    scope MyFile ofile = new MyFile(iname, FileMode.OutNew);

    // Header first
    ArchiveHeader head;
    head.magic = CACHE_MAGIC;
    head.quads = quadList.length;
    head.rootLevel = maxLevel;
    head.alphaSize = alphaSize;
    head.stringNum = stringList.length;
    head.stringSize = totalStringLength;
    ofile.dump(head);

    // Write the quads
    ofile.dumpArray(quadList);

    // String table next. We need to sort it in order of the indices
    // first.
    char[][] strVector;
    strVector.length = head.stringNum;

    foreach(char[] key, int value; stringList)
      strVector[value] = key;

    // Next, write the strings to file while we fill in the offset
    // list
    int[] offsets = new int[head.stringNum];

    size_t curOffs = 0;
    for(int i=0; i<head.stringNum; i++)
      {
        // Add one byte for the zero terminator
        int len = strVector[i].length + 1;
        char *ptr = strVector[i].ptr;

        if(ptr[len-1] != 0)
          ptr = toStringz(strVector[i]);

        assert(ptr[len-1] == 0);

        ofile.writeExact(ptr, len);

        // Store the offset
        offsets[i] = curOffs;
        curOffs += len;
      }
    // At the end the offset should match the buffer size we set in
    // the header.
    assert(curOffs == head.stringSize);

    // Finally, write the offset table itself
    ofile.dumpArray(offsets);

    // Write the common vertex and index buffers
    assert(maxLevel == 7);
    for(int i=1;i<maxLevel;i++)
      {
        // Write vertex buffer
        ofile.writeArray(vertBuf[i]);
        delete vertBuf[i];
      }

    // Then the index buffer
    ofile.writeArray(indexBuf);

    // Don't need these anymore
    delete offsets;
    delete strVector;
    delete quadList;
    delete vertBuf;
    delete indexBuf;
    delete buf;
    delete mainFile;
  }

  // Add a common vertex buffer for a given level
  void addVertexBuffer(int level, float[] buf)
  {
    assert(vertBuf.length > level);
    vertBuf[level] = buf;
  }

  // Add a common index buffer
  void setIndexBuffer(ushort[] buf)
  {
    indexBuf = buf;
  }

  // Write a finished quad to the archive file. All the offsets and
  // numbers in the *Info structs are filled in automatically based on
  // the additional data in the Holder structs.
  void writeQuad(ref QuadHolder qh)
  {
    scope auto _trc = new MTrace("writeQuad");

    // Write the MeshInfo's first
    int meshNum = qh.meshes.length;

    MeshInfo meshes[] = buf.write!(MeshInfo)(meshNum);

    float minh = float.infinity;
    float maxh = -float.infinity;

    // Then write the mesh data in approximately the order it's read
    for(int i=0; i<meshNum; i++)
      {
        assert(meshes !is null);

        auto mh = &qh.meshes[i];

        // Copy the basic data first
        meshes[i] = mh.info;

        minh = min(minh,mh.info.minHeight);
        maxh = max(maxh,mh.info.maxHeight);

        // Set everything else except the offsets
        int alphaNum = mh.alphas.length;
        meshes[i].alphaNum = alphaNum;

        // Write the vertex buffer
        meshes[i].vertBufOffset = buf.size;
        meshes[i].vertBufSize = mh.vertexBuffer.length;
        writeBuf(mh.vertexBuffer);
        assert(buf.size == meshes[i].vertBufOffset + meshes[i].vertBufSize);

        // Next write the alpha maps, if any
        meshes[i].alphaOffset = buf.size;
        AlphaInfo ais[] = buf.write!(AlphaInfo)(alphaNum);

        // Loop through the alpha maps
        foreach(int k, ref ai; ais)
          {
            AlphaHolder ah = mh.alphas[k];
            ai = ah.info;

            // Write the alpha pixel buffer
            ai.bufOffset = buf.size;
            ai.bufSize = ah.buffer.length;
            writeBuf(ah.buffer);
          }
      }
    // Finally set up the QuadInfo itself
    QuadInfo qi;

    // Copy basic info
    qi = qh.info;

    // Derived info
    qi.meshNum = meshNum;
    qi.offset = fileOffset;
    qi.size = buf.size;
    qi.minHeight = minh;
    qi.maxHeight = maxh;

    // Get the side length, or the height difference if that is bigger
    qi.boundingRadius = max(maxh-minh,qi.worldWidth);

    // Multiply with roughly sqrt(1/2), converts from side length to
    // radius with some extra slack
    qi.boundingRadius *= 0.8;

    // The quad cache is done, write it to file
    buf.writeTo(mainFile);

    // Update the main offset
    fileOffset += qi.size;

    // Add the quad to the list. This list isn't written to the main
    // cache file, but to the index file.
    quadList ~= qi;
  }

  // Add a texture name as a string. Will convert .tga file names to
  // .dds as a convenience. TODO: Use the resource system to do this,
  // it automatically searches for the dds variant.
  int addTexture(char[] orig)
  {
    if(orig.iEnds(".tga"))
      orig = orig[0..$-3] ~ "dds";
    return addString(orig);
  }

  // Convert a string to an index
  int addString(char[] str)
  {
    // Do we already have the string?
    if(str in stringList)
      return stringList[str];

    // Nope, insert it
    int index = stringList.length;
    stringList[str] = index;
    stringLookup[index] = str;

    // Sum up the string lengths + 1 byte for the zero
    totalStringLength += str.length + 1;
    return index;
  }

  char[] getString(int index)
  {
    char[] res = stringLookup[index];
    assert(stringList[res] == index);
    return res;
  }

private:
  // Write the given block of memory to 'buf', possibly compressing
  // the data.
  void writeBuf(void[] ptr)
  {
    ulong size = ptr.length;

    // Reserve the maximum bytes needed.
    void toPtr[] = buf.reserve(size);

    // Store the data
    toPtr[] = ptr[];

    // Add the result buffer
    buf.add(toPtr[0..size]);
  }

  // Used for 'writing' to a changable memory buffer before writing to
  // file
  OutBuffer buf;

  // Common vertex and index buffers for all quads. One buffer per
  // level.
  float[][] vertBuf;
  ushort[] indexBuf;

  // Variables that must be set during the gen phase
  int maxLevel;
  int alphaSize;

  // Contains a unique index for each string
  int[char[]] stringList;
  char[][int] stringLookup;
  size_t totalStringLength;

  // List of all quads
  QuadInfo[] quadList;

  // Output file
  File mainFile;
  size_t fileOffset;

  // Index file name
  char[] iname;
}
