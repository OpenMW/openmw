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

class Generator
{
public:
  Generator(const std::string& baseFileName) :
    mBaseFileName(baseFileName)
  {
#if GEN_LANDDATA
    mDataO.open(std::string(baseFileName+".data").c_str(), std::ios::binary);
#endif
  }

  inline void addLandData(RecordPtr record, const std::string& source)
  { mMWLand.addLandData(record, source); }

  inline void addLandTextureData(RecordPtr record, const std::string& source)
  { mMWLand.addLandTextureData(record, source); }

  void beginGeneration()
  {
    // Find the maxiumum distance from (0,0) in any direction
    int max = 0;
    max = std::max<int>(mMWLand.getMaxX(), max);
    max = std::max<int>(mMWLand.getMaxY(), max);
    max = std::max<int>(-mMWLand.getMinX(), max);
    max = std::max<int>(-mMWLand.getMinY(), max);

    //round up to nearest binary number. makes some stuff easier iirc
    //FIXME
    for ( int i = 1;;i++ ) {
      if ( max < pow((float)2, i) )
        {
          max = pow((float)2, i);
          break;
        }
      assert(i<=8); //don't go too high. Used for debug
    }

    int maxDepth = 0; //temp var used below
    // FIXME: make 8192 a constant
    mIndex.setRootSideLength(max*2*8192); //given that 8192 is the length of a cell

    //Keep deviding the root side length by 2 (thereby simulating a
    //split) until we reach the width of the base cell (or the
    //smallest quad). TODO: We should make sure that the right number
    //of levels are actually generated automatically, of course.
    for (long i = mIndex.getRootSideLength(); i > 8192; i/=2 )
      maxDepth++;
    mIndex.setMaxDepth(maxDepth);
  }

