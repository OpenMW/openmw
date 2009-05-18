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
              Ogre::SceneNode *parent)
    : Ogre::Renderable(),
      Ogre::MovableObject(),
      mWidth(width),
      mUseSkirts(skirts),
      mDepth(depth),
      mVertexes(0),
      mIndices(0),
      mLODMorphFactor(0),
      mTextureFadeFactor(0),
      mMin(30000),
      mMax(-30000),
      mExtraMorphAmount(0),
      mHasFadePass(false),
      mQuadData(qd),
      mSegmentSize(segSize),
      mX(startX),
      mY(startY)
  {
    // From QuadSegment()
    assert(qd);
    assert(segSize>0&&segSize<=1);
    assert(mY>=0&&mY<=1);
    assert(mX>=0&&mY<=1);

#ifdef QSDEBUG
    {
      //check sizes
      const float qw = mQuadData->getVertexWidth()-1;
      const float fsw = qw*segSize;
      const int isw = (int)fsw;
      assert(fsw==isw);
    }
#endif

    //precalc offsets, as getVertex/getNormal get called a lot (1000s of times)
    computeOffsets();

    // From Quad
    node = parent->createChildSceneNode(pos);

    // From create()
    createVertexBuffer();
    calculateVertexValues();
    calculateIndexValues();
    setBounds();

    node->attachObject(this);

    createMaterial();

    if ( g_heightMap->isMorphingEnabled() &&
         mDepth != g_heightMap->getMaxDepth() )
      {
        Ogre::Technique* tech = getMaterial()->getTechnique(0);
        for ( size_t i = 0; i < tech->getNumPasses(); ++i )
          {
            assert(g_heightMap->isMorphingEnabled());
            tech->getPass(i)->setVertexProgram(MORPH_VERTEX_PROGRAM);
          }
      }

    if ( g_heightMap->isMorphingEnabled() )
      calculateDeltaValues();
  }

  ~TerrainMesh()
  {
    //deleting null values is fine iirc
    delete mIndices;

#   if ENABLED_CRASHING == 1
    {
      delete mVertexes;
    }
#   else
    {
      if ( mDepth != g_heightMap->getMaxDepth() ){
        delete mVertexes;
      }
    }
#   endif

    assert(node);

    node->detachAllObjects();
    node->getCreator()->destroySceneNode(node);
  }

  /**
   * @brief Checks if it needs to be split or unsplit and deals with
   * the morph factor. time seconds since last frame
   */
  void update(Ogre::Real time, Ogre::Real camDist, Ogre::Real usplitDist, Ogre::Real morphDist)
  {
    TRACE("TerrainMesh::update");
    //if ( USE_MORPH ){

    //as aprocesh mUnsplitDistance, lower detail
    if ( camDist > morphDist && mDepth > 1 ) {
      mLODMorphFactor = 1 - (usplitDist - camDist)/(usplitDist-morphDist);
    } else
      mLODMorphFactor = 0;
    mTextureFadeFactor = mLODMorphFactor;


    //on an split, it sets the extra morph amount to 1, we then ensure this ends up at 0... slowly
    if ( mExtraMorphAmount > 0 ) {
      mLODMorphFactor += mExtraMorphAmount;
      mExtraMorphAmount -= (time/g_heightMap->getMorphSpeed()); //decrease slowly
    }
    if ( mExtraFadeAmount > 0 ) {
      mTextureFadeFactor += mExtraFadeAmount;
      mExtraFadeAmount -= (time/g_heightMap->getTextureFadeSpeed());
    }

    //Ensure within valid bounds
    if ( mLODMorphFactor > 1 )
      mLODMorphFactor = 1;
    else if ( mLODMorphFactor < 0 )
      mLODMorphFactor = 0;

    if ( mTextureFadeFactor > 1 )
      mTextureFadeFactor = 1;
    else if ( mTextureFadeFactor < 0 )
      mTextureFadeFactor = 0;

    //}

    //remove pass. Keep outside in case terrain fading is removed while it is active
    if ( mHasFadePass && mTextureFadeFactor == 0 ) {
      removeFadePass();
    } else if ( g_heightMap->isTextureFadingEnabled() &&
                !mHasFadePass &&
                mTextureFadeFactor > 0 &&
                hasParentTexture() ) {
      addFadePass();
    }

  }


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
                                 Ogre::GpuProgramParameters* params) const
  {
    using namespace Ogre;
    if (constantEntry.data == MORPH_CUSTOM_PARAM_ID)
      params->_writeRawConstant(constantEntry.physicalIndex, mLODMorphFactor);
    else if ( constantEntry.data == FADE_CUSTOM_PARAM_ID )
      params->_writeRawConstant(constantEntry.physicalIndex, mTextureFadeFactor);
    else
      Renderable::_updateCustomGpuParameter(constantEntry, params);
  }


  /**
   * @brief sets the mExtraMorphAmount so it slowly regains detail from the lowest morph factor
   */
  void justSplit()
  {
    mExtraMorphAmount = 1;
    mLODMorphFactor = 1;
    mTextureFadeFactor = 1;
    mExtraFadeAmount = 1;

    if ( g_heightMap->isTextureFadingEnabled() && hasParentTexture() )
      addFadePass();
  }

  /**
   * @brief Does nothing
   */
  inline void justUnsplit(){}

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
  float getVertexHeight(int x, int y)
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

    /*
    ti.resize((size+2)*(size+2), -1);
    for ( int y = 0; y < size+2; ++y ){
      for ( int x = 0; x < size+2; ++x ){
        ti[(y)*(size+2)+(x)] = tref.at((y+yoff)*(indexSize)+(x+xoff));
      }
    }
    */
    ti.resize(size*size, -1);
    for ( int y = 0; y < size; ++y ){
      for ( int x = 0; x < size; ++x ){
        ti[y*size+x] = tref.at((1+y+yoff)*(indexSize)+(1+x+xoff));
      }
    }

    mMaterial = g_materialGen->getAlphaMat
      (ti,size,
       1.0f/size,
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
  void addFadePass()
  {
    assert(mHasFadePass==false);

    if ( mDepth == g_heightMap->getMaxDepth() ) return;


    mHasFadePass = true;
    Ogre::MaterialPtr mat = getMaterial();
    Ogre::Pass* newPass = mat->getTechnique(0)->createPass();
    newPass->setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

    //set fragment program
    assert(g_heightMap->isTextureFadingEnabled());
    newPass->setFragmentProgram(FADE_FRAGMENT_PROGRAM);

    if ( g_heightMap->isMorphingEnabled() && mDepth != g_heightMap->getMaxDepth() ) {
      assert(g_heightMap->isMorphingEnabled());
      newPass->setVertexProgram(MORPH_VERTEX_PROGRAM);
    }


    //set texture to be used
    newPass->createTextureUnitState(getParentTexture(), 1);
  }

  /**
   * @brief removes the last pass from the material. Assumed to be the fade pass
   */
  void removeFadePass()
  {
    assert(mHasFadePass==true);
    mHasFadePass = false;
    Ogre::MaterialPtr mat = getMaterial();
    Ogre::Technique* tech = mat->getTechnique(0);

    tech->removePass(tech->getNumPasses()-1);
  }

  /**
   * Inits the vertex stuff
   */
  void createVertexBuffer()
  {
    using namespace Ogre;

    size_t vw = mWidth;
    if ( mUseSkirts ) vw += 2;

    mVertexes = new VertexData();
    mVertexes->vertexStart = 0;
    mVertexes->vertexCount = vw*vw;// VERTEX_WIDTH;

    VertexDeclaration* vertexDecl = mVertexes->vertexDeclaration;
    size_t currOffset = 0;

    vertexDecl->addElement(MAIN_BINDING, currOffset, VET_FLOAT3, VES_POSITION);
    currOffset += VertexElement::getTypeSize(VET_FLOAT3);

    vertexDecl->addElement(MAIN_BINDING, currOffset, VET_FLOAT3, VES_NORMAL);
    currOffset += VertexElement::getTypeSize(VET_FLOAT3);


    vertexDecl->addElement(MAIN_BINDING, currOffset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    currOffset += VertexElement::getTypeSize(VET_FLOAT2);

    if ( g_heightMap->isTextureFadingEnabled() ) {
      vertexDecl->addElement(MAIN_BINDING, currOffset, VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 1);
      currOffset += VertexElement::getTypeSize(VET_FLOAT2);
    }

    mMainBuffer = HardwareBufferManager::getSingleton().createVertexBuffer(
                                                                           vertexDecl->getVertexSize(0), // size of one whole vertex
                                                                           mVertexes->vertexCount, // number of vertices
                                                                           HardwareBuffer::HBU_STATIC_WRITE_ONLY, // usage
                                                                           false); // no shadow buffer

    mVertexes->vertexBufferBinding->setBinding(MAIN_BINDING, mMainBuffer); //bind the data

    if ( g_heightMap->isMorphingEnabled() )
      vertexDecl->addElement(DELTA_BINDING, 0, VET_FLOAT1, VES_BLEND_WEIGHTS);


  }


  /**
   * @brief fills the vertex buffer with data
   * @todo I don't think tex co-ords are right
   */
  void calculateVertexValues()
  {
    //get the texture offsets for the higher uv
    float xUVOffset = 0;
    float yUVOffset = 0;

    if ( g_heightMap->isTextureFadingEnabled() ) {
      assert(0);
    }

    int start = 0;
    int end = mWidth;

    if ( mUseSkirts )
      {
        --start;
        ++end;
      }

    float* verts = static_cast<float*>(mMainBuffer->lock(HardwareBuffer::HBL_DISCARD));
    for ( int y = start; y < end; y++ ) {
      for ( int x = start; x < end; x++ ) {

        //the skirts
        if ( y < 0 || y > (mWidth-1) || x < 0 || x > (mWidth-1) ) {

          if ( x < 0 )		*verts++ = 0;
          else if ( x > (mWidth-1) )	*verts++ = (mWidth-1)*getVertexSeperation();
          else				*verts++ = x*getVertexSeperation();

          *verts++ = -4096; //2048 below base sea floor height

          if ( y < 0 )		        *verts++ = 0;
          else if ( y > (mWidth-1) )	*verts++ = (mWidth-1)*getVertexSeperation();
          else        				*verts++ = y*getVertexSeperation();


          for ( Ogre::uint i = 0; i < 3; i++ )
            *verts++ = 0;

          float u = (float)(x) / (mWidth-1);
          float v = (float)(y) / (mWidth-1);

          //clamped, so shouldn't matter if > 1

          *verts++ = u;
          *verts++ = v;

          if ( g_heightMap->isTextureFadingEnabled() ) {
            *verts++ = u;
            *verts++ = v;
          }
        } else {

          assert(y*mWidth+x>=0&&y*mWidth+x<mWidth*mWidth);

          //verts
          *verts++ = x*getVertexSeperation();
          *verts++ = getVertexHeight(x,y);
          *verts++ = y*getVertexSeperation();

          mMax = std::max(mMax, getVertexHeight(x,y));
          mMin = std::min(mMin, getVertexHeight(x,y));

          //normals
          for ( Ogre::uint i = 0; i < 3; i++ )
            *verts++ = getNormal(x,y,i);
          //*verts++ = mInterface->getNormal((y*mWidth+x)*3+i);

          const float u = (float)(x) / (mWidth-1);
          const float v = (float)(y) / (mWidth-1);
          assert(u>=0&&v>=0);
          assert(u<=1&&v<=1);

          *verts++ = u;
          *verts++ = v;

          if ( g_heightMap->isTextureFadingEnabled() ) {
            *verts++ = u/2.0f + xUVOffset;
            *verts++ = v/2.0f + yUVOffset;
          }
        }
      }
    }
    mMainBuffer->unlock();
  }

  /**
   * @brief returns a a new Vertex Buffer ready for input
   * @remarks Unlike other terrain libs, this isn't 0s when it is returend
   */
  Ogre::HardwareVertexBufferSharedPtr createDeltaBuffer()
  {
    size_t vw = mWidth;
    if ( mUseSkirts ) vw += 2;

    const size_t totalVerts = (vw * vw);
    return Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
                                                                          Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT1),
                                                                          totalVerts,
                                                                          Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                                                                          false); //no shadow buff

  }



  /**
   * @brief DOESN'T WORK FULLY
   * @todo fix
   */
