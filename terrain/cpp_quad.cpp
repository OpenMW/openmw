/**
 * defines an area of Landscape
 *
 * A quad can either hold a mesh, or 4 other sub quads The functions
 * split and unsplit either break the current quad into smaller quads,
 * or alternatively remove the lower quads and create the terrain mesh
 * on the the current (now the lowest) level
 *
 * needUnsplit and needSplit query the state of the meshes to see if
 * it needs spliting or unspliting
 *
 */
/* Previously for MeshInterface:
 * Interface between the quad and the terrain renderble classes, to the
 * quad it looks like this rendereds a single mesh for the quad. This
 * may not be the case.
 *
 * It also could allow several optimizations (e.g. multiple splits)
 */
class Quad
{

  /**
   * when each quad is split, the children can be one of 4 places,
   * topleft (NW) top right (NE) etc etc. The other position is the
   * root which is the top quad.  The root quad doesn't have a mesh,
   * and should always have 4 children.
   */

  typedef std::list<TerrainMesh*> MeshList;

public:

  // FIXME: There's probably a better way to do this
  enum QuadLocation { QL_NW, QL_NE, QL_SW, QL_SE, QL_ROOT };
  /**
   * @param l the location of the quad
   * @param p the parent quad. Leave 0 if it is root
   * @param t the terrain object
   *
   * Constructor mainly sets up the position variables/depth etc
   */
  Quad(QuadLocation l, Quad* p)
    : mParent(p), mLocation(l), mQuadData(NULL)
  {
    TRACE("Quad");
    //as mentioned elsewhere, the children should all be null.
    memset(mChildren, NULL, sizeof(Quad*)*NUM_CHILDREN);

    //find the location of the quad
    if ( l != Quad::QL_ROOT )
      { //if it isn't the root node
        mDepth = p->getDepth() + 1;

        mPosition = p->getPosition();
        mSideLength = p->getSideLength()/2;

        //horrible bit of code
        // FIXME
        switch (l) {
        case Quad::QL_NE:
          mPosition.x += mSideLength/2;
          mPosition.y += mSideLength/2;
          break;
        case Quad::QL_NW:
          mPosition.x -= mSideLength/2;
          mPosition.y += mSideLength/2;
          break;
        case Quad::QL_SE:
          mPosition.x += mSideLength/2;
          mPosition.y -= mSideLength/2;
          break;
        case Quad::QL_SW:
          mPosition.x -= mSideLength/2;
          mPosition.y -= mSideLength/2;
          break;
        default:
          break;
        }

        //set after positions have been retrived
        mMaxDepth = g_heightMap->getMaxDepth();
        mHasData = g_heightMap->hasData(mPosition.x, mPosition.y);

        if ( needSplit() ) //need to "semi" build terrain
          split();
        else if ( mHasData )
          {
            buildTerrain();
            justSplit();
          }
      }
    else
      { //assume it is root node, get data and position
        mDepth = 0; //root
        mSideLength = g_heightMap->getRootSideLength();
        mPosition = Point2<long>(0,0);

        mHasData = false;

        //always split, as this node never has data
        split();
      }
  }

  /**
   * Destroyes the terrain mesh OR all children.
   * If it needs to do both, there is a bug, as that would lead to a terrain mesh existing
   * On more than one level in the same space.
   * assert checks that there is only one or the other
   */
  ~Quad()
  {
    TRACE("~Quad");
    destroyTerrain();
    for (size_t i = 0; i < NUM_CHILDREN; i++)
      delete mChildren[i];
  }

  /**
   * @return true if the node needs to be split.
   *
   * The issue with this is that at present this requires the mesh to
   * be built to check. This is fine but it could lead to a lot more
   * loading when teleporting
   */
  bool needSplit()
  {
    TRACE("needSplit");
    if ( hasChildren() ||
         getDepth() == mMaxDepth ||
         !hasData() )
      return false;
    return ( mQuadData && (mSplitState == SS_SPLIT) );
  }

  /**
   * Deletes the landscape, if there is any
   *   Creates children, and either splits them, or creates landscape for them
   */
  void split()
  {
    TRACE("split");
    destroyTerrain();

    //create a new terrain
    for ( size_t i = 0; i < NUM_CHILDREN; ++i )
      mChildren[i] = new Quad((QuadLocation)i, this);

    assert(!needUnsplit());
  }

  /**
   * @brief removes all children, and builds terrain on this level
   */
  void unsplit()
  {
    TRACE("unsplit");
    //shouldn't unsplit 0 depth
    assert(getDepth());

    for ( size_t i = 0; i < NUM_CHILDREN; i++ )
      delete mChildren[i];
    memset(mChildren, NULL, sizeof(Quad*)*NUM_CHILDREN);

    if ( mHasData )
      {
        buildTerrain();
        justUnsplit();
      }

    assert(!needSplit());
  }

