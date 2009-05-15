/**
* Interface between the quad and the terrain renderble classes, to the
* quad it looks like this rendereds a single mesh for the quad. This
* may not be the case.
*
* It also could allow several optomizations (e.g. multiple splits)
*/
class MeshInterface
{
    /**
    *
    * @brief Holds a group of objects and destorys them in the
    * destructor. Avoids the needs for 100s of vectors
    */
    struct TerrainObjectGroup {
        /**
        * @brief inits all ptrs at 0
        */
        inline TerrainObjectGroup() : segment(0), terrain(0), node(0) {}

        /**
        * @brief destorys all objects
        */
        inline ~TerrainObjectGroup() {
            if ( node ) {
                node->detachAllObjects();
                node->getCreator()->destroySceneNode(node);
            }
            delete terrain;
            delete segment;
        }
        QuadSegment* segment;
        TerrainRenderable* terrain;
        Ogre::SceneNode* node;
    };
public:
    enum SplitState { SS_NONE, SS_SPLIT, SS_UNSPLIT };

    MeshInterface(Quad* p, Terrain* t);
    ~MeshInterface() ;

    /**
    * @brief creates all required meshes. If it is at the max depth, it creates 16, otherwise just one
    */
    void create();

    /**
    * @brief updates all meshes.
    * @remarks the camera distance is calculated here so that all terrain has the correct morph levels etc
    */
    void update(Ogre::Real time);

    inline SplitState getSplitState() {
        return mSplitState;
    }

    /**
    * @brief propergates the just split through all terrain
    */
    inline void justSplit() {
        for ( std::vector<TerrainObjectGroup*>::iterator itr = mTerrainObjects.begin();
                itr != mTerrainObjects.end();
                ++itr )
            (*itr)->terrain->justSplit();
    }
    /**
    * @brief propergates the just unsplit through all terrain
    */
    inline void justUnsplit() {
        for ( std::vector<TerrainObjectGroup*>::iterator itr = mTerrainObjects.begin();
                itr != mTerrainObjects.end();
                ++itr )
            (*itr)->terrain->justUnsplit();
    }

private:

    Quad* mParentQuad;
    Terrain* mTerrain;

    ///Must be a ptr, else it destorys before we are ready
    std::vector<TerrainObjectGroup*> mTerrainObjects;

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
    void getBounds();

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
                      float startX = 0, float startY = 0 );



};