  void generateLODLevel(int level, int textureSize)
  {
    std::cout << "Generating Level " << level << "\n";

    assert(level <= mIndex.getMaxDepth());
    assert(level > 0 );
    assert(textureSize>2);
    assert(textureSize<=4096); //change to gpu max if pos

    const int initialLevel = level;

    // FIXME: Should probably use another name than 'level' here
    level = pow((float)2, level); //gap between verts that we want
    // Use this instead
    assert(level == 1 << initialLevel);

    const int halfLevel = level/2;
    assert(halfLevel > 0 );

    // FIXME. Search for all pow() calls and fix them
    int cellDist = pow((float)2, mIndex.getMaxDepth());

    // Temporary storage
    QuadData qd;
#if GEN_LANDDATA
    qd.setVertexSeperation(128*halfLevel); //dist between two verts

    std::vector<float>& gh = qd.getHeightsRef(); //ref to the data
                                                 //storage in the quad
    std::vector<char>& gn  = qd.getNormalsRef();
    gh.resize(LAND_NUM_VERTS); //allocate memory for mesh functions
    gn.resize(LAND_NUM_VERTS*3);
#endif

    //the 16*16 array used for holding the LTEX records (what texure
    //is splatted where)
    std::vector<int>& gl = qd.getTextureIndexRef();
    gl.resize((LAND_LTEX_WIDTH+2)*(LAND_LTEX_WIDTH+2));

    const std::string stringLevel(Ogre::StringConverter::toString(level));
    const std::string defaultTexture(stringLevel + "_default.png");
    bool hasUsedDefault = false;

    // loops over the start of each quad we want to get
    for(   int y = -(cellDist/2); y < (cellDist/2); y+=halfLevel )
      for( int x = -(cellDist/2); x < (cellDist/2); x+=halfLevel )
        {
          qd.setParentTexture("");
          bool usingDefaultTexture = false;

          if ( initialLevel == 1 )
            // For level one (up close), there's no need to generate
            // any textures, as we can use the textures from the BSAs
            // directly.
            generateLTEXData(x, y, gl);
          else
            {
              // Texture file name
              std::string name = stringLevel + "_" +
                Ogre::StringConverter::toString(x)  + "_" +
                Ogre::StringConverter::toString(y) + ".png";

              // Generate the texture
              bool hasGen = generateTexture(std::string(TEXTURE_OUTPUT) + name, textureSize, x, y, halfLevel);

              if ( hasGen ) qd.setTexture(name);
              else
                {
                  qd.setTexture(defaultTexture);
                  hasUsedDefault = true;
                  usingDefaultTexture = true;
                }
            }

          // Calculate parent texture
          if ( initialLevel != mIndex.getMaxDepth() )
            {
              // FIXME: This can definitely be improved, although it
              // doesn't matter performance wise

              //calcualte the level one higher
              // FIXME: pow() again...
              int higherLevel = pow((float)2, (initialLevel+1));
              int highHalfLevel = higherLevel/2;

              int higherX = x;
              if ( (higherX-halfLevel) % highHalfLevel == 0 )
                higherX -= halfLevel;

              int higherY = y;
              if ( (higherY-halfLevel) % highHalfLevel  == 0 )
                higherY -= halfLevel;

              std::string higherName = Ogre::StringConverter::toString(higherLevel) + "_" +
                Ogre::StringConverter::toString(higherX)  + "_" +
                Ogre::StringConverter::toString(higherY) + ".png";

              //check file exists without needing boost filesystem libs
              FILE* fp = fopen((std::string(TEXTURE_OUTPUT) + higherName).c_str(), "r");
              if ( fp )
                {
                  qd.setParentTexture(higherName);
                  fclose(fp);
                }
              else
                qd.setParentTexture("");
            }

#if GEN_LANDDATA
          generateMesh(gh, gn, x, y, halfLevel );

          bool isEmptyQuad = true;
          if ( usingDefaultTexture )
            {
              for ( int i = 0; i < LAND_NUM_VERTS; i++ ){
                if ( gh.at(i) != LAND_DEFAULT_HEIGHT ){
                  isEmptyQuad = false;
                  break;
                }
              }
            }
          else isEmptyQuad = false;

          if ( isEmptyQuad )
            continue;

          //save data
          //the data is the position of the generated quad
          mIndex.setOffset(x*8192+halfLevel*8192/2,
                           y*8192+halfLevel*8192/2,
                           mDataO.tellp());
          boost::archive::binary_oarchive oa(mDataO); //using boost fs to write the quad
          oa << qd;
#endif

        }

    //check if we have used a default texture
    if ( hasUsedDefault )
      {
        std::vector<int> ltex;
        ltex.resize(halfLevel*LAND_LTEX_WIDTH*halfLevel*LAND_LTEX_WIDTH, mPalette.getOrAddIndex("_land_default.dds"));
        renderTexture(std::string(TEXTURE_OUTPUT) + defaultTexture, ltex, textureSize, halfLevel*LAND_LTEX_WIDTH);
      }
  }

  void endGeneration()
  {
    // FIXME: Just write one file?
#if GEN_LANDDATA
    mDataO.close();
    std::ofstream ofi(std::string(mBaseFileName + ".index").c_str(), std::ios::binary);
    boost::archive::binary_oarchive oai(ofi);
    oai << mIndex;
#endif

    std::ofstream ofp(std::string(mBaseFileName + ".palette").c_str(), std::ios::binary);
    boost::archive::binary_oarchive oap(ofp);
    oap << mPalette;
  }

private:

