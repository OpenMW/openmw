#include <Ogre.h>
#include <iostream>
#include <assert.h>

using namespace std;
using namespace Ogre;

struct MyMeshLoader : ManualResourceLoader
{
  void loadResource(Resource *resource)
  {
    Mesh *mesh = dynamic_cast<Mesh*>(resource);
    assert(mesh);

    const String& name = mesh->getName();
    cout << "Manually loading mesh " << name << endl;

    // Create the mesh here
  }
};

MyMeshLoader mml;

int main()
{
  // When the test is done, consider disabling the rendering part
  // unless a command line parameter is given (and write a note about
  // this to console.) This allows you to run the test from scripts
  // and still do some meaningful testing, even if you can't inpsect
  // the result visually.

  /*
  // Disable Ogre logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);
  */

  // Set up Root.
  Root *root = new Root("plugin.cfg","ogre.cfg","");

  if(!root->restoreConfig())
    if(!root->showConfigDialog())
      return 1;

  // Create a window
  RenderWindow *window = root->initialise(true, "Test");

  // We might need input managment too

  // More initialization
  SceneManager *mgr = mRoot->createSceneManager(ST_GENERIC);
  Camera *cam = mgr->createCamera("cam");
  ViewPort *vp = window->addViewport(cam);
  cam->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
  cam->setFOVy(Degree(55));

  // Background color
  vp->setBackgroundColour(ColourValue(0,0,0));

  // Declare a couple of manual meshes
  ResourceGroupManager::getSingleton().declareResource("mesh1.mm", "Mesh", "General", &mml);
  ResourceGroupManager::getSingleton().declareResource("mesh2.mm", "Mesh", "General", &mml);

  // Display the meshes here

  return 0;
}
