/**
 * @brief the class that deals with loading quads from the disk @todo a
 * major improvment would be to store the data as a quad tree. It might
 * improve lookup times. Then again. Might not
 */
class MWHeightmap : public TerrainHeightmap
{
public:
  QuadData* getData(Quad* q)
  {
    MWQuadData* data = loadQuad(q->getPosition().x,q->getPosition().y);

    if ( !data )
      assert(0);

    return data;
  }

  inline bool hasData(Quad* q) {
    return hasQuad(q->getPosition().x,q->getPosition().y);
  }

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
    mMaterialGen.setTexturePaths(mPalette.getPalette());

    mMaterialGenerator = new MWQuadMaterialGen(&mMaterialGen);

    mDataIFS.open(std::string(fn + ".data").c_str(), std::ios::binary);
    return true;
  }

private:

  inline long hasQuad(long x, long y) {
    return (mIndex.getOffset(x,y) != -1 ) ;
  }

  /**
   * @brief loads the quad data from the disk
   */
  MWQuadData* loadQuad(long x, long y)
  {
    long offset = mIndex.getOffset(x,y);
    if ( offset == -1 ) //check we have xy
      return 0;

    mDataIFS.seekg(offset);

    //load the quad from the file
    MWQuadData* q = new MWQuadData(this);
    boost::archive::binary_iarchive oa(mDataIFS);
    oa >> *q;
    return q;
  }

  ///ifs for the data file. Opned on load
  std::ifstream mDataIFS;
  ///holds the offsets of the quads
  Index mIndex;
  TexturePalette mPalette;
  ///material generator. gens a ogre::material from quad data
  MaterialGenerator mMaterialGen;
};
