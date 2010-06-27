#include "mwscene.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

#include "Caelum.h"

using namespace MWRender;
using namespace Ogre;
using namespace Caelum;

class MWWeatherFrameListener : public Ogre::FrameListener
{
protected:
    Caelum::CaelumSystem*   mpCaelumSystem;
    Ogre::SceneManager*     mpScene;
    float                   mfSpeedFactor;
    bool                    mbPaused;
    float                   mfTimeTillNextUpdate;

public:
    MWWeatherFrameListener(RenderWindow* pRenderWindow, Camera* pCamera) 
        : mpCaelumSystem        (NULL)
        , mpScene               (NULL)
        , mfSpeedFactor         (1.0f)
        , mbPaused              (false)
        , mfTimeTillNextUpdate  (0.0f)
    {
        ConfigFile cf;
        cf.load("resources.cfg");
        ResourceGroupManager::getSingleton().addResourceLocation(".", "FileSystem");

        ConfigFile::SectionIterator seci = cf.getSectionIterator();

        String secName, typeName, archName;
        while (seci.hasMoreElements())
        {
            secName = seci.peekNextKey();
            ConfigFile::SettingsMultiMap *settings = seci.getNext();
            ConfigFile::SettingsMultiMap::iterator i;
            for (i = settings->begin(); i != settings->end(); ++i)
            {
                typeName = i->first;
                archName = i->second;
                ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
            }
        }

        mpScene = pCamera->getSceneManager();

        ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

        Caelum::CaelumSystem::CaelumComponent componentMask = CaelumSystem::CAELUM_COMPONENTS_DEFAULT;
        componentMask = static_cast<Caelum::CaelumSystem::CaelumComponent> (
                //Caelum::CaelumSystem::CAELUM_COMPONENT_SUN |				
                //Caelum::CaelumSystem::CAELUM_COMPONENT_MOON |
                //Caelum::CaelumSystem::CAELUM_COMPONENT_SKY_DOME |
                //Caelum::CaelumSystem::CAELUM_COMPONENT_IMAGE_STARFIELD |
                //Caelum::CaelumSystem::CAELUM_COMPONENT_POINT_STARFIELD |
                Caelum::CaelumSystem::CAELUM_COMPONENT_CLOUDS |
                0);
        componentMask = CaelumSystem::CAELUM_COMPONENTS_DEFAULT;

        mpCaelumSystem = new Caelum::CaelumSystem (Root::getSingletonPtr(), mpScene, componentMask);
        mpCaelumSystem->setManageSceneFog(false);
//        mpCaelumSystem->getCloudSystem()->

        // Set time acceleration.
        mpCaelumSystem->getUniversalClock ()->setTimeScale(512);
        mfSpeedFactor = mpCaelumSystem->getUniversalClock ()->getTimeScale();

        // Register caelum as a listener.
        pRenderWindow->addListener(mpCaelumSystem);
        Root::getSingletonPtr()->addFrameListener(mpCaelumSystem);
    }

    ~MWWeatherFrameListener() 
    {
        if (mpCaelumSystem) 
        {
            mpCaelumSystem->shutdown (false);
            mpCaelumSystem = NULL;
        }
    }

};

MWScene::MWScene(Render::OgreRenderer &_rend)
  : rend(_rend)
{
  Root *root = rend.getRoot();
  RenderWindow *window = rend.getWindow();

  // Get the SceneManager, in this case a generic one
  sceneMgr = root->createSceneManager(ST_GENERIC);

  // Create the camera
  camera = sceneMgr->createCamera("PlayerCam");

  camera->setNearClipDistance(5);
  
  // Create one viewport, entire window
  vp = window->addViewport(camera);

  // Alter the camera aspect ratio to match the viewport
  camera->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
  camera->setFOVy(Degree(55));

  // Set default mipmap level (NB some APIs ignore this)
  TextureManager::getSingleton().setDefaultNumMipmaps(5);

  // Load resources
  ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

  // Turn the entire scene (represented by the 'root' node) -90
  // degrees around the x axis. This makes Z go upwards, and Y go into
  // the screen (when x is to the right.) This is the orientation that
  // Morrowind uses, and it automagically makes everything work as it
  // should.
  SceneNode *rt = sceneMgr->getRootSceneNode();
  mwRoot = rt->createChildSceneNode();
  mwRoot->pitch(Degree(-90));

  // For testing
  sceneMgr->setAmbientLight(ColourValue(1,1,1));

  try
  {
    MWWeatherFrameListener* pWeather = new MWWeatherFrameListener (window, camera);
  }
  catch (Exception& e)
  {
      std::cout << "\nERROR: " << e.getFullDescription().c_str() << std::endl;
  }
  catch(std::exception &e)
  {
      std::cout << "\nERROR: " << e.what() << std::endl;
  }
}
