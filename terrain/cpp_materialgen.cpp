class TextureSplatter
{
public:
  inline TextureSplatter(const std::vector<int>& ltex,
                         int texSize,
                         int ltexsize)
    : mLTEX(ltex),
      mTexSize(texSize), //the root of the size of the texture
      mLTEXSize(ltexsize) //the root of the ltex array
      { mSizeDiff = texSize/ltexsize; }

  int getAlphaAtPixel(int x, int y, int tid)
  {
    if ( getTextureAtPixel(x,y) == tid )
      return 255;
    /*
    else if ( mBorder > 0 )
      {
        //hacky remove fix. perofrmance issues. skips if not ingame gen.
        float amount = 0;
        for ( int ty = y-1; ty <= y+1; ++ty ) {
          for ( int tx = x-1; tx <= x+1; ++tx ) {
            if ( ty < -mBorder*mSizeDiff ||
                 tx < -mBorder*mSizeDiff ||
                 ty >= mTexSize+mBorder*mSizeDiff ||
                 tx >= mTexSize+mBorder*mSizeDiff  )
              continue;

            if ( getTextureAtPixel(tx,ty) == tid )
              amount += 0.18f;
          }
        }
        if ( amount > 1 ) amount = 1;
        assert(amount>=0&&amount<=1);
        return amount*255;
      }
    */
    return 0;
  }

private:

  int getTextureAtPixel(int x, int y) {
    x = (x - x%mSizeDiff)/mSizeDiff;
    y = (y - y%mSizeDiff)/mSizeDiff;

    return mLTEX[(y)*mLTEXSize+(x)];
  }

  const std::vector<int>& mLTEX;
  const int mTexSize;
  const int mLTEXSize;
  int mSizeDiff;
};

/**
 * Handles the runtime generation of materials
 *
 */
class MaterialGenerator
{
public:

