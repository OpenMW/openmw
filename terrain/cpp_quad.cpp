/**
 * @brief defines an area of Landscape
 *
 * A quad can either hold a mesh, or 4 other sub quads
 * The functions split and unslip either break the current quad into smaller quads, or
 * alternatively remove the lower quads and create the terrain mesh on the the current (now the lowest) level
 *
 * needUnsplit and needSplit query the state of the meshes to see if it needs spliting or unspliting
 *
 */
class Quad
{
public:

  /**
   * when each quad is split, the children can be one of 4 places,
   * topleft (NW) top right (NE) etc etc. The other position is the
   * root which is the top quad.  The root quad doesn't have a mesh,
   * and should always have 4 children.
   */

  // FIXME: There's probably a better way to do this
  enum QuadLocation { QL_NW, QL_NE, QL_SW, QL_SE, QL_ROOT };

  /**
   * @param l the location of the quad
   * @param p the parent quad. Leave 0 if it is root
   * @param t the terrain object
   *
   * Constructor mainly sets up the position variables/depth etc
   */
  Quad(QuadLocation l, Quad* p, Terrain* t)
    : mParent(p), mTerrain(t), mTerrainMesh(0), mLocation(l)
  {
    //as mentioned elsewhere, the children should all be null.
    memset(mChildren, NULL, sizeof(Quad*)*NUM_CHILDREN);

    //find the location of the quad
    if ( l != Quad::QL_ROOT )
      { //if it isn't the root node
        mDepth = p->getDepth() + 1;

        mPosition = p->getPosition();
        mSideLength = p->getSideLength()/2;

        //horrible bit of code
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
            break;//get rid of warning
        }

        //set after positions have been retrived
        TerrainHeightmap* d = mTerrain->getTerrainData();
        mMaxDepth = d->getMaxDepth();
        mHasData = d->hasData(this);

        if ( needSplit() ) //need to "semi" build terrain
            split();
        else if ( mHasData )
          {
            buildTerrain();
            getMesh()->justSplit();
          }

      }
    else
      { //assume it is root node, get data and possition
        mDepth = 0; //root
        mSideLength = mTerrain->getTerrainData()->getRootSideLength();
        mPosition = Point2<long>(0,0); //see Quad::getPosition as to why this is always 0

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
    destroyTerrain();
    for (size_t i = 0; i < NUM_CHILDREN; i++)
      delete mChildren[i];
  }

  void update(Ogre::Real t)
  {
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
    if ( mTerrainMesh )
      mTerrainMesh->update(t);
    else if ( hasChildren() )
      {
        for (size_t i = 0; i < NUM_CHILDREN; ++i) {
          assert( mChildren[i] );
          mChildren[i]->update(t);
        }
      }
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
    if ( hasChildren() ||
         getDepth() == mMaxDepth ||
         !hasData() )
      return false;
    return ( mTerrainMesh && mTerrainMesh->getSplitState()
             == MeshInterface::SS_SPLIT );
  }

  /**
   * The functions preforms work on the quad tree state. There is no
   * requirement for this to be called every frame, but it is not
   * thread safe due to interacting with OGRE The function checks if a
   * split or unsplit is needed. It also calls update on all children

   * @param t the time since the last frame
   */

  /**
   * Deletes the landscape, if there is any
   *   Creates children, and either splits them, or creates landscape for them
   */
  void split()
  {
    destroyTerrain();

    //create a new terrain
    for ( size_t i = 0; i < NUM_CHILDREN; ++i )
        mChildren[i] = new Quad((QuadLocation)i, this, mTerrain);

    assert(!needUnsplit());
  }

  /**
   * @brief removes all children, and builds terrain on this level
   */
  void unsplit()
  {
    //shouldn't unsplit 0 depth
    assert(getDepth());

    for ( size_t i = 0; i < NUM_CHILDREN; i++ )//{
      delete mChildren[i];
    memset(mChildren, NULL, sizeof(Quad*)*NUM_CHILDREN);

    if ( mHasData ) {
      buildTerrain();
      getMesh()->justUnsplit();
    }
    assert(!needSplit());
  }

  /**
   * @return true if the node needs to delete all its child nodes, and
   * rebuild the terrain its level
   */
  bool needUnsplit()
  {
    if ( hasChildren() && getDepth() ) {
      for (size_t i=0;i< NUM_CHILDREN;i++) {
        if ( mChildren[i]->hasData()  ) {
          if ( !mChildren[i]->hasMesh() )
            return false;
          else if ( mChildren[i]->getMesh()->getSplitState() != MeshInterface::SS_UNSPLIT)
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
    assert(hasData());
    assert(getMesh() == NULL); //the terrain sould not exist
    mTerrainMesh = new MeshInterface(this, mTerrain);
    mTerrainMesh->create();
  }

  /**
   * @brief destroys the terrain.
   */
  void destroyTerrain()
  {
    delete mTerrainMesh;
    mTerrainMesh = NULL;
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
  inline bool hasMesh() const{ return mTerrainMesh != 0; }

  /**
   * @return true if there are any children
   */
  inline bool hasChildren() const { return mChildren[0] != 0; }

  /**
   * @return the mesh. 0 if there is no mesh
   */
  MeshInterface* getMesh()
  {
    if ( mTerrainMesh == 0 ) return 0;
    return mTerrainMesh;
  }

  /**
   * @brief checks if the quad has any data (i.e. a mesh avaible for rendering
   */
  inline bool hasData() const{ return mHasData; }

private:
  static const size_t NUM_CHILDREN = 4;

  Quad* mParent; /// this is the node above this. 0 if this is root
  Quad* mChildren[4]; ///optionaly the children. Should be 0 if not exist

  Terrain* mTerrain; ///the pointer to the root terrain
  MeshInterface* mTerrainMesh; ///the terrain mesh, only used if this is the bottom node
  Quad::QuadLocation mLocation; ///the location within the quad (ne, se, nw, sw). See Quad::QuadLocation
  Point2<long> mPosition; ///the center of the mesh. this is a long so can be used as comparison. See Quad::getPosition
  long mSideLength; ///the length in units of one side of the quad. See Quad::getSideLength
  int mDepth; ///depth of the node. See Quad::getDepth for more info

  bool mHasData; ///holds if there is terrain data about this quad
  int mMaxDepth; ///the maxmium depth. Cached. This is not valid is mDepth == 0
};

//const size_t Quad::NUM_CHILDREN = 4;
