/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (generator.d) is part of the OpenMW package.

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

// This module is responsible for generating the cache files.
module terrain.generator;

import std.stdio;
import std.string;

import terrain.cachewriter;
import terrain.esmland;
import terrain.terrain;
import util.cachefile;

const float TEX_SCALE = 1.0/16;

int mCount;

// Texture sizes for the various levels. For the most detailed level
// (level 1), this give the size of the alpha splatting textures rather
// than a final texture.
int[] texSizes;

CacheWriter cache;

void generate(char[] filename)
{
  makePath(cacheDir);

  //cache.openFile(filename);

  // Find the maxiumum distance from (0,0) in any direction
  int max = mwland.getMaxCoord();

  // Round up to nearest power of 2
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
  texSizes.length = depth+1;
  texSizes[6] = 1024;
  texSizes[5] = 512;
  texSizes[4] = 256;
  texSizes[3] = 256;
  texSizes[2] = 256;
  texSizes[1] = 64;

  writefln("Data generation not implemented yet");

  // Set some general parameters for the runtime
  cache.setParams(depth+1, texSizes[1]);

  /*
  // Create some common data first
  writefln("Generating common data");
  genDefaults();
  genIndexData();

  writefln("Generating quad data");
  GenLevelResult gen;
  // Start at one level above the top, but don't generate a mesh for
  // it
  genLevel(depth+1, -max, -max, gen, false);
  writefln("Writing index file");
  cache.finish();
  writefln("Pregeneration done. Results written to ", filename);
  */
}

/+

// Default textures
GenLevelResult[] defaults;

// Generates the default texture images "2_default.png" etc
void genDefaults()
{
  int size = texSizes.length-1;
  defaults.length = size;

  for(int i=1; i<size; i++)
    defaults[i].quad.info.level = i;

  // Sending null as the first parameter tells the function to only
  // render the default background.
  assert(size > 2);
  genLevel2Map(null, defaults[2]);

  for(int i=3; i<size; i++)
    mergeMaps(null, defaults[i]);
}

// Generates common mesh information that's stored in the .index
// file. This includes the x/y coordinates of meshes on each level,
// the u/v texture coordinates, and the triangle index data.
void genIndexData()
{
  // Generate mesh data for each level.

  /*
    TODO: The mesh data is very easy to generate, and we haven't
    really tested whether it's worth it to pregenerate it rather than
    to just calculate it at runtime. Unlike the index buffer below
    (which is just a memcpy at runtime, and definitely worth
    pregenerating), we have to loop through all the vertices at
    runtime anyway in order to splice this with the height data. It's
    possible that the additional memory use, pluss the slowdown
    (CPU-cache-wise) of reading from two buffers instead of one, makes
    it worthwhile to generate this data at runtime instead. However I
    guess the differences will be very small either way.
  */
  for(int lev=1; lev<=6; lev++)
    {
      // Make a new buffer to store the data
      int size = 65*65*4;
      auto vertList = new float[size];
      float *vertPtr = vertList.ptr;

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
      assert(vertPtr-vertList.ptr == size);
      // Store the buffer
      cache.addVertexBuffer(lev,vertList);
    }

  // Next up, triangle indices
  int size = 64*64*6;
  auto indList = new ushort[size];
  ushort *indPtr = indList.ptr;

  bool flag = false;
  for ( int y = 0; y < 64; y++ )
    {
      for ( int x = 0; x < 64; x++ )
        {
          int line1 = y*65 + x;
          int line2 = (y+1)*65 + x;

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
        }
      flag = !flag; //flip tries for next row
    }
  assert(indPtr-indList.ptr==size);

  // The index buffers are the same for all levels
  cache.addIndexBuffer(1,indList);
  cache.addIndexBuffer(2,indList);
  cache.addIndexBuffer(3,indList);
  cache.addIndexBuffer(4,indList);
  cache.addIndexBuffer(5,indList);
  cache.addIndexBuffer(6,indList);
}