#define SET_DELTA_AT(x, y, v)                       \
if ( mUseSkirts )   pDeltas[( y + 1)*vw+ x + 1] = v; \
else                pDeltas[( y)*vw+ x] = v;
  void calculateDeltaValues()
  {
    assert(0);
    /*
      size_t vw = mWidth;
      if ( mUseSkirts ) vw += 2;

      //must be using morphing to use this function
      assert(g_heightMap->isMorphingEnabled());

      const size_t step = 2;

      // Create a set of delta values
      mDeltaBuffer = createDeltaBuffer();
      float* pDeltas = static_cast<float*>(mDeltaBuffer->lock(HardwareBuffer::HBL_DISCARD));
      memset(pDeltas, 0, (vw)*(vw) * sizeof(float));

      return;

      bool flag=false;
      for ( size_t y = 0; y < mWidth-step; y += step ) {
      for ( size_t x = 0; x < mWidth-step; x += step ) {
      //create the diffrence between the full vertex if the vertex wasn't their

      float bottom_left = getVertexHeight(x,y);
      float bottom_right = getVertexHeight(x+step,y);

      float top_left = getVertexHeight(x,y+step);
      float top_right = getVertexHeight(x+step,y+step);

      //const int vw = mWidth+2;
      SET_DELTA_AT(x, y+1, (bottom_left+top_left)/2 - getVertexHeight(x, y+1)) //left
      SET_DELTA_AT(x+2, y+1, (bottom_right+top_right)/2 - getVertexHeight(x+2, y+1)) //right

      SET_DELTA_AT(x+1, y+2, (top_right+top_left)/2 - getVertexHeight(x+1, y+2)) //top
      SET_DELTA_AT(x+1, y, (bottom_right+bottom_left)/2 - getVertexHeight(x+1, y)) //bottom

      //this might not be correct
      if ( !flag )
      SET_DELTA_AT(x+1, y+1, (bottom_left+top_right)/2 - getVertexHeight(x+1, y+1)) //center
      else
      SET_DELTA_AT(x+1, y+1, (bottom_right+top_left)/2 - getVertexHeight(x+1, y+1)) //center

      flag = !flag;
      }
      flag = !flag; //flip tries for next row
      }

      mDeltaBuffer->unlock();
      mVertexes->vertexBufferBinding->setBinding(DELTA_BINDING,mDeltaBuffer);
    */
  }
