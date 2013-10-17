#include "ogre_common.cpp"

void C::doTest()
{
  SkeletonManager &skm = SkeletonManager::getSingleton();

  SkeletonPtr skp = skm.create("MySkel", "General");

  cout << "hello\n";
  /*
  MeshPtr msh = makeMesh("mesh1");

  // Display the mesh
  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
    Entity *ent = mgr->createEntity("Mesh1", "mesh1");
    node->attachObject(ent);
    node->setPosition(0,0,4);
  }
  */
}
