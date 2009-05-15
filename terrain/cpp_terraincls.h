class Terrain
{
public:
  /**
   * @brief inits vars and creates quad tree
   * @param d the heightmap that contains the data
   * @param s the current scne manager
   * @param r the root scene node that all terrain nodes are barsed off
   * @param c the camera that the poistion data is taken from
   *
   * The terrain is create in the constructor.
   * @todo change quad root creation to create/destroy funcs?
   */
  explicit Terrain(TerrainHeightmap* d, Ogre::SceneNode* r);
  /**
   * @brief deletes the quad tree
   */
  ~Terrain();

  /**
   * @brief creates the quad tree
   * @remarks This must be called before any call to upate(). We cannot call it from the constructor as options
   *   may not have been set yet
   */
  void create();

  /**
   * @brief recreates the quad tree by destroying everything and starting again.
   * @remarks this is very slow, as it drops all created alpha maps and meshes.
   * @todo check this works
   */
  void reload();

  /**
   * @brief sets the scene node that all of the terrain nodes are based off
   * @param r the scene node to use for terrain
   */
  inline void setTerrainSceneNode(Ogre::SceneNode* r){ mTerrainSceneNode = r; }

  /**
   * @return the scene node that should be used as the root of the terrain
   *
   * This is mainly used by the quad tree
   */
  inline Ogre::SceneNode* getTerrainSceneNode(){return mTerrainSceneNode;}

  /**
   * @brief updates the quad tree, splittig and unsplitting where needed.
   *
   * There is no requirement for this to be called every frame
   */
  void update(Ogre::Real t);

  /**
   * @return the heightmap data
   */
  inline TerrainHeightmap* getTerrainData(){ return mTerrainData; }

  /**
   * @brief handles the actions to take on the creation of a terrain mesh
   * @param qd the quad data. It is valid until the same variable is passed to _quadDestroyed
   */
  void _quadCreated(QuadData* qd);

  /**
   * @brief The quad as defined by the quad data qd has been destroyed
   * @param qd the quad that has been destroyed. This is only valid for this function. Is is deleted just after
   */
  void _quadDestroyed(QuadData* qd);

  /**
   * @brief sets the function to be used in the callback when a quad is created
   * @param f the function to use. Set to null to disable callbacks
   */
  inline void setQuadCreateFunction(void (*f)(QuadData*)){
    mQuadCreateFunction = f;
  }
  /**
   * @brief sets the function to be used in the callback when a quad is destroyed
   * @param f the function to use. Set to null to disable callbacks
   */
  inline void setQuadDestroyFunction(void (*f)(QuadData*)){
    mQuadDestroyFunction = f;
  }

  /**
   * @brief time in seconds to morph to full detail after an unsplit.
   */
  inline Ogre::Real getMorphSpeed(){return 1.0f;}
  /**
   * @brief the time in seconds it takes for the texture to fade to the higher detail
   */
  inline Ogre::Real getTextureFadeSpeed(){ return 2.0f;}

  /**
   * @return the maximum depth of the quad tree
   * @remarks this is used by the terrain renderable for adding morhping to textures. It doesn't want to happen
   *   on the lowest detail
   */
  int getMaxDepth();

  /**
   * @brief disables/enables morphing
   * @remarks This will not happen instantly unless reload() is called
   */

  inline void setMorphingEnabled(bool enabled){
    mMorphingEnabled = enabled;

  }
  /**
   * @return true if morphing is enabled
   */
  inline bool isMorhpingEnabled() const{
    return mMorphingEnabled;
  }

  /**
   * @brief disables/enables texture fading.
   * @remarks This will not happen instantly unless reload() is called. Morphing is required atm
   */
  inline void setTextureFadingEnabled(bool enabled){
    if ( enabled && !mMorphingEnabled )
      OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Cannot have fading but not morphing active", "Terrain::setTextureFadingEnabled");
    mTextureFadingEnabled = enabled;
  }
  /**
   * @return true if texture fading is enabled
   */
  inline bool isTextureFadingEnabled() const{
    return mTextureFadingEnabled;
  }
protected:
  TerrainHeightmap* mTerrainData;

  /// the scenenode that every other node is decended from. This
  /// should be surplied by the user
  Ogre::SceneNode* mTerrainSceneNode;

  ///the root node for all the quads.
  Quad* mQuadRoot;

  ///quad callback function
  void (*mQuadCreateFunction)(QuadData*);
  ///quad callback function
  void (*mQuadDestroyFunction)(QuadData*);

  bool mMorphingEnabled;
  bool mTextureFadingEnabled;

  BaseLand mBaseLand;
};