void genLevel(int level, int X, int Y, ref GenLevelResult result,
              bool makeData = true)
{
  result.quad.info.cellX = X;
  result.quad.info.cellY = Y;
  result.quad.info.level = level;

  assert(result.isEmpty);

  // Level 1 (most detailed) is handled differently from the
  // other leves.
  if(level == 1)
    {
      assert(makeData);

      if(!mwland.hasData(X,Y))
        // Oops, there's no data for this cell. Skip it.
        return;

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
  assert(level > 1);

  // Number of cells along one side in each sub-quad (not in this
  // quad)
  int cells = 1 << (level-2);

  // Call the sub-levels and store the result
  GenLevelResult sub[4];
  genLevel(level-1, X, Y, sub[0]);             // NW
  genLevel(level-1, X+cells, Y, sub[1]);       // NE
  genLevel(level-1, X, Y+cells, sub[2]);       // SW
  genLevel(level-1, X+cells, Y+cells, sub[3]); // SE

  scope(exit)
    {
      foreach(ref s; sub)
        s.kill();
    }

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
      if(level == 2)
        // For level==2, generate a new texture from the alpha maps.
        genLevel2Map(sub, result);
      else
        // Level 3+, merge the images from the previous levels
        mergeMaps(sub, result);

      // Create the landscape mesh by merging the result from the
      // children.
      mergeMesh(sub, result);
    }

  // Store the result in the cache file
  cache.writeQuad(result.quad);
}

// Generate mesh data for one cell
void genLevel1Meshes(ref GenLevelResult res)
{
  // Constants
  const int intervals = 64;
  const int vertNum = intervals+1;
  const int vertSep = 128;

  // Allocate the mesh buffer
  res.allocMesh(vertNum);

  int cellX = res.quad.info.cellX;
  int cellY = res.quad.info.cellY;
  assert(res.quad.info.level==1);

  MeshHolder *mh = &res.quad.meshes[0];
  MeshInfo *mi = &mh.info;

  mi.worldWidth = vertSep*intervals;
  assert(mi.worldWidth == 8192);

  auto land = mwland.getLandData(cellX, cellY);

  byte[] heightData = land.vhgt.heights;
  byte[] normals = land.normals;
  mi.heightOffset = land.vhgt.heightOffset;

  float max=-1000000.0;
  float min=1000000.0;

  byte *vertPtr = mh.vertexBuffer.ptr;
  assert(vertPtr !is null);

  // Loop over all the vertices in the mesh
  float rowheight = mi.heightOffset;
  float height;
  for(int y=0; y<65; y++)
    for(int x=0; x<65; x++)
      {
        // Offset of this vertex within the source buffer
        int offs=y*65+x;

        // The vertex data from the ESM
        byte data = heightData[offs];

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
        // Adjust the accumulated height with the new data. The
        // adjustment is a signed number.
        height += data;

        // Calculate the min and max
        max = height > max ? height : max;
        min = height < min ? height : min;

        // Store the normals
        for(int k=0; k<3; k++)
          *vertPtr++ = normals[offs*3+k];
      }

  // Make sure we wrote exactly the right amount of data
  assert(vertPtr-mh.vertexBuffer.ptr ==
         mh.vertexBuffer.length - 1);

  // Store the min/max values
  mi.minHeight = min * 8;
  mi.maxHeight = max * 8;
}

