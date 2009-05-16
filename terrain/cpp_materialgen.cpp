class TextureSplatter
{
public:
  inline TextureSplatter(const std::vector<int>& ltex,
                         int texSize,
                         int ltexsize,
                         int border) :
    mLTEX(ltex),
    mTexSize(texSize), //the root of the size of the texture
    mLTEXSize(ltexsize), //the root of the ltex array
    mBorder(border) {   //the size of the border of the ltex
    mSizeDiff = texSize/(ltexsize-border*2);
  }

  int getAlphaAtPixel(int x, int y, int tid) {
    //offset for border
    x += (mBorder*mSizeDiff);
    y += (mBorder*mSizeDiff);

    if ( getTextureAtPixel(x,y) == tid ) {
      return 255;
    } else if ( mBorder > 0 )
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
  const int mBorder;
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
  Ogre::MaterialPtr getAlphaMat(std::vector<int>& ltex,
                                int size,
                                int border,
                                float scaleDiv,
                                std::list<Ogre::ResourcePtr>& createdResources)
  {
    const std::string materialName("MAT" + Ogre::StringConverter::toString(mCount++));

    const int sizeDiff = 4;
    size *= sizeDiff;

    const int rootSideLength = Ogre::Math::Sqrt(ltex.size())-(border*2); //used for looping
    const int ltexWidth = Ogre::Math::Sqrt(ltex.size());
    assert(size%rootSideLength==0);

    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().
      create(materialName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    createdResources.push_back(material);


    Ogre::Pass* np = material->getTechnique(0)->getPass(0);
    np->setLightingEnabled(false);
    np->createTextureUnitState("_land_default.dds")->setTextureScale(0.1f/scaleDiv,0.1f/scaleDiv);

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
                       1,	0, //depth, mipmaps
                       Ogre::PF_A8, //we only need one channel to hold the splatting texture
                       Ogre::TU_STATIC_WRITE_ONLY //best performace, we shopuldn't need the data again
                       );

        createdResources.push_back(texPtr);


        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texPtr->getBuffer();
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);
        memset(pDest,0, sizeof(Ogre::uint8)*size*size);

        TextureSplatter ts(ltex, size, ltexWidth, border);
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