  /**
   * @return true if the node needs to delete all its child nodes, and
   * rebuild the terrain its level
   */
  bool needUnsplit()
  {
    TRACE("needUnsplit");
    if ( hasChildren() && getDepth() )
      {
        for (size_t i=0;i< NUM_CHILDREN;i++)
          {
            if ( mChildren[i]->hasData()  )
              {
                if ( !mChildren[i]->hasMesh() )
                  return false;
                else if ( mChildren[i]->getSplitState() != SS_UNSPLIT)
                  return false;
              }
          }
        return true;
      }

    //get depth ensures the root doesn't try and unsplit
    if ( getDepth() && !hasData() )
      return true;

    return false;
  }

  /**
   * @brief constructs the terrain on this level. The terrain must on
   * exist before hand
   */
  void buildTerrain()
  {
    TRACE("buildTerrain");
    assert(!mQuadData);
    assert(hasData());

    // This was in MeshInterface().

    mMax = 0;
    mMin = 0;
    mSplitState = SS_NONE;

    long qx = mPosition.x;
    long qy = mPosition.y;
    mQuadData = g_heightMap->getData(qx, qy);

    //the mesh is created at zero, so an offset is applied
    const Ogre::Vector3 pos(qx - mSideLength/2,
                            0,qy - mSideLength/2);

    mSceneNode = g_heightMap->getTerrainSceneNode()->createChildSceneNode(pos);

    // This was in create()

    if ( mDepth == g_heightMap->getMaxDepth() )
      for ( int y = 0; y < 4; ++y )
        for ( int x = 0; x < 4; ++x )
          {
            addNewObject(Ogre::Vector3(x*16*128, 0, y*16*128), //pos
                         17, //size
                         false, //skirts
                         0.25f, float(x)/4.0f, float(y)/4.0f);//quad seg location
          }
    else
      addNewObject(Ogre::Vector3(0,0,0), 65);

    getBounds();
  }

  /**
   * @brief destroys the terrain.
   */
  void destroyTerrain()
  {
    TRACE("destroyTerrain");
    if(!mQuadData)
      return;

    // From ~MeshInterface()
    for ( MeshList::iterator itr =
            mMeshList.begin();
          itr != mMeshList.end();
          ++itr )
      delete *itr;
    mMeshList.clear();

    mSceneNode->removeAndDestroyAllChildren();
    mSceneMgr->destroySceneNode(mSceneNode);

    g_heightMap->getTerrainSceneNode()->detachAllObjects();

    delete mQuadData;
    mQuadData = NULL;
  }

  /**
   * @brief gets the position in relation to the root (always 0,0)
   * @return the position as a long in a container holding the .x and .y vals
   *
   * This is called form the subnodes of this node, and the TerrainRenderable to work out what needs positiong where
   *  This is a long (as opposed to a float) so it can be used in comparisons
   * @todo typedef this, so it can be changed to int or long long (long64)
   *
   * The roots position is always 0. This is as the roots position is totally independent of the position
   * that the terrain is rendered, as you can just move the root terrain node.
   * In other words, it makes everything simpler
   *
   * The position of the quad is always taken from the center of the quad. Therefore, a top left quads location can be
   * defined as:
   * xpos = parent x pos - sidelength/2
   * ypos = parent y pos + sidelength/2
   *
   * Where the side length is half the parents side length.
   * The calcs are all handled in the consturctor (Quad::Quad)
   */
  inline Point2<long> getPosition() const{ return mPosition; }

  /**
   * @return the location of the quad. E.g. North West
   */
  inline QuadLocation getLocation() const{ return mLocation; }

  /**
   * @brief simply the length of one side of the current quad.
   * @return the side length of the current quad
   *
   * As all quads are square, all sides are this length.
   * Obviously this has to fit in sizeof(long)
   */
  inline long getSideLength() const{ return mSideLength;}

  /**
   * @brief The depth is how many splits have taken place since the root node.
   * @return the depth of the current quad
   *
   * The root node has a depth 0. As this is the case, all of its children will have
   * a depth of one. Simply depth = parent depth + 1
   *
   * Set in the consturctor (Quad::Quad)
   */
  inline int getDepth() const{return mDepth;}

  /**
   * @return true if their is a terrain mesh alocated
   */
  inline bool hasMesh() const{ return mQuadData; }

  /**
   * @return true if there are any children
   */
  inline bool hasChildren() const { return mChildren[0] != 0; }

  /**
   * @brief checks if the quad has any data (i.e. a mesh avaible for rendering
   */
  inline bool hasData() const{ return mHasData; }