// Generate the alpha splatting bitmap for one cell.
void genCellAlpha(ref GenLevelResult res)
{
  int cellX = res.quad.info.cellX;
  int cellY = res.quad.info.cellY;
  assert(res.quad.info.level == 1);

  // List of texture indices for this cell. A cell has 16x16 texture
  // squares.
  int ltex[16][16];

  auto ltexData = mwland.getLTEXData(cellX, cellY);

  // A map from the global texture index to the local index for this
  // cell.
  int[int] textureMap;

  int texNum = 0; // Number of local indices

  // Loop through all the textures in the cell and get the indices
  bool isDef = true;
  for(int ty = 0; ty < 16; ty++)
    for(int tx = 0; tx < 16; tx++)
      {
        // Get the texture in a given cell
        char[] textureName = ltexData.getTexture(tx,ty);

        if(textureName == "")
          textureName = "_land_default.dds";
        else
          isDef = false;

        // Store the global index
        int index = cache.addTexture(textureName);
        ltex[ty][tx] = index;

        // Add the index to the map
        if(!(index in textureMap))
          textureMap[index] = texNum++;
      }
  assert(texNum == textureMap.length);

  // If we only found default textures, exit now.
  if(isDef)
    return;

  int imageRes = texSizes[1];
  int dataSize = imageRes*imageRes;

  // Number of alpha pixels per texture square
  int pps = imageRes/16;

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
  foreach(int global, int local; textureMap)
    res.setAlphaTex(local, global);

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

// Generate a texture for level 2 from four alpha maps generated in
// level 1.
void genLevel2Map(GenLevelResult *maps, ref GenLevelResult res)
{
  int fromSize = texSizes[1];
  int toSize = texSizes[2];

  // Create a new int type that's automatcially initialized to -1.
  typedef int mint=-1;
  static assert(mint.init == -1);
  typedef mint[4] LtexList;

  // Create an overview of which texture is used where. The 'key' is
  // the global texture index, the 'value' is the corresponding
  // local indices in each of the four submaps.
  LtexList[int] lmap;

  if(maps !is null) // NULL means only render default
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

            // Store it in the map. TODO: This won't work I think :(
            lmap[gIndex].inds[mi] = ltex;
          }
      }

  const float scale = TEX_SCALE/2;

  char[] materialName = "MAT" ~ toString(mCount++);

  auto mat = cpp_makeMaterial(materialName);

  /*
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
  */

  // Loop through all our textures
  if(maps !is null)
    foreach(int gIndex, LtexList inds; lmap)
      {
        char[] name = cache.getString(gIndex);
        if ( name == "_land_default.dds" )
          continue;

        // Instead of passing 'mat', it's better to just make it
        // global in C++ even if it's messy and definitely not thread
        // safe.
        auto pDest = cpp_makeAlphaLayer(mat, materialName ~ "_A_" ~ name);

        /*
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
        */

        // Fill in the alpha values. TODO: Do all this with slices instead.
        memset(pDest, 0, 4*fromSize*fromSize);
        for(int i=0;i<4;i++)
          {
            // Does this sub-image have this texture?
            if(inds[i] == -1) continue;

            assert(!maps[i].isEmpty());

            // Find the right sub-texture in the alpha map
            ubyte *from = maps[i].image.data +
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

        cpp_closeAlpha(name, scale);
        /*
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
        */
      }

  // Create the result buffer
  res.allocImage(toSize);

  // Output file name. TODO: The result structure can do this for us
  // now, it knows both the level and the cell coords. Figure out
  // what to do in the default case though.
  int X = res.quad.info.cellX;
  int Y = res.quad.info.cellY;
  char[] outname =
    "2_" ~ toString(X) ~ "_" ~ toString(Y) ~ ".png";

  // Override for the default image
  if(maps == NULL)
    outname = "2_default.png";

  outname = g_cacheDir + outname;

  // TODO: Store the file name in the cache (ie. in res), so we don't
  // have to generate it at runtime.

  cpp_cleanupAlpha(outname);

  /*
  Ogre::TexturePtr tex1 = getRenderedTexture(mp,materialName + "_T",
                                             toSize,Ogre::PF_R8G8B8);
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

  res.save(outname);
  */
}

void mergeMaps(GenLevelResult *maps, ref GenLevelResult res)
{
  int level = res.quad.info.level;

  assert(texSizes.length > level);
  assert(level > 2);
  int fromSize = texSizes[level-1];
  int toSize = texSizes[level];

  // Create a new image buffer large enough to hold the four
  // sub textures
  res.allocImage(fromSize*2);

  // Add the four sub-textures
  for(int mi=0;mi<4;mi++)
    {
      // Need to do this in pure D. Oh well.
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

      // Copy the image to the box. Might as well rewrite this to do
      // what we want.
      copyBox(dst, src);
    }

  // Resize image if necessary
  if(toSize != 2*fromSize)
    res.resize(toSize);

  int X = res.quad.info.cellX;
  int Y = res.quad.info.cellY;

  // Texture file name
  char[] outname = res.getPNGName(maps==NULL);

  outname = g_cacheDir + outname;

  // Save the image
  res.save(outname);
}

/*
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
*/

