/**
 * @brief gets a material for the quad data
 */
class QuadMaterialGenerator
{
public:
  virtual Ogre::MaterialPtr getMaterial(QuadData* qd) = 0;
  /**
   * @brief can't overload :(
   */
  virtual Ogre::MaterialPtr getMaterialSegment(QuadData* qd,QuadSegment* qs) = 0;
private:
  /**
   * @brief fix for gcc not putting the vtable anywhere unless there is a none inline, none virtual function
   *   @remarks does absolutly nothing ofc
   */
  void _vtablefix();
};

/**
 * @brief abstract class used for getting LOD data.
 *
 * This class enables storing of data in whatever form is wanted
 */
class TerrainHeightmap {
public:
  TerrainHeightmap() : mMaterialGenerator(0){
  }
  virtual ~TerrainHeightmap() {
    delete mMaterialGenerator;
  }

  /**
   * @brief called to load some data for the given quad
   * @return NULL if the data doesn't exist
   *
   * The deleting of the memory is handled by TerrainMesh
   */
  virtual QuadData* getData(Quad* q) = 0;

  /**
   * @brief check if any data exists for this level
   * @param q the quad that is asking for the data
   */
  virtual bool hasData(Quad* q) = 0;

  /**
   * @brief get the distance from one end of the map to the other
   */
  virtual long getRootSideLength() = 0;
  virtual int getMaxDepth() = 0;

  inline QuadMaterialGenerator* getMaterialGenerator(){
    assert(mMaterialGenerator);
    return mMaterialGenerator;
  }
protected:
  QuadMaterialGenerator* mMaterialGenerator;
};

/**
 * @brief holds data that is passed to the mesh renderer. heights normals etc
 *
 * This needs a rework, as really the mesh renderer should accept just a set of verts
 * Normals and indicies to allow us to pass optoizied meshes
 */
class QuadData
{
public:
  QuadData(TerrainHeightmap* p)
    : mHeightmap(p) {}

  virtual ~QuadData() {}

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

  /**
   * @brief should be removed when we get around to developing optomized meshes
   */
  inline int getVertexSeperation() {
    return mVertexSeperation;
  }
  inline void setVertexSeperation(int vs) {
    mVertexSeperation = vs;
  }

  /**
   * @brief lazy get function. Avoids creating materail until requested
   */
  inline Ogre::MaterialPtr getMaterial() {
    if ( mMaterial.isNull() )
      createMaterial();
    assert(!mMaterial.isNull());
    return mMaterial;
  }
  QuadMaterialGenerator* getMaterialGenerator()
  {
    return mHeightmap->getMaterialGenerator();
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

protected:
  void createMaterial()
  {
    mMaterial = getMaterialGenerator()->getMaterial(this);
  }
  TerrainHeightmap* mHeightmap;
  Ogre::MaterialPtr mMaterial;

  std::string mParentTexture;

  int mVertexSeperation;
  std::vector<float> mHeights;
  std::vector<char> mNormals;
};

class MWQuadData : public QuadData
{
public:
  typedef std::list<Ogre::ResourcePtr> ResourceList;
  typedef std::list<Ogre::ResourcePtr>::const_iterator ResourceListCItr;

  MWQuadData(TerrainHeightmap* thm) : QuadData(thm) {}

  ~MWQuadData ()
  {
    const ResourceListCItr end = mResources.end();
    for ( ResourceListCItr itr = mResources.begin(); itr != end; ++itr )
      (*itr)->getCreator()->remove((*itr)->getHandle());
  }

  /**
   * @return a ref to the list of used resourcs
   */
  inline ResourceList& getUsedResourcesRef()
  { return mResources; }

  inline void setTexture(const std::string& t)
  { mTexture = t; }

  inline std::string& getTexture()
  { return mTexture; }

  inline std::vector<int>& getTextureIndexRef()
  { return mTextureIndex; }

private:
  ///Holds the resources used by the quad
  ResourceList mResources;

  std::vector<int> mTextureIndex; ///holds index that corespond the the palette
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

BOOST_CLASS_TRACKING(MWQuadData, boost::serialization::track_never);
