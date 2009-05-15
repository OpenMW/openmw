MeshInterface::MeshInterface(Quad* p, Terrain* t) :
  mParentQuad(p),
  mTerrain(t),
  mMax(0),
  mMin(0),
  mSplitState(SS_NONE) {

  mQuadData = t->getTerrainData()->getData(mParentQuad);

  //the mesh is created a zero, so an offset is applied
  const Ogre::Vector3 pos(mParentQuad->getPosition().x - mParentQuad->getSideLength()/2,
                          0,
                          mParentQuad->getPosition().y - mParentQuad->getSideLength()/2);

  mSceneNode = mTerrain->getTerrainSceneNode()->createChildSceneNode(pos);
}

MeshInterface::~MeshInterface() {
  for ( std::vector<TerrainObjectGroup*>::iterator itr = mTerrainObjects.begin();
        itr != mTerrainObjects.end();
        ++itr )
    delete *itr;

  mSceneNode->removeAndDestroyAllChildren();
  mSceneMgr->destroySceneNode(mSceneNode);

  mTerrain->getTerrainSceneNode()->detachAllObjects();

  mTerrain->_quadDestroyed(mQuadData);
  delete mQuadData;

}

void MeshInterface::create() {
  //LOG("Creating");
  if ( mParentQuad->getDepth() == mTerrain->getMaxDepth() ) {
    for ( int y = 0; y < 4; ++y ) {
      for ( int x = 0; x < 4; ++x ) {
        addNewObject(
                     Ogre::Vector3(x*16*128, 0, y*16*128), //pos
                     17, //size
                     false, //skirts
                     0.25f, float(x)/4.0f, float(y)/4.0f); //quad seg location
      }
    }
  } else {
    addNewObject(Ogre::Vector3(0,0,0), 65);
  }

  getBounds();
  mTerrain->_quadCreated(mQuadData);
}

void MeshInterface::addNewObject(const Ogre::Vector3& pos,
                                 int terrainSize,
                                 bool skirts /*= true*/,
                                 float segmentSize /*= 1*/,
                                 float startX /*= 0*/,
                                 float startY /*= 0*/ ) {

  TerrainObjectGroup* to = new TerrainObjectGroup();

  to->segment = new QuadSegment(mQuadData, segmentSize, startX, startY);
  to->node = mSceneNode->createChildSceneNode(pos);
  to->terrain = new TerrainRenderable(mTerrain, to->segment, terrainSize, mParentQuad->getDepth(), skirts);
  to->terrain->create(to->node);

  mMax = std::max(to->terrain->getMax(), mMax);
  mMin = std::max(to->terrain->getMin(), mMin);

  mTerrainObjects.push_back(to);
}

void MeshInterface::update(Ogre::Real time) {
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


  for ( std::vector<TerrainObjectGroup*>::iterator itr = mTerrainObjects.begin();
        itr != mTerrainObjects.end();
        ++itr )
    (*itr)->terrain->update(time, camDist, mUnsplitDistance, mMorphDistance);
}

void MeshInterface::getBounds() {
  mBounds.setExtents( 0,
                      mMin,
                      0,
                      (65 - 1) * mQuadData->getVertexSeperation(),
                      mMax,
                      (65 - 1) * mQuadData->getVertexSeperation());

  mBoundingRadius = (mBounds.getMaximum() - mBounds.getMinimum()).length() / 2;

  mSplitDistance = pow(mBoundingRadius * 0.5, 2);
  mUnsplitDistance = pow(mBoundingRadius * 2.0, 2);
  mMorphDistance = pow(mBoundingRadius * 1.5, 2);
}