  /**
   * @brief updates all meshes.
   * @remarks the camera distance is calculated here so that all terrain has the correct morph levels etc
   */
  void update(Ogre::Real time)
  {
    TRACE("Quad::update");
    if (  needSplit()  )
      {
        split();
        return;
      }
    else if ( needUnsplit() )
      {
        unsplit();
        return;
      }

    //deal with updating the mesh.
    if ( !mQuadData )
      {
        // We don't have a mesh
        if ( hasChildren() )
          {
            for (size_t i = 0; i < NUM_CHILDREN; ++i) {
              assert( mChildren[i] );
              mChildren[i]->update(time);
            }
          }
        return;
      }

    // We have a mesh. Update it.

    const Ogre::Vector3 cpos = mCamera->getDerivedPosition();
    Ogre::Vector3 diff(0, 0, 0);

    //copy?
    Ogre::AxisAlignedBox worldBounds = mBounds;
    worldBounds.transformAffine(mSceneNode->_getFullTransform());

    diff.makeFloor(cpos - worldBounds.getMinimum() );
    diff.makeCeil(cpos - worldBounds.getMaximum() );
    const Ogre::Real camDist = diff.squaredLength();

    mSplitState = SS_NONE;
    if ( camDist < mSplitDistance )             mSplitState = SS_SPLIT;
    else if ( camDist > mUnsplitDistance )      mSplitState = SS_UNSPLIT;

    for ( MeshList::iterator itr = mMeshList.begin();
          itr != mMeshList.end();
          ++itr )
      {
        assert(*itr);
        (*itr)->update(time, camDist, mUnsplitDistance, mMorphDistance);
      }
  }

  inline SplitState getSplitState() {
    return mSplitState;
  }

  /**
   * @brief propergates the just split through all terrain
   */
  inline void justSplit() {
    for ( MeshList::iterator itr = mMeshList.begin();
          itr != mMeshList.end();
          ++itr )
      (*itr)->justSplit();
  }
  /**
   * @brief propergates the just unsplit through all terrain
   */
  inline void justUnsplit() {
    for ( MeshList::iterator itr = mMeshList.begin();
          itr != mMeshList.end();
          ++itr )
      (*itr)->justUnsplit();
  }

private:

  ///Must be a ptr, else it destorys before we are ready
  MeshList mMeshList;

  Ogre::SceneNode* mSceneNode;

  ///use for split distances
  Ogre::Real mBoundingRadius;
  Ogre::AxisAlignedBox mBounds;

  ///max and min heights
  float mMax, mMin;

  Ogre::Real mSplitDistance,mUnsplitDistance,mMorphDistance;

  SplitState mSplitState;
  QuadData* mQuadData;

  /**
   * @brief sets the bounds and split radius of the object
   */
  void getBounds()
  {
    mBounds.setExtents( 0,
                        mMin,
                        0,
                        (65 - 1) * mQuadData->getVertexSeperation(),
                        mMax,
                        (65 - 1) * mQuadData->getVertexSeperation());

    mBoundingRadius = (mBounds.getMaximum() - mBounds.getMinimum()).length() / 2;

    mSplitDistance = pow(mBoundingRadius * SPLIT_FACTOR, 2);
    mUnsplitDistance = pow(mBoundingRadius * UNSPLIT_FACTOR, 2);
    mMorphDistance = pow(mBoundingRadius * 1.5, 2);
  }

  /**
   * @brief Adds a new mesh
   * @param pos the position in relation to mSceneNode
   * @param terrainSize the size of the terrain in verts. Should be n^2+1
   * @param skirts true if the terrain should have skirts
   * @param segmentSize the size of the segment. So if splitting terrain into 4*4, it should be 0.25
   * @param startX, startY the start position of this segment (0 <= startX < 1)
   */
  void addNewObject(const Ogre::Vector3& pos,  int terrainSize,
                    bool skirts = true, float segmentSize = 1,
                    float startX = 0, float startY = 0 )
  {
    assert(mQuadData);

    TerrainMesh *tm = new TerrainMesh(mQuadData, segmentSize,
                                      startX, startY, pos,
                                      terrainSize, mDepth, skirts,
                                      mSceneNode);

    mMax = std::max(tm->getMax(), mMax);
    mMin = std::max(tm->getMin(), mMin);

    mMeshList.push_back(tm);
  }

  static const size_t NUM_CHILDREN = 4;

  Quad* mParent; /// this is the node above this. 0 if this is root
  Quad* mChildren[NUM_CHILDREN]; ///optionaly the children. Should be
                                 ///0 if not exist

  Quad::QuadLocation mLocation; ///the location within the quad (ne, se, nw, sw). See Quad::QuadLocation
  Point2<long> mPosition; ///the center of the mesh. this is a long so can be used as comparison. See Quad::getPosition
  long mSideLength; ///the length in units of one side of the quad. See Quad::getSideLength
  int mDepth; ///depth of the node. See Quad::getDepth for more info

  bool mHasData; ///holds if there is terrain data about this quad
  int mMaxDepth; ///the maxmium depth. Cached. This is not valid is mDepth == 0
};
