Terrain::Terrain(Ogre::SceneNode* r)
  :  mTerrainSceneNode(r),
     mQuadRoot(0),
     mMorphingEnabled(true),
     mTextureFadingEnabled(true),
     mBaseLand(r)
{

}
//----------------------------------------------
Terrain::~Terrain(){
  delete mQuadRoot;
}
//----------------------------------------------
void Terrain::create(){
  mQuadRoot = new Quad(Quad::QL_ROOT, 0);
}
//----------------------------------------------
void Terrain::update(Ogre::Real t){
  assert(mQuadRoot);
  mQuadRoot->update(t);
  mBaseLand.update();
}
//----------------------------------------------
int Terrain::getMaxDepth(){
  return g_heightMap->getMaxDepth();
}
/*
//----------------------------------------------
void Terrain::reload(){
  delete mQuadRoot;
  mQuadRoot = new Quad(Quad::QL_ROOT, 0);
}
//----------------------------------------------
*/