  // Generate texture data for level 1.
  void generateLTEXData(int x, int y, std::vector<int>& index)
  {
    // All this does is to loop through all the 16x16 'land grid
    // points' in the cell (plus the outer border of grid points from
    // the surrounding cells) and store the texture names in the
    // index.
    for ( int texY = 0; texY < LAND_LTEX_WIDTH+2; texY++ )
      for ( int texX = 0; texX < LAND_LTEX_WIDTH+2; texX++ )
        {
          int tempX = x;
          int tempY = y;

          int sourceX = texX - 1;
          int sourceY = texY - 1;

          if ( sourceX == -1 )
            {
              tempX--;
              sourceX = LAND_LTEX_WIDTH-1;
            }
          else if ( sourceX == LAND_LTEX_WIDTH)
            {
              tempX++;
              sourceX = 0;
            }

          if ( sourceY == -1 )
            {
              tempY--;
              sourceY = LAND_LTEX_WIDTH-1;
            }
          else if ( sourceY == LAND_LTEX_WIDTH )
            {
              tempY++;
              sourceY = 0;
            }

          std::string source;
          short texID = 0;

          if ( mMWLand.hasData(tempX, tempY) )
            {
              source = mMWLand.getSource(tempX, tempY);
              texID = mMWLand.getLTEXIndex(tempX,tempY, sourceX, sourceY);
            }

          std::string texturePath = "_land_default.dds";
          if ( texID != 0 && mMWLand.hasLTEXRecord(source,--texID) )
            texturePath = mMWLand.getLTEXRecord(source,texID);

          // textures given as tga, when they are a dds. I hate that "feature"
          // FIXME: do we handle this already?
          size_t d = texturePath.find_last_of(".") + 1;
          texturePath = texturePath.substr(0, d) + "dds";
          std::transform(texturePath.begin(), texturePath.end(), texturePath.begin(), tolower);

          index[texY*(LAND_LTEX_WIDTH+2)+texX] = mPalette.getOrAddIndex(texturePath);
        }
  }

  // Create a cache texture.
  bool generateTexture(const std::string& name, int size,
                       int blockX, int blockY, int halfLevel)
  {
    int width = size/(halfLevel*LAND_LTEX_WIDTH);
    assert(width>=1);

    // Preallocate a buffer of texture indices
    std::vector<int> ltex;
    ltex.resize(halfLevel*LAND_LTEX_WIDTH*halfLevel*LAND_LTEX_WIDTH, 0);

    // This loop collection is just to gather the texture indices for
    // this quad. All texture paths are inserted into the
    for ( int y = 0; y < halfLevel; y++ )
      for ( int x = 0; x < halfLevel; x++ )
        {
          // And loop over all 16x16 textures in each cell
          for ( int texY = 0; texY < LAND_LTEX_WIDTH; texY++ )
            for ( int texX = 0; texX < LAND_LTEX_WIDTH; texX++ )
              {
                std::string source;
                short texID = 0;

                // Get the data from the ESM
                if ( mMWLand.hasData(x + blockX, y + blockY) )
                  {
                    source = mMWLand.getSource(x + blockX, y + blockY);
                    texID = mMWLand.getLTEXIndex(x + blockX, y + blockY, texX, texY);
                  }

                std::string texturePath = "_land_default.dds";
                if ( texID != 0 && mMWLand.hasLTEXRecord(source,--texID) )
                  texturePath = mMWLand.getLTEXRecord(source,texID);

                // Convert .tga to .dds
                // FIXME: ditto earlier comment.
                size_t d = texturePath.find_last_of(".") + 1;
                texturePath = texturePath.substr(0, d) + "dds";
                //FIXME2: BTW, didn't we make the BSA reader case insensitive?
                std::transform(texturePath.begin(), texturePath.end(), texturePath.begin(), tolower);
                const int index = (y*LAND_LTEX_WIDTH+texY)*halfLevel*LAND_LTEX_WIDTH + x*LAND_LTEX_WIDTH+texX;

                // FIXME: In any case, we should let mPalette do the
                // name conversion. So if the texture is already
                // inserted (which will almost always be the case) we
                // can avoid costly string operations.
                ltex[index] = mPalette.getOrAddIndex(texturePath);
              }
        }

    // See if we have used anything at all. FIXME: This isn't needed
    // as a separate loop
    int sum = 0;
    for ( std::vector<int>::iterator i = ltex.begin(); i != ltex.end(); ++i )
      sum += *i;

    if ( sum == 0 ) //not used any textures
      return false;

    renderTexture(name, ltex, size, halfLevel*LAND_LTEX_WIDTH);

    return true;
  }

