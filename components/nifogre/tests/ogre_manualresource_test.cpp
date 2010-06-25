/*
  This is a test of the manual resource loader interface to Ogre,
  applied to manually created meshes. It defines a simple mesh
  consisting of two triangles, and creates three instances of it as
  different meshes using the same loader. It is a precursor to the NIF
  loading code. If the Ogre interface changes and you have to change
  this test, then you will also have to change parts of the NIF
  loader.
 */

#include "ogre_mesh_common.cpp"

void C::doTest()
{
  // Create a couple of manual meshes
  makeMesh("mesh1.mm");
  makeMesh("mesh2.mm");
  makeMesh("mesh3.mm");

  // Display the meshes
  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
    Entity *ent = mgr->createEntity("Mesh1", "mesh1.mm");
    node->attachObject(ent);
    node->setPosition(3,1,8);
  }

  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node2");
    Entity *ent = mgr->createEntity("Mesh2", "mesh2.mm");
    node->attachObject(ent);
    node->setPosition(-3,1,8);
  }
  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node3");
    Entity *ent = mgr->createEntity("Mesh3", "mesh3.mm");
    node->attachObject(ent);
    node->setPosition(0,-2,8);
  }
}