// Create the mesh for this level, by merging the meshes from the
// previous levels.
void mergeMesh(GenLevelResult *sub, ref GenLevelResult res)
{
  // How much to shift various numbers to the left at this level
  // (ie. multiply by 2^shift). The height at each vertex is
  // multiplied by 8 in each cell to get the final value. However,
  // when we're merging two cells (in each direction), we have to
  // scale down all the height values by 2 in order to fit the result
  // in one byte. This means multiplying the factor by 2 for each
  // level above the cell level.
  int shift = res.quad.info.level - 1;
  assert(shift >= 1);

  // Constants
  int intervals = 64;
  int vertNum = intervals+1;
  int vertSep = 128 << shift;

  // Allocate the result buffer
  res.allocMesh(vertNum);

  MeshHolder *mh = &res.quad.meshes[0];
  MeshInfo *mi = &mh.info;

  mi.worldWidth = vertSep*intervals;
  assert(mi.worldWidth == 8192<<shift);
  byte *vertPtr = mh.vertexBuffer;

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
              if(!s.isEmpty() && 0)
                {
                  MeshHolder *smh = &s.quad.meshes[0];
                  byte* inPtr = smh.vertexBuffer;

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

  // Set max and min values here
}

struct GenLevelResult
{
  bool isAlpha;

  QuadHolder quad;
  //PixelBox image;
  bool hasMesh;

  void kill()
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
    //image = PixelBox(width, width, texNum, Ogre::PF_A8);

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
        ubyte *ptr = image.data;
        quad.meshes[0].alphas[i].buffer = ptr + width*width*i;
        quad.meshes[0].alphas[i].info.bufSize = width*width;
      }

    return image.data;
  }

  void allocMesh(int size)
  {
    quad.meshes.resize(1);

    MeshHolder *mh = &quad.meshes[0];
    MeshInfo *mi = &mh.info;

    mi.vertRows = size;
    mi.vertCols = size;
    // 1 height, 3 normal components = 4 bytes per vertex. The reader
    // algorithm (fillVertexBuffer) needs 1 extra byte so add that
    // too.
    mi.vertBufSize = 4 * size*size + 1;
    mh.vertexBuffer = malloc(mi.vertBufSize);

    hasMesh = true;
  }

  void allocImage(int width)
  {
    assert(isEmpty());

    //image = PixelBox(width, width, 1, Ogre::PF_R8G8B8);
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
    //PixelBox nbox = PixelBox(newSize, newSize, 1, Ogre::PF_R8G8B8);
    nbox.data = malloc(newSize*newSize*4);

    // Resize the image. The nearest neighbour filter makes sure
    // there is no blurring.
    //Image::scale(image, nbox, Ogre::Image::FILTER_NEAREST);

    // Kill and replace the old buffer
    free(image.data);
    image = nbox;
  }

  // TODO: Handle file naming internally
  void save(char[] name)
  {
    assert(!isEmpty());
    assert(!hasAlpha());

    writefln("  Creating ", name);

    // Create an image from the pixelbox data
    Image img;
    img.loadDynamicImage(image.data,
                         image.getWidth(), image.getHeight(),
                         image.format);

    // and save it
    img.save(name);
  }
}

// Copy from one pixel box to another
void copyBox(PixelBox *dst, PixelBox *src)
{
  // Some sanity checks first
  assert(src.format == dst.format);
  assert(src.data !is null);
  assert(dst.data !is null);

  // Memory size of one pixel
  //int pixSize = Ogre::PixelUtil::getNumElemBytes(src.format);

  int fskip = src.rowPitch * pixSize;
  int tskip = dst.rowPitch * pixSize;
  int rows = src.getHeight();
  int rowSize = src.getWidth()*pixSize;

  assert(rows == dst.getHeight());
  assert(src.getWidth() == dst.getWidth());
  assert(dst.left == 0);
  assert(dst.top == 0);

  // Source and destination pointers
  ubyte *from = src.data;
  ubyte *to = dst.data;

  for(;rows>0;rows--)
    {
      memcpy(to, from, rowSize);
      to += tskip;
      from += fskip;
    }
}

// ------- OLD CODE - use these snippets later -------

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
+/
