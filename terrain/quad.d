module terrain.quad;

import terrain.archive;
import terrain.bindings;

const int CELL_WIDTH = 8192;
const float SPLIT_FACTOR = 0.5;
const float UNSPLIT_FACTOR = 2.0;

class Quad
{
  this(int cellX=0, int cellY=0, Quad parent = null)
    {
      mCellX = cellX;
      mCellY = cellY;

      // Do we have a parent?
      if(parent !is null)
        {
          mLevel = parent.mLevel-1;

          if(mLevel == 1)
            {
              // Create the terrain and leave it there.
              buildTerrain();
              isStatic = true;
            }

          // Coordinates relative to our parent
          int relX = cellX - parent.mCellX;
          int relY = cellY - parent.mCellY;

          // The coordinates give the top left corner of the quad, or our
          // relative coordinates within that should always be positive.
          assert(relX >= 0);
          assert(relY >= 0);

          // Create a child scene node. The scene node position is given in
          // world units, ie. CELL_WIDTH units per cell.
          mNode = terr_createChildNode(relX*CELL_WIDTH,
                                       relY*CELL_WIDTH,
                                       parent.mNode);

          // Get the archive data for this quad.
          mInfo = g_archive.getQuad(mCellX,mCellY,mLevel);
        }
      else
        {
          // No parent, this is the top-most quad. Get all the info from
          // the archive.
          mInfo = g_archive.rootQuad;

          mLevel = mInfo.level;
          cellX = mCellX = mInfo.cellX;
          cellY = mCellY = mInfo.cellY;

          mNode = terr_createChildNode(cellX*CELL_WIDTH,
                                       cellY*CELL_WIDTH,
                                       null);

          // Split up
          split();

          // The root can never be unsplit
          isStatic = true;
        }

      assert(mLevel >= 1);
      assert(mNode !is null);

      // Set up the bounding box. Use MW coordinates all the way
      mBounds = terr_makeBounds(mInfo.minHeight,
                                mInfo.maxHeight,
                                mInfo.worldWidth,
                                mNode);

      float radius = mInfo.boundingRadius;

      mSplitDistance   = radius * SPLIT_FACTOR;
      mUnsplitDistance = radius * UNSPLIT_FACTOR;

      // Square the distances
      mSplitDistance *= mSplitDistance;
      mUnsplitDistance *= mUnsplitDistance;

      // Update the terrain. This will create the mesh or children if
      // necessary.
      update();
    }

  ~this()
    {
      // TODO: We might rewrite the code so that the quads are never
      // actually destroyed, just 'inactivated' by hiding their scene
      // node. We only call update on our children if we don't have a
      // mesh ourselves.
      if(hasMesh)
        destroyTerrain();
      else if(hasChildren)
        for (size_t i = 0; i < 4; i++)
          delete mChildren[i];
    
      terr_destroyNode(mNode);
      terr_killBounds(mBounds);
    }

  // Remove the landscape for this quad, and create children.
  void split()
    {
      // Never split a static quad or a quad that already has children.
      assert(!isStatic);
      assert(!hasChildren);
      assert(mLevel > 1);

      if(hasMesh)
        destroyTerrain();

      // Find the cell width of our children
      int cWidth = 1 << (mLevel-2);

      // Create children
      for ( size_t i = 0; i < 4; ++i )
        {
          if(!mInfo.hasChild[i])
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
      // Never unsplit the root quad
      assert(mLevel < g_archive.rootQuad.level);
      // Never unsplit a static or quad that isn't split.
      assert(!isStatic);
      assert(hasChildren);
      assert(!hasMesh);

      for( size_t i = 0; i < 4; i++ )
        {
          delete mChildren[i];
          mChildren[i] = null;
        }

      buildTerrain();

      hasChildren = false;
    }

  // Determines whether to split or unsplit the quad, and immediately
  // does it.
  void update()
    {
      // Static quads don't change
      if(!isStatic)
        {
          assert(mUnsplitDistance > mSplitDistance);

          // Get (squared) camera distance. TODO: shouldn't this just
          // be a simple vector difference from the mesh center?
          float camDist = terr_getSqCamDist(mBounds);

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

          // If we get here, we either had children when we entered,
          // or we just performed a split.
          assert(!hasMesh);
          assert(hasChildren);

          // If the camera is too far away, kill the children.
          if(camDist > mUnsplitDistance)
            {
              unsplit();
              return;
            }
        }
      else if(!hasChildren)
        return;

      // We have children and we're happy about it. Update them too.
      for(int i; i < 4; ++i)
        {
          Quad q = mChildren[i];
          if(q !is null) q.update();
        }
    }

  // Build the terrain for this quad
  void buildTerrain()
    {
      assert(!hasMesh);
      assert(!isStatic);

      // Map the terrain data into memory.
      g_archive.mapQuad(mInfo);

      // Create one mesh for each segment in the quad. TerrainMesh takes
      // care of the loading.
      meshList.length = mInfo.meshNum;
      foreach(i, ref m; meshList)
        {
          MeshInfo *mi = g_archive.getMeshInfo(i);
          m = terr_makeMesh(mNode, mi, mInfo.level, mInfo.texScale);
        }

      hasMesh = true;
    }

  void destroyTerrain()
    {
      assert(hasMesh);

      foreach(m; meshList)
        terr_killMesh(m);

      meshList[] = null;
      hasMesh = false;
    }

 private:

  // List of meshes, if any. The meshes are C++ objects.
  MeshObj meshList[];

  // Scene node. All child quads are added to this.
  SceneNode mNode;

  // Bounding box, transformed to world coordinates. Used to calculate
  // camera distance.
  Bounds mBounds;

  float mSplitDistance,mUnsplitDistance;

  Quad mChildren[4];

  // Contains the 'level' of this node. Level 1 is the closest and
  // most detailed level
  int mLevel;
  int mCellX, mCellY;

  QuadInfo *mInfo;

  bool hasMesh;
  bool hasChildren;
  bool isStatic;    // Static quads are never split or unsplit
}
