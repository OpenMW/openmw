/**
 * Handles the runtime generation of materials
 *
 */
class MaterialGenerator
{
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
      } else if ( mBorder > 0 ) { //hacky remove fix. perofrmance issues. skips if not ingame gen.
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
      //y = floor(float(y)/float(mSizeDiff));
      //x = floor(float(x)/float(mSizeDiff));

      return mLTEX[(y)*mLTEXSize+(x)];
    }

    const std::vector<int>& mLTEX;
    const int mTexSize;
    const int mLTEXSize;
    const int mBorder;
    int mSizeDiff;
  };

public:

  /**
   * @brief generates a material for a quad using a single texture
   */
  Ogre::MaterialPtr generateSingleTexture(const std::string& matname, const std::string& texName, std::list<Ogre::ResourcePtr> createdResources)
  {
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
   * Currently doesn't work
   */
  Ogre::MaterialPtr getShaderAlpha(const std::string& materialName, std::vector<int>& ltex, int size, std::list<Ogre::TexturePtr>& generatedAlphas)
  {
    Ogre::TexturePtr currentAlphaTexture; //ptr to the texture we are currnly writing to
    Ogre::uint8* currentAlphaPtr = 0; //pointer to the data
    Ogre::HardwarePixelBufferSharedPtr currentPixelBuffer; //pointer to the buffer for ctrling locks/unlicjks
    int currentColour = 0; //current colour index. ARGB.
    std::vector<std::string> texturesWritten; //a list of "done" textures. Order important
    std::vector<std::string> alphasGenerated; //alphas generated

    std::set<short> tidDone; //holds the list of done textures

    const int rootSideLength = Ogre::Math::Sqrt(ltex.size()); //used for looping

    //loop over every splat possition
    for ( int y1 = 0; y1 < rootSideLength; y1++ ) {
      for ( int x1 = 0; x1 < rootSideLength; x1++ ) {
        const short tid = ltex[y1*rootSideLength+x1];

        //if already done.
        if ( tidDone.find(tid) != tidDone.end())	continue;

        //insert it into the done list
        tidDone.insert(tid);

        //get the textuyre path. If it is default we don't need to do anything, as it
        //is done in the first pass. CHANGE? We end up using a whole pass for it??
        const std::string tn(mTexturePaths[tid]);
        if ( tn == "_land_default.dds" ) continue;

        texturesWritten.push_back(tn);

        //unqiue alpha name
        const std::string alphaName(materialName + "_A_" + tn);


        if ( Ogre::TextureManager::getSingleton().resourceExists(alphaName) )
          OGRE_EXCEPT(0, "ALPHA Already Exists", ""); //shouldn't happen.

        //check if we need to create a new apla
        if ( currentColour == 4 || //we only have 4 colours per tex ofc
             currentAlphaTexture.isNull() ) { //no texture assigned yet

          //unlock old buffer, if we have one
          if ( !currentAlphaTexture.isNull() )
            currentPixelBuffer->unlock();

          //new texture
          currentAlphaTexture  = Ogre::TextureManager::getSingleton().
            createManual(
                         alphaName,
                         Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                         Ogre::TEX_TYPE_2D,
                         size,size, //size ofc
                         1,	0, //depth, mipmaps
                         Ogre::PF_A8R8G8B8, //4 channesl for 4 splats
                         Ogre::TU_STATIC_WRITE_ONLY //we only need to write data
                         );

          generatedAlphas.push_back(currentAlphaTexture); //record
          alphasGenerated.push_back(alphaName);


          currentPixelBuffer = currentAlphaTexture->getBuffer();
          currentPixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
          const Ogre::PixelBox& pixelBox = currentPixelBuffer->getCurrentLock();

          currentAlphaPtr = static_cast<Ogre::uint8*>(pixelBox.data);

          //zero out all data
          for ( int i = 0; i < size*size*4; i++ )
            currentAlphaPtr[i] = 0;


          currentColour = 0;
        } else {
          currentColour++; //increase the colour we are working with by one
        }


        //for every splat that the texture is splatted,
        //check if it is the same as the one in the current splat
        for ( int y2 = 0; y2 < rootSideLength; y2++ ) {
          for ( int x2 = 0; x2 < rootSideLength; x2++ ) {
            if ( ltex[y2*rootSideLength+x2] == tid ) {

              //add splat to current alpha map
              const int splatSize = size/rootSideLength;

              for ( int ys = 0; ys < splatSize ; ys++ ) {
                for ( int xs = 0; xs < splatSize; xs++ ) {
                  //calc position on main texture
                  const int pxpos = splatSize*x2+xs;
                  const int pypos = splatSize*y2+ys;

                  //write splat to buffer
                  const int index = (pypos*size*4)+(pxpos*4)+currentColour;
                  currentAlphaPtr[index] = 255;
                }
              }
            }
          }
        }//for ( int y2 = 0; y2 < rootSideLength; y2++ ){


      }
    }//for ( int y1 = 0; y1 < rootSideLength; y1++ ){

    //check to see if we need to unlock a buff again
    if ( !currentAlphaTexture.isNull() )
      currentPixelBuffer->unlock();

    //sort material
    assert(Ogre::MaterialManager::getSingleton().getByName("AlphaSplatTerrain").getPointer());

    Ogre::MaterialPtr material = ((Ogre::Material*)Ogre::MaterialManager::getSingleton().getByName("AlphaSplatTerrain").getPointer())->clone(materialName);

    Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

    //write alphas
    if ( alphasGenerated.size() > 0 )
      pass->getTextureUnitState(0)->setTextureName(alphasGenerated[0]);
    if ( alphasGenerated.size() > 1 )
      pass->getTextureUnitState(1)->setTextureName(alphasGenerated[1]);

    //write 8 textures
    int c = 1;
    for ( std::vector<std::string>::iterator itr = texturesWritten.begin();
          itr != texturesWritten.end();
          ++itr ) {
      if ( ++c > 8 ) break;
      pass->getTextureUnitState(1+c)->setTextureName(*itr);
      //ta["Splat" + Ogre::StringConverter::toString(c)] = *itr;
    }

    //pass->applyTextureAliases(ta, true);

    return material;

  }

  /**
   * gets the material for the alpha textures
   */
  Ogre::MaterialPtr getAlphaMat(const std::string& materialName,
                                std::vector<int>& ltex,
                                int size,
                                int border,
                                float scaleDiv,
                                std::list<Ogre::ResourcePtr>& createdResources)
  {
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

  std::string generateAlphaMap(const std::string& name,
                               std::vector<int>& ltex,
                               int size,
                               int border,
                               float scaleDiv,
                               std::list<Ogre::ResourcePtr>& createdResources)
  {
    //std::string materialName = "TEX_" + Ogre::StringConverter::toString(cp.x) + "_" + Ogre::StringConverter::toString(cp.y);

    if ( Ogre::MaterialManager::getSingleton().resourceExists(name) )
      assert(0);
    //return materialName;

    getAlphaMat(name, ltex, size, border, scaleDiv, createdResources);
    //getShaderAlpha(materialName, ltex, size, newTextures);

    return name;
  }

  inline void setTexturePaths( std::map<int, std::string> r) {
    mTexturePaths = r;
  }
private:
  /**
   * Merged records accross all mods for LTEX data
   */
  std::map<int, std::string> mTexturePaths;
};