  /**
   * @brief generates a material for a quad using a single
   * texture. Only used at runtime, not while generating.
   */
  Ogre::MaterialPtr generateSingleTexture
    (const std::string& texName,
     std::list<Ogre::ResourcePtr> createdResources)
  {
    const std::string matname("MAT" + Ogre::StringConverter::toString(mCount++));

    if ( !Ogre::MaterialManager::getSingleton().resourceExists(matname) )
      {
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(matname,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        Ogre::Pass* p = mat->getTechnique(0)->getPass(0);
        p->setLightingEnabled(false);
        p->createTextureUnitState(texName)->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
        createdResources.push_back(mat);

        return mat;
      }
    return Ogre::MaterialPtr();
  }

  /**
   * gets the material for the alpha textures
   */
  Ogre::MaterialPtr new_getAlphaMat(std::vector<int>& ltex,
                                int size,
                                int border,
                                float scaleDiv,
                                std::list<Ogre::ResourcePtr>& createdResources)
  {
    const std::string materialName("MAT" + Ogre::StringConverter::toString(mCount++));

    // We REALLY need to clean these variables up

    // Number of textures along one side
    const int ltexWidth = Ogre::Math::Sqrt(ltex.size());

    // Only true if border = 0, which I guess I've assumed below
    assert(ltexWidth == size);

    // Side for the quad only, not including the border. Used for the
    // assert() below only.
    const int rootSideLength = ltexWidth-(border*2);

    // Multiply up the number of textures along a side to get the
    // resolution of the alpha map. This gives 4*4=16 alpha pixels per
    // texture square, or 4*16 = 64 alpha pixels across the entire
    // quad.
    const int sizeDiff = 4;
    size *= sizeDiff;

    assert(size%rootSideLength==0);

    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().
      create(materialName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    createdResources.push_back(material);

    // But the default texture in the bottom 'layer', so that we don't
    // end up seeing through the landscape.
    Ogre::Pass* np = material->getTechnique(0)->getPass(0);
    np->setLightingEnabled(false);
    // FIXME: Why 0.1f? 
    np->createTextureUnitState("_land_default.dds")->setTextureScale(0.1f/scaleDiv,0.1f/scaleDiv);

    // Put all the texture indices in a set
    typedef std::set<int> SI;
    typedef SI::iterator SI_it;
    SI textures;
    for( int in = 0; in < ltex.size(); in++ )
      textures.insert(ltex[in]);

    // Allocate the buffers
    typedef std::vector<uint8_t> AlphaBuf;
    std::map<int,AlphaBuf> alphaMap;

    const int bufSize = size*size;

    //TextureSplatter ts(ltex, size, ltexWidth, border);

    // Peform splatting. Loop over each texture square in this quad.
    for(int ty=0; ty < size; ty += sizeDiff)
      for(int tx=0; tx < size; tx += sizeDiff)
        {
          // Get the texture index for this square
          const int thisInd = ltex[tx + ltexWidth*ty];

          // Offset for this texture
          const int offs = ty*size + tx;

          AlphaBuf &abuf = alphaMap[thisInd];

          abuf.resize(bufSize);

          // Set alphas to full for this square
          for(int y=0; y<sizeDiff; y++)
            for(int x=0; x<sizeDiff; x++)
              {
                int toffs = offs + y*size + x;
                if(toffs >= abuf.size())
                std::cout << "tx=" << tx << " ty=" << ty
                          << " x=" << x << " y=" << y
                          << " offs=" << offs
                          << " toffs=" << toffs
                          << " abuf.size()=" << abuf.size()
                          << "\n";
                assert(toffs < abuf.size());
                abuf[toffs] = 255;
              }

          // Get alpha influence from neighbouring cells.

          // FIXME: Rewrite TextureSplatter
        }

    // Create passes for each alpha buffer
    for ( SI_it itr = textures.begin(); itr != textures.end(); ++itr )
      {
        int tid = *itr;

        const std::string tn(mTexturePaths[tid]);
        if ( tn == "_land_default.dds" )
          continue;

        //std::cout << "  Generating texture " << tn << "\n";

        std::string alphaName(materialName + "_A_" + tn);
        if ( Ogre::TextureManager::getSingleton().resourceExists(alphaName) )
          OGRE_EXCEPT(0, "ALPHA Already Exists", "");

        //create alpha map
        Ogre::TexturePtr texPtr  = Ogre::TextureManager::getSingleton().
          createManual(
                       alphaName, // Name of texture
                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, // Name of resource group in which the texture should be created
                       Ogre::TEX_TYPE_2D,
                       size,size, //size ofc
                       1,0, //depth, mipmaps
                       Ogre::PF_A8, //we only need one channel to hold the splatting texture
                       Ogre::TU_STATIC_WRITE_ONLY //best performace, we shopuldn't need the data again
                       );

        createdResources.push_back(texPtr);

        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texPtr->getBuffer();
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);

        AlphaBuf *abuf = &alphaMap[tid];
        for(AlphaBuf::iterator it = abuf->begin(); it != abuf->end(); it++)
          *(pDest++) = *it;

        pixelBuffer->unlock();

        np = material->getTechnique(0)->createPass();
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
        tus->setTextureScale(0.1f/scaleDiv,0.1f/scaleDiv);
      }

    return material;
  }

  // The old variant
  Ogre::MaterialPtr getAlphaMat(std::vector<int>& ltex,
                                int size,
                                float scaleDiv,
                                std::list<Ogre::ResourcePtr>& createdResources)
  {
    const std::string materialName("MAT" + Ogre::StringConverter::toString(mCount++));

    // Multiply up the number of textures along a side to get the
    // resolution of the alpha map. This gives 4*4=16 alpha pixels per
    // texture square, or 4*16 = 64 alpha pixels across the entire
    // quad.
    const int sizeDiff = 4;
    size *= sizeDiff;

    // Number of textures along one side
    const int ltexWidth = Ogre::Math::Sqrt(ltex.size());

    assert(size%ltexWidth==0);

    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().
      create(materialName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    createdResources.push_back(material);

    // But the default texture in the bottom 'layer', so that we don't
    // end up seeing through the landscape.
    Ogre::Pass* np = material->getTechnique(0)->getPass(0);
    np->setLightingEnabled(false);
    np->createTextureUnitState("_land_default.dds")->setTextureScale(0.1f/scaleDiv,0.1f/scaleDiv);

    // Put all the texture indices in a set
    std::set<int> textures;
    for ( int y1 = 0; y1 < ltexWidth; y1++ ) {
      for ( int x1 = 0; x1 < ltexWidth; x1++ ) {
        textures.insert(ltex[(y1)*ltexWidth+x1]);
      }
    }

    for ( std::set<int>::iterator itr = textures.begin(); itr != textures.end(); ++itr )
      {
        int tid = *itr;

        const std::string tn(mTexturePaths[tid]);
        if ( tn == "_land_default.dds" )
          continue;

        //std::cout << "  Generating texture " << tn << "\n";

        std::string alphaName(materialName + "_A_" + tn);
        if ( Ogre::TextureManager::getSingleton().resourceExists(alphaName) )
          OGRE_EXCEPT(0, "ALPHA Already Exists", "");

        //create alpha map
        Ogre::TexturePtr texPtr  = Ogre::TextureManager::getSingleton().
          createManual(
                       alphaName, // Name of texture
                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, // Name of resource group in which the texture should be created
                       Ogre::TEX_TYPE_2D,
                       size,size, //size ofc
                       1,0, //depth, mipmaps
                       Ogre::PF_A8, //we only need one channel to hold the splatting texture
                       Ogre::TU_STATIC_WRITE_ONLY //best performace, we shopuldn't need the data again
                       );

        createdResources.push_back(texPtr);


        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texPtr->getBuffer();
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);
        memset(pDest,0, sizeof(Ogre::uint8)*size*size);

        TextureSplatter ts(ltex, size, ltexWidth);
        for ( int ty = 0; ty < size; ty++ ) {
          for ( int tx = 0; tx < size; tx++ ) {
            pDest[ty*size+tx] = ts.getAlphaAtPixel(tx,ty, tid);
          }
        }

        pixelBuffer->unlock();

        np = material->getTechnique(0)->createPass();
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
        tus->setTextureScale(0.1f/scaleDiv,0.1f/scaleDiv);
      }

    return material;
  }

  inline void setTexturePaths( std::map<int, std::string> r) {
    mTexturePaths = r;
  }
private:
  /**
   * Merged records accross all mods for LTEX data
   */
  std::map<int, std::string> mTexturePaths;

  unsigned int mCount;
};
