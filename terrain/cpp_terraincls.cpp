Terrain::Terrain(TerrainHeightmap* d,
                 Ogre::SceneNode* r)
  :  mTerrainData(d),
     mTerrainSceneNode(r),
     mQuadRoot(0),
     mQuadCreateFunction(0),
     mQuadDestroyFunction(0),
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
  mQuadRoot = new Quad(Quad::QL_ROOT, 0, this); //cleaned in Terrain::~Terrain
}
//----------------------------------------------
void Terrain::update(Ogre::Real t){
  assert(mQuadRoot);
  mQuadRoot->update(t);
  mBaseLand.update();
}
//----------------------------------------------
void Terrain::_quadCreated(QuadData* qd){
  if ( mQuadCreateFunction ) (*mQuadCreateFunction)(qd);
}
//----------------------------------------------
void Terrain::_quadDestroyed(QuadData* qd){
  if ( mQuadDestroyFunction ) (*mQuadDestroyFunction)(qd);
}
//----------------------------------------------
int Terrain::getMaxDepth(){
  return mTerrainData->getMaxDepth();
}
//----------------------------------------------
void Terrain::reload(){
  delete mQuadRoot;
  mQuadRoot = new Quad(Quad::QL_ROOT, 0, this);
}
//----------------------------------------------
