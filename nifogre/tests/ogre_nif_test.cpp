#include "../ogre_nif_loader.h"
#include "../../bsa/bsa_archive.h"

//#define SCREENSHOT

#include "ogre_common.cpp"

//const char* mesh = "meshes\\a\\towershield_steel.nif";
//const char* mesh = "meshes\\r\\bonelord.nif";
//const char* mesh = "meshes\\m\\text_scroll_open_01.nif";
const char* mesh = "meshes\\f\\ex_ashl_a_banner_r.nif";

void C::doTest()
{
  // Add Morrowind.bsa resource location
  addBSA("../../data/Morrowind.bsa");

  // Insert the mesh
  NIFLoader::load(mesh);
  NIFLoader::load(mesh);

  /*
  SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
  Entity *ent = mgr->createEntity("Mesh1", mesh);
  node->attachObject(ent);

  // Works great for the scroll
  node->setPosition(0,4,50);
  node->pitch(Degree(20));
  node->roll(Degree(10));
  node->yaw(Degree(-10));

  /* Bone lord
  node->setPosition(0,-70,170);
  node->pitch(Degree(-90));
  */

  // Display it from two different angles - shield and banner
  const int sep = 45;
  SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
  Entity *ent = mgr->createEntity("Mesh1", mesh);
  node->attachObject(ent);
  node->setPosition(sep,0,130);
  node = node->createChildSceneNode("node2");
  ent = mgr->createEntity("Mesh2", mesh);
  node->attachObject(ent);
  node->setPosition(-2*sep,0,0);
  node->yaw(Degree(180));
  //*/
}
