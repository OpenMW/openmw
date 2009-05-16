/**
 * @brief Terrain for one cell. Handles building, destroying and LOD morphing
 */
#define QSDEBUG
class TerrainMesh : public Ogre::Renderable, public Ogre::MovableObject
{
 public:

  TerrainMesh(QuadData* qd, float segSize, float startX, float startY,
              const Ogre::Vector3 &pos,
              int width, int depth, bool skirts,
              Ogre::SceneNode *parent);

  ~TerrainMesh()
    {
      destroy();

      assert(node);

      node->detachAllObjects();
      node->getCreator()->destroySceneNode(node);
    }

  /**
   * Can be called manually and can be called from the destuctor. This
   * destroyes the mesh
   */
  // FIXME: We don't need this as a separate function.
  void destroy();

  /**
   * @brief Checks if it needs to be split or unsplit and deals with
   * the morph factor. time seconds since last frame
   */
  void update(Ogre::Real time, Ogre::Real camDist, Ogre::Real usplitDist, Ogre::Real morphDist);

  /**
   * @todo Needs to work out what this does (if it does what it is meant to)
   */
  void visitRenderables(Renderable::Visitor* visitor,
                        bool debugRenderables = false) {
    visitor->visit(this, 0, false);
  }

  virtual const Ogre::MaterialPtr& getMaterial( void ) const {
    return mMaterial;
  }
  //-----------------------------------------------------------------------
  void getRenderOperation( Ogre::RenderOperation& op ) {
    op.useIndexes = true;
    op.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
    op.vertexData = mVertexes;
    op.indexData = mIndices;
  }

  //-----------------------------------------------------------------------
  void getWorldTransforms( Ogre::Matrix4* xform ) const {
    *xform = mParentNode->_getFullTransform();
  }
  //-----------------------------------------------------------------------
  const Ogre::Quaternion& getWorldOrientation(void) const {
    return mParentNode->_getDerivedOrientation();
  }
  //-----------------------------------------------------------------------
  const Ogre::Vector3& getWorldPosition(void) const {
    return mParentNode->_getDerivedPosition();
  }
  //-----------------------------------------------------------------------

  Ogre::Real getSquaredViewDepth(const Ogre::Camera *cam) const {
    Ogre::Vector3 diff = mCenter - cam->getDerivedPosition();
    // Use squared length to avoid square root
    return diff.squaredLength();
  }
  //-----------------------------------------------------------------------

  const Ogre::LightList& getLights(void) const {
    if (mLightListDirty) {
      getParentSceneNode()->getCreator()->_populateLightList(
                                                             mCenter, this->getBoundingRadius(), mLightList);
      mLightListDirty = false;
    }
    return mLightList;
  }
  //-----------------------------------------------------------------------
  virtual const Ogre::String& getMovableType( void ) const {
    static Ogre::String t = "MW_TERRAIN";
    return t;
  }
  //-----------------------------------------------------------------------
  void _updateRenderQueue( Ogre::RenderQueue* queue ) {
    mLightListDirty = true;
    queue->addRenderable(this, mRenderQueueID);
  }

  const Ogre::AxisAlignedBox& getBoundingBox( void ) const {
    return mBounds;
  };
  //-----------------------------------------------------------------------
  Ogre::Real getBoundingRadius(void) const {
    return mBoundingRadius;
  }
  //-----------------------------------------------------------------------

  /**
   * @brief passes the morph factor to the custom vertex program
   */
  void _updateCustomGpuParameter(const Ogre::GpuProgramParameters::AutoConstantEntry& constantEntry,
                                 Ogre::GpuProgramParameters* params) const;

  /**
   * @brief sets the mExtraMorphAmount so it slowly regains detail from the lowest morph factor
   */
  void justSplit();
  /**
   * @brief Does nothing
   */
  inline void justUnsplit(){
  }

  inline float getMax(){
    return mMax;
  }
  inline float getMin(){
    return mMin;
  }


  /**
   * Gets how many vertexes wide this quad segment is. Should always be 2^n+1
   * @return the vertex width of this quad segment
   */
  int getVertexWidth()
  {
    return (mQuadData->getVertexWidth()-1)*mSegmentSize+1;
  }

  /**
   * @brief gets a vertex assuming that x = 0, y = 0 addresses the start of the quad
   */
  float getVertex(int x, int y)
  {
#ifdef QSDEBUG
    {
      const int vw = getVertexWidth();
      assert(x<vw);
    }
#endif
    return mQuadData->getVertex((mYOffset + y)*mQuadData->getVertexWidth()+(mXOffset+x));
  }

  float getNormal(int x, int y, int z)
  {
    assert(z>=0&&z<3);
    return mQuadData->getNormal(((mYOffset + y)*mQuadData->getVertexWidth()+(mXOffset+x))*3+z);
  }

 private:

  void createMaterial()
  {
    assert(mSegmentSize>0);
    if ( mSegmentSize == 1 ) //we can use the top level material
      {
        mMaterial = mQuadData->getMaterial();
        return;
      }

    if ( mQuadData->getTexture().length() )
      assert(0&&"NOT IMPLEMENTED");

    const std::vector<int>& tref = mQuadData->getTextureIndexRef();
    const int indexSize = sqrt(tref.size());
    const int cellIndexSize = indexSize - 2;

    //plus 1 to take into account border
    const int xoff = float(cellIndexSize) * mX;
    const int yoff = float(cellIndexSize) * mY;
    const int size = float(cellIndexSize) * mSegmentSize;

    std::vector<int> ti;

    ti.resize((size+2)*(size+2), -1);

    for ( int y = 0; y < size+2; ++y ){
      for ( int x = 0; x < size+2; ++x ){
        ti[(y)*(size+2)+(x)] = tref.at((y+yoff)*(indexSize)+(x+xoff));
      }
    }

    mMaterial = g_materialGen->getAlphaMat
      (ti,size,
       1, 1.0f/size,
       mQuadData->getUsedResourcesRef());
  }

  inline bool hasParentTexture() const{
    return mQuadData->hasParentTexture();
  }
  inline const std::string& getParentTexture() const{
    return mQuadData->getParentTexture();
  }
  inline int getVertexSeperation(){
    return mQuadData->getVertexSeperation();
  }

  inline float getSegmentSize(){
    return mSegmentSize;
  }
  inline float getStartX(){
    return mX;
  }
  inline float getStartY(){
    return mY;
  }

  /**
   * @brief Adds another pass to the material to fade in/out the material from a higher level
   */
  void addFadePass();
  /**
   * @brief removes the last pass from the material. Assumed to be the fade pass
   */
  void removeFadePass();

  /**
   * @return the height at the given vertex
   */
  float getVertexHeight(int x, int y);

  /**
   * Inits the vertex stuff
   */
  void createVertexBuffer();

  /**
   * @brief fills the vertex buffer with data
   * @todo I don't think tex co-ords are right
   */
  void calculateVetexValues();

  /**
   * @brief returns a a new Vertex Buffer ready for input
   * @remarks Unlike other terrain libs, this isn't 0s when it is returend
   */
  Ogre::HardwareVertexBufferSharedPtr createDeltaBuffer( );

  /**
   * @brief DOESN'T WORK FULLY
   * @todo fix
   */
  void calculateDeltaValues();
  /**
   * @brief gets the position of a vertex. It will not interpolate
   */
  Ogre::Vector3 getVertexPosition(int x, int y);

  /**
   * @brief gets the indicies for the vertex data.
   */
  void calculateIndexValues();

  /*
   * Sets the bounds on the renderable. This requires that mMin/mMax
   * have been set. They are set in createVertexData. This may look
   * as though it is done twice, as it is also done in MeshInterface,
   * however, there is no guarentee that the mesh sizes are the same
   */
  void setBounds();

  Ogre::SceneNode* node;

  const int mWidth;
  const bool mUseSkirts;

  ///true if the land has been built
  bool mBuilt;

  int mDepth;

  Ogre::MaterialPtr mMaterial;
  Ogre::ManualObject* mObject;

  Ogre::VertexData* mVertexes;
  Ogre::IndexData* mIndices;
  Ogre::HardwareVertexBufferSharedPtr mMainBuffer;
  Ogre::HardwareVertexBufferSharedPtr mDeltaBuffer;

  mutable bool mLightListDirty;

  Ogre::Real mBoundingRadius;
  Ogre::AxisAlignedBox mBounds;
  Ogre::Vector3 mCenter;

  Ogre::Real mLODMorphFactor, mTextureFadeFactor;

  /** Max and min heights of the mesh */
  float mMin, mMax;

  int computeOffset(float x)
  {
#ifdef QSDEBUG
    {
      //check it is a valid position
      const int start = (mQuadData->getVertexWidth()-1)*x;
      const int vw = getVertexWidth()-1;
      assert(vw>0);
      assert((start%vw)==0);
    }
#endif
    return float((mQuadData->getVertexWidth()-1))*x;
  }

  void computeOffsets()
  {
    mXOffset = computeOffset(mX);
    mYOffset = computeOffset(mY);
  }

  QuadData* mQuadData;
  float mSegmentSize;
  float mX, mY;
  int mXOffset, mYOffset;

  /**
   * @brief holds the amount to morph by, this decreases to 0 over a periord of time
   * It is changed and used in the update() function
   */
  Ogre::Real mExtraMorphAmount, mExtraFadeAmount;

  /**
   * True if the fade pass has been added to the material.
   */
  bool mHasFadePass;

  //Custom params used in terrian shaders.
  static const size_t MORPH_CUSTOM_PARAM_ID = 77;
  static const size_t FADE_CUSTOM_PARAM_ID = 78;

  //Terrain bindings
  static const int MAIN_BINDING = 0;
  static const int DELTA_BINDING = 1;
};
