#include <Ogre.h>
#include <iostream>

using namespace std;
using namespace Ogre;

Root *root;
RenderWindow *window;
SceneManager *mgr;

int shot = 0;

// Lets you quit by closing the window
struct QuitListener : FrameListener
{
  bool frameStarted(const FrameEvent& evt)
  {
#ifdef SCREENSHOT
    if(shot == 1) window->writeContentsToFile("nif.png");
    if(shot < 2) shot++;
#endif

    if(window->isClosed())
      return false;
    return true;
  }
} qlistener;

// This has to be packaged in a struct because C++ sucks
struct C
{
  static void doTest();
};

int main(int argc, char**args)
{
  // Disable Ogre logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Root.
  root = new Root("plugins.cfg","ogre.cfg","");

  if(!root->restoreConfig())
    {
      cout << "WARNING: we do NOT recommend fullscreen mode!\n";
      if(!root->showConfigDialog())
        return 1;
    }

  mgr = root->createSceneManager(ST_GENERIC);

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

  // Run the actual test
  C::doTest();

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

void doTest()
{
  cout << "hello\n";
}