  // FIXME: renderTexture and getRenderedTexture don't strike me as
  // the optimal ways of doing this. For one it's hardware/driver
  // dependent, when it should be a simple computational exercise. But
  // understand what it actually does, before you change anything.

  // This function (and it's sub calls) is where most of the time is
  // spent in the gen process, so this is also where we need to
  // optimize.
  void renderTexture(const std::string& outputName, std::vector<int>& ltex,
                     int texSize, int alphaSize)
  {
    std::cout << "  Creating " << outputName << "\n";

    assert(Ogre::Math::Sqrt(ltex.size())==alphaSize);
    std::list<Ogre::ResourcePtr> createdResources;

    // FIXME: Move out of this function?
    MaterialGenerator mg;
    mg.setTexturePaths(mPalette.getPalette());

    // Aaand, this is just halfLevel again, which already is a very
    // poor name for the number of cells along the side of the quad.
    const int scaleDiv = alphaSize/LAND_LTEX_WIDTH;

    //genetate material/aplahas
    Ogre::MaterialPtr mp = mg.getAlphaMat(ltex, alphaSize, scaleDiv,createdResources);
    Ogre::TexturePtr tex1 = getRenderedTexture(mp, "RTT_TEX_1",texSize, Ogre::PF_R8G8B8);
    tex1->getBuffer()->getRenderTarget()->writeContentsToFile(outputName);
    Ogre::MaterialManager::getSingleton().remove(mp->getHandle());

    //remove the texture we have just written to the fs
    Ogre::TextureManager::getSingleton().remove(tex1->getHandle());

    //remove all the materials
    const std::list<Ogre::ResourcePtr>::const_iterator iend = createdResources.end();
    for ( std::list<Ogre::ResourcePtr>::const_iterator itr = createdResources.begin();
          itr != iend;
          ++itr) {
      (*itr)->getCreator()->remove((*itr)->getHandle());
    }
  }

  // Does everything in OGRE
  Ogre::TexturePtr getRenderedTexture(Ogre::MaterialPtr mp, const std::string& name,
                                      int texSize, Ogre::PixelFormat tt)
  {
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

    renderTexture->removeAllViewports(); //needed?

    return texture;
  }

  void generateMesh(std::vector<float>& gh, std::vector<char>& gn, int blockX,
                    int blockY, int halfLevel)
  {
    int gnc = 0;
    int ghc = 0;
    for ( int y = 0; y < LAND_VERT_WIDTH; y++ )
      for ( int x = 0; x < LAND_VERT_WIDTH; x++ )
        {
          //FIXME: Eh, what?
          int cellY = floor((float)y/LAND_VERT_WIDTH*halfLevel) + blockY;
          int cellX = floor((float)x/LAND_VERT_WIDTH*halfLevel) + blockX;

          std::vector<float>& ch = mMWLand.getHeights(cellX,cellY);
          std::vector<char>& cn =  mMWLand.getNormals(cellX,cellY);

          int vertY = (((float)y/LAND_VERT_WIDTH*halfLevel) -
                       (float)floor((float)y/LAND_VERT_WIDTH*halfLevel)) * LAND_VERT_WIDTH;
          int vertX = (((float)x/LAND_VERT_WIDTH*halfLevel) -
                       (float)floor((float)x/LAND_VERT_WIDTH*halfLevel)) * LAND_VERT_WIDTH;

          assert(vertY < LAND_VERT_WIDTH && vertX < LAND_VERT_WIDTH);

          //store data
          gh[ghc++] = ch[vertY*LAND_VERT_WIDTH+vertX];

          for ( int z = 0; z < 3; z++ )
            gn[gnc++] = cn[(vertY*LAND_VERT_WIDTH+vertX)*3+z];
        }
  }

  std::string mBaseFileName; ///base file name so we gen mBaseFileName + ".index" etc
  std::ofstream mDataO;

  Index mIndex; ///the index of the data. holds offsets and max depth, rsl etc
  TexturePalette mPalette; ///all the textures from all mods are merge into a single palette
  MWLand mMWLand; ///deals with MW land stuff
};
