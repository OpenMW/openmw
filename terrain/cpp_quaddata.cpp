/**
 * holds data that is passed to the mesh renderer. heights normals etc
 *
 * This needs a rework, as really the mesh renderer should accept just
 * a set of verts Normals and indicies to allow us to pass optimized
 * meshes
 */
class QuadData
{
  typedef std::list<Ogre::ResourcePtr> ResourceList;
  typedef std::list<Ogre::ResourcePtr>::const_iterator ResourceListCItr;

public:
  virtual ~QuadData()
  {
    const ResourceListCItr end = mResources.end();
    for ( ResourceListCItr itr = mResources.begin(); itr != end; ++itr )
      (*itr)->getCreator()->remove((*itr)->getHandle());
  }

  /**
   * How many vertes wide the qd is. Usally 65.
   * @todo cache?
   */
  inline int getVertexWidth() {
    return sqrt(getHeightsRef().size());
  }

  inline std::vector<float>& getHeightsRef() {
    return mHeights;
  }
  inline float getVertex(int offset){
    return getHeightsRef().at(offset);
  }
  inline std::vector<char>& getNormalsRef() {
    return mNormals;
  }
  inline float getNormal(int offset){
    return getNormalsRef().at(offset);
  }

  inline ResourceList& getUsedResourcesRef()
  { return mResources; }

  inline void setTexture(const std::string& t)
  { mTexture = t; }

  inline std::string& getTexture()
  { return mTexture; }

  inline std::vector<int>& getTextureIndexRef()
  { return mTextureIndex; }

  /**
   * @brief should be removed when we get around to developing optimized meshes
   */
  inline int getVertexSeperation() {
    return mVertexSeperation;
  }
  inline void setVertexSeperation(int vs) {
    mVertexSeperation = vs;
  }

  /**
   * @brief lazy get function. Avoids creating material until requested
   */
  inline Ogre::MaterialPtr getMaterial() {
    if ( mMaterial.isNull() )
      createMaterial();
    assert(!mMaterial.isNull());
    return mMaterial;
  }

  /**
   * @brief gets the texture for the above quad
   */
  inline const std::string& getParentTexture() const {
    return mParentTexture;
  }
  inline bool hasParentTexture() const {
    return (mParentTexture.length() > 0);
  }
  inline void setParentTexture(const std::string& c) {
    mParentTexture = c;
  }

private:
  void createMaterial()
  {
    assert(mTexture.length());

    mMaterial = g_materialGen->generateSingleTexture(mTexture, mResources);
  }

  Ogre::MaterialPtr mMaterial;

  std::string mParentTexture;

  int mVertexSeperation;
  std::vector<float> mHeights;
  std::vector<char> mNormals;

  ///Holds the resources used by the quad
  ResourceList mResources;

  std::vector<int> mTextureIndex; ///holds index that correspond to the palette
  std::string mTexture; ///The land texture. Mutally exclusive with the above

  friend class boost::serialization::access;

  /**
   * Saves the data for the heights, noramals and texture indexies.
   * Texture as well
   */
  template<class Archive>
  inline void serialize(Archive& ar, const unsigned int version){
    ar &mVertexSeperation;
    ar &mHeights;
    ar &mNormals;
    ar &mParentTexture;

    ar &mTextureIndex;
    ar &mTexture;
  }
};

BOOST_CLASS_TRACKING(QuadData, boost::serialization::track_never);
