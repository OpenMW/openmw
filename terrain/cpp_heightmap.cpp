class HeightMap
{
public:
  HeightMap(Ogre::SceneNode* r);

  ~HeightMap();

  /**
   * loads the quad data from the disk
   */
  QuadData* getData(long x, long y)
  {
    long offset = mIndex.getOffset(x,y);
    if ( offset == -1 ) //check we have xy
      assert(0);

    mDataIFS.seekg(offset);

    //load the quad from the file
    QuadData* q = new QuadData();
    boost::archive::binary_iarchive oa(mDataIFS);
    oa >> *q;
    return q;
  }

  inline bool hasData(long x, long y)
  { return (mIndex.getOffset(x,y) != -1 ); }

  inline long getRootSideLength() {
    return mIndex.getRootSideLength();
  }
  inline int getMaxDepth() {
    return mIndex.getMaxDepth();
  }

  /**
   * Loads the index and palette
   */
  bool load(const std::string& fn)
  {
    {
      std::ifstream ifs(std::string(fn + ".index").c_str(), std::ios::binary);
      boost::archive::binary_iarchive oa(ifs);
      oa >> mIndex;
    }
    {
      std::ifstream ifs(std::string(fn + ".palette").c_str(), std::ios::binary);
      boost::archive::binary_iarchive oa(ifs);
      oa >> mPalette;
    }
    g_materialGen->setTexturePaths(mPalette.getPalette());

    mDataIFS.open(std::string(fn + ".data").c_str(), std::ios::binary);
    return true;
  }

  void create();

  inline Ogre::SceneNode* getTerrainSceneNode(){return mTerrainSceneNode;}

  void update(Ogre::Real t);

  inline Ogre::Real getMorphSpeed(){return 1.0f;}
  inline Ogre::Real getTextureFadeSpeed(){ return 2.0f;}
  inline void setMorphingEnabled(bool enabled){
    mMorphingEnabled = enabled;
  }
  inline bool isMorphingEnabled() const{
    return mMorphingEnabled;
  }
  inline void setTextureFadingEnabled(bool enabled){
    if ( enabled && !mMorphingEnabled )
      OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Cannot have fading but not morphing active", "Terrain::setTextureFadingEnabled");
    mTextureFadingEnabled = enabled;
  }

  inline bool isTextureFadingEnabled() const{
    return mTextureFadingEnabled;
  }

  private:
  Ogre::SceneNode* mTerrainSceneNode;
  Quad* mQuadRoot;
  bool mMorphingEnabled;
  bool mTextureFadingEnabled;

  BaseLand mBaseLand;

  ///ifs for the data file. Opned on load
  std::ifstream mDataIFS;
  ///holds the offsets of the quads
  Index mIndex;
  TexturePalette mPalette;
};