#undef SET_DELTA_AT

  /**
   * @brief gets the position of a vertex. It will not interpolate
   */
  Ogre::Vector3 getVertexPosition(int x, int y)
  {
    return Ogre::Vector3(x*getVertexSeperation(), getVertexHeight(x,y) , y*getVertexSeperation());
  }

  /**
   * @brief gets the indicies for the vertex data.
   */
  void calculateIndexValues()
  {
    size_t vw = mWidth-1;
    if ( mUseSkirts ) vw += 2;

    const size_t indexCount = (vw)*(vw)*6;

    //need to manage allocation if not null
    assert(mIndices==0);

    mIndices = new IndexData();
    mIndices->indexCount = indexCount;
    mIndices->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
                                                                                    HardwareIndexBuffer::IT_16BIT,
                                                                                    indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);

    unsigned short* indices = static_cast<unsigned short*>(mIndices->indexBuffer->lock(0,
                                                                                       mIndices->indexBuffer->getSizeInBytes(),
                                                                                       HardwareBuffer::HBL_DISCARD));

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
    mIndices->indexBuffer->unlock();
    //return mIndices;
  }


  /*
   * Sets the bounds on the renderable. This requires that mMin/mMax
   * have been set. They are set in createVertexData. This may look
   * as though it is done twice, as it is also done in MeshInterface,
   * however, there is no guarentee that the mesh sizes are the same
   */
  void setBounds()
  {
    mBounds.setExtents(0,mMin,0,
                       (mWidth - 1) * getVertexSeperation(),
                       mMax,
                       (mWidth - 1) * getVertexSeperation());

    mCenter = Ogre::Vector3( ( (mWidth  - 1) * getVertexSeperation() ) / 2,
                             ( mMin + mMax ) / 2,
                             ( (mWidth - 1) * getVertexSeperation() ) / 2);

    mBoundingRadius = (mBounds.getMaximum() - mBounds.getMinimum()).length() / 2;
  }


  Ogre::SceneNode* node;

  const int mWidth;
  const bool mUseSkirts;

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
