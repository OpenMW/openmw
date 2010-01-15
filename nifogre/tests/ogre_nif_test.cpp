#include <Ogre.h>

#include "../ogre_nif_loader.h"
#include "../../bsa/bsa_archive.h"

using namespace std;
using namespace Ogre;

RenderWindow *window;

//const char* mesh = "meshes\\a\\towershield_steel.nif";
//const char* mesh = "meshes\\r\\bonelord.nif";
const char* mesh = "meshes\\m\\text_scroll_open_01.nif";

int shot = 0;

// Lets you quit by closing the window
struct QuitListener : FrameListener
{
  bool frameStarted(const FrameEvent& evt)
  {
    if(shot == 1) window->writeContentsToFile("nif.png");
    if(shot < 2) shot++;

    if(window->isClosed())
      return false;
    return true;
  }
} qlistener;

int main(int argc, char**args)
{
  // When the test is done, consider disabling the rendering part
  // unless a command line parameter is given (and write a note about
  // this to console.) This allows you to run the test from scripts
  // and still do some meaningful testing, even if you can't inpsect
  // the result visually.

  // Disable Ogre logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Root.
  Root *root = new Root("plugins.cfg","ogre.cfg","");

  if(!root->restoreConfig())
    {
      cout << "WARNING: we do NOT recommend fullscreen mode!\n";
      if(!root->showConfigDialog())
        return 1;
    }

  SceneManager *mgr = root->createSceneManager(ST_GENERIC);

  // Only render if there are arguments on the command line (we don't
  // care what they are.)
  bool render = (argc>=2);

  // Create a window
  window = root->initialise(true, "Test");
  if(render)
    {
      // More initialization
      Camera *cam = mgr->createCamera("cam");
      Viewport *vp = window->addViewport(cam);
      cam->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
      cam->setFOVy(Degree(55));
      cam->setPosition(0,0,0);
      cam->lookAt(0,0,10);
      cam->setNearClipDistance(1);

      root->addFrameListener(&qlistener);

      // Background color
      vp->setBackgroundColour(ColourValue(0.5,0.5,0.5));

      mgr->setAmbientLight(ColourValue(1,1,1));
    }

  // Add Morrowind.bsa resource location
  addBSA("../../data/Morrowind.bsa");

  // Insert the mesh
  NIFLoader::load(mesh);
  NIFLoader::load(mesh);

  //*
  SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
  Entity *ent = mgr->createEntity("Mesh1", mesh);
  node->attachObject(ent);
  node->setPosition(0,4,50);
  node->pitch(Degree(20));
  node->roll(Degree(10));
  node->yaw(Degree(-10));
  /*
  node->setPosition(0,-70,170);
  node->pitch(Degree(-90));
  /*
  // Display it from two different angles
  const int sep = 45;
  SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
  Entity *ent = mgr->createEntity("Mesh1", mesh);
  node->attachObject(ent);
  node->setPosition(sep,0,100);
  node = node->createChildSceneNode("node2");
  ent = mgr->createEntity("Mesh2", mesh);
  node->attachObject(ent);
  node->setPosition(-2*sep,0,0);
  node->yaw(Degree(180));
  //*/

  // Render loop
  if(render)
    {
      cout << "Rendering. Close the window to exit.\n";
      root->startRendering();
    }

  // Cleanup
  delete root;
  return 0;
}
