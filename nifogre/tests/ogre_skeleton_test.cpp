#include "ogre_mesh_common.cpp"

void C::doTest()
{
  MeshPtr msh = makeMesh("mesh1");

  // Display the mesh
  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
    Entity *ent = mgr->createEntity("Mesh1", "mesh1");
    node->attachObject(ent);
    node->setPosition(0,0,4);
  }
}
