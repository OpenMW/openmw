HeightMap::HeightMap(Ogre::SceneNode* r)
  :  mTerrainSceneNode(r),
     mQuadRoot(0),
     mMorphingEnabled(true),
     mTextureFadingEnabled(true),
     mBaseLand(r)
{

}
//----------------------------------------------
HeightMap::~HeightMap(){
  delete mQuadRoot;
}
//----------------------------------------------
void HeightMap::create(){
  mQuadRoot = new Quad(Quad::QL_ROOT, 0);
}
//----------------------------------------------
void HeightMap::update(Ogre::Real t){
  assert(mQuadRoot);
  mQuadRoot->update(t);
  mBaseLand.update();
}

