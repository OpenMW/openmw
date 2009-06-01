/**
 * defines an area of Landscape
 *
 * A quad can either hold a mesh, or 4 other sub quads The functions
 * split and unsplit either break the current quad into smaller quads,
 * or alternatively remove the lower quads and create the terrain mesh
 * on the the current (now the lowest) level
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
  typedef std::list<TerrainMesh*> MeshList;

public:

  Quad(int cellX=0, int cellY=0, Quad* parent = NULL)
    : mCellX(cellX),
      mCellY(cellY)
  {
    RTRACE("Quad");

    memset(mChildren, NULL, sizeof(Quad*)*NUM_CHILDREN);

    hasMesh = false;
    hasChildren = false;
    isStatic = false;

    // Do we have a parent?
    if(parent != NULL)
      {
        mLevel = parent->mLevel-1;

        if(mLevel == 1)
          {
            // Create the terrain and leave it there.
            buildTerrain();
            isStatic = true;
          }

        // Coordinates relative to our parent
        const int relX = cellX - parent->mCellX;
        const int relY = cellY - parent->mCellY;

        // The coordinates give the top left corner of the quad, or our
        // relative coordinates within that should always be positive.
        assert(relX >= 0);
        assert(relY >= 0);

        // Create a child scene node. The scene node position is given in
        // world units, ie. CELL_WIDTH units per cell.
        const Ogre::Vector3 pos(relX * CELL_WIDTH,
                                relY * CELL_WIDTH,
                                0);
        mSceneNode = parent->mSceneNode->createChildSceneNode(pos);

        // Get the archive data for this quad.
        mInfo = g_archive.getQuad(mCellX,mCellY,mLevel);
      }
    else
      {
        // No parent, this is the top-most quad. Get all the info from
        // the archive.
        mInfo = g_archive.rootQuad;

        mLevel = mInfo->level;
        cellX = mCellX = mInfo->cellX;
        cellY = mCellY = mInfo->cellY;

        const Ogre::Vector3 pos(cellX * CELL_WIDTH,
                                cellY * CELL_WIDTH,
                                0);
        mSceneNode = g_rootTerrainNode->
          createChildSceneNode(pos);

        // Split up
        split();

        // The root can never be unsplit
        isStatic = true;
      }

    assert(mLevel >= 1);
    assert(mSceneNode != NULL);

    // Set up the bounding box. Use MW coordinates all the way
    mBounds.setExtents(0,0,mInfo->minHeight,
                       mInfo->worldWidth,mInfo->worldWidth,
                       mInfo->maxHeight);

    // Transform the box to world coordinates, so it can be compared
    // with the camera later.
    mBounds.transformAffine(mSceneNode->_getFullTransform());

    const float radius = mInfo->boundingRadius;

    mSplitDistance   = radius * SPLIT_FACTOR;
    mUnsplitDistance = radius * UNSPLIT_FACTOR;

    // Square the distances
    mSplitDistance *= mSplitDistance;
    mUnsplitDistance *= mUnsplitDistance;

    // Update the terrain. This will create the mesh or children if
    // necessary.
    update();
  }

  ~Quad()
  {
    RTRACE("~Quad");
    if(hasMesh)
      destroyTerrain();
    else if(hasChildren)
      for (size_t i = 0; i < NUM_CHILDREN; i++)
        delete mChildren[i];
    
    mSceneNode->removeAndDestroyAllChildren();
    mSceneMgr->destroySceneNode(mSceneNode);
  }

  // Remove the landscape for this quad, and create children.
  void split()
  {
    RTRACE("split");

    // Never split a static quad or a quad that already has children.
    assert(!isStatic);
    assert(!hasChildren);
    assert(mLevel > 1);

    if(hasMesh)
      destroyTerrain();

    // Find the cell width of our children
    int cWidth = 1 << (mLevel-2);

    // Create children
    for ( size_t i = 0; i < NUM_CHILDREN; ++i )
      {
        if(!mInfo->hasChild[i])
          continue;

        // The cell coordinates for this child quad
        int x = (i%2)*cWidth + mCellX;
        int y = (i/2)*cWidth + mCellY;

        mChildren[i] = new Quad(x,y,this);
      }
    hasChildren = true;
  }

  // Removes children and rebuilds terrain
  void unsplit()
  {
    RTRACE("unsplit");

    // Never unsplit the root quad
    assert(mLevel < g_archive.rootQuad->level);
    // Never unsplit a static or quad that isn't split.
    assert(!isStatic);
    assert(hasChildren);
    assert(!hasMesh);

    for( size_t i = 0; i < NUM_CHILDREN; i++ )
      {
        delete mChildren[i];
        mChildren[i] = NULL;
      }

    buildTerrain();

    hasChildren = false;
  }

  // Determines whether to split or unsplit the quad, and immediately
  // does it.
  void update()
  {
    RTRACE("Quad::update");

    // Static quads don't change
    if(isStatic)
      return;

    assert(mUnsplitDistance > mSplitDistance);

    // Get (squared) camera distance. TODO: shouldn't this just be a
    // simple vector difference from the mesh center?
    float camDist;
    {
      const Ogre::Vector3 cpos = mCamera->getDerivedPosition();
      Ogre::Vector3 diff(0, 0, 0);
      diff.makeFloor(cpos - mBounds.getMinimum() );
      diff.makeCeil(cpos - mBounds.getMaximum() );
      camDist = diff.squaredLength();
    }

    // No children?
    if(!hasChildren)
      {
        // If we're close, split now.
        if(camDist < mSplitDistance)
          split();
        else
          {
            // We're not close, and don't have any children. Should we
            // built terrain?
            if(!hasMesh)
              buildTerrain();

            return;
          }
      }

    // If we get here, we either had children when we entered, or we
    // just performed a split.
    assert(!hasMesh);
    assert(hasChildren);

    // If the camera is too far away, kill the children.
    if( camDist > mUnsplitDistance )
      {
        unsplit();
        return;
      }

    // We have children and we're happy about it. Update them too.
    for (size_t i = 0; i < NUM_CHILDREN; ++i)
      {
        Quad *q = mChildren[i];
        if(q != NULL) q->update();
      }
  }

  // Build the terrain for this quad
  void buildTerrain()
  {
    RTRACE("buildTerrain");
    assert(!hasMesh);
    assert(!isStatic);

    // Map the terrain data into memory.
    g_archive.mapQuad(mInfo);

    // Create one mesh for each segment in the quad. TerrainMesh takes
    // care of the loading.
    for(int i=0; i < mInfo->meshNum; i++)
      mMeshList.push_back(new TerrainMesh(i, mSceneNode));
  }

  /**
   * @brief destroys the terrain.
   */
  void destroyTerrain()
  {
    RTRACE("destroyTerrain");
    assert(hasMesh);

    for ( MeshList::iterator itr = mMeshList.begin();
          itr != mMeshList.end(); ++itr )
      delete *itr;

    mMeshList.clear();
  }

private:

  // List of meshes, if any
  MeshList mMeshList;

  // Scene node. All child quads are added to this.
  SceneNode* mSceneNode;

  // Bounding box, transformed to world coordinates. Used to calculate
  // camera distance.
  Ogre::AxisAlignedBox mBounds;

  Ogre::Real mSplitDistance,mUnsplitDistance;

  static const size_t NUM_CHILDREN = 4;

  Quad* mChildren[NUM_CHILDREN]; ///optionaly the children. Should be
                                 ///0 if not exist

  // Contains the 'level' of this node. Level 1 is the closest and
  // most detailed level
  int mLevel;
  int mCellX, mCellY;

  QuadInfo *mInfo;

  bool hasMesh;
  bool hasChildren;
  bool isStatic;    // Static quads are never split or unsplit
};
