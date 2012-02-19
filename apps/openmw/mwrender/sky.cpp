#include "sky.hpp"
#include "Caelum.h"

#include <OgreMesh.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreCamera.h>

#include <components/nifogre/ogre_nif_loader.hpp>

using namespace Ogre;

namespace MWRender
{
    class MWSkyManager : public SkyManager
    {
    public:
        MWSkyManager(Ogre::SceneNode* pMwRoot, Ogre::Camera* pCamera);
        virtual ~MWSkyManager();
        
        virtual void update(float duration);
        
        virtual void enable();
        
        virtual void disable();
        
        virtual void setHour (double hour) {}
        ///< will be called even when sky is disabled.
        
        virtual void setDate (int day, int month) {}
        ///< will be called even when sky is disabled.
        
        virtual int getMasserPhase() const { return 0; }
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon
        
        virtual int getSecundaPhase() const { return 0; }
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon
        
        virtual void setMoonColour (bool red) {}
        
    private:
        Camera* mCamera;
        SceneNode* mRootNode;
        SceneManager* mSceneMgr;
        
        MaterialPtr mCloudMaterial;
        MaterialPtr mAtmosphereMaterial;
    };
    
    MWSkyManager::MWSkyManager (SceneNode* pMwRoot, Camera* pCamera)
    {
        mSceneMgr = pMwRoot->getCreator();
        mRootNode = pMwRoot->createChildSceneNode();

        // Atmosphere
        NifOgre::NIFLoader::load("meshes\\sky_atmosphere.nif");
        Entity* atmosphere_ent = mSceneMgr->createEntity("meshes\\sky_atmosphere.nif");
        atmosphere_ent->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY);
        Ogre::SceneNode* atmosphere_node = mRootNode->createChildSceneNode();
        atmosphere_node->attachObject(atmosphere_ent);
        mAtmosphereMaterial = atmosphere_ent->getSubEntity(0)->getMaterial();

        // Clouds
        NifOgre::NIFLoader::load("meshes\\sky_clouds_01.nif");
        Entity* clouds_ent = mSceneMgr->createEntity("meshes\\sky_clouds_01.nif");
        clouds_ent->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY);
        SceneNode* clouds_node = mRootNode->createChildSceneNode();
        clouds_node->attachObject(clouds_ent);
        mCloudMaterial = clouds_ent->getSubEntity(0)->getMaterial();
        
        // I'm not sure if the materials are being used by any other objects
        // Make a unique "modifiable" copy of the materials to be sure
        mCloudMaterial = mCloudMaterial->clone("Clouds");
        clouds_ent->getSubEntity(0)->setMaterial(mCloudMaterial);
        mAtmosphereMaterial = mAtmosphereMaterial->clone("Atmosphere");
        atmosphere_ent->getSubEntity(0)->setMaterial(mAtmosphereMaterial);
        
        // Default atmosphere color: light blue
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setAmbient(0.235, 0.5, 0.73);
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setDiffuse(0.0, 0.0, 0.0, 1.0);
        // Set up an UV scroll animation to move the clouds
        mCloudMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setScrollAnimation(0.01f, 0.01f);
        // Disable depth writing so that the sky does not cover any objects
        mCloudMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        // Alpha-blend the clouds with the atmosphere
        mCloudMaterial->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
        
        mCamera = pCamera;
        mCamera->setFarClipDistance(50000.f);
    }
    MWSkyManager::~MWSkyManager()
    {
    }
    
    void MWSkyManager::update(float duration)
    {
        // Sync the position of the skydomes with the camera
        /// \todo for some reason this is 1 frame delayed, which causes the skydome move funnily when the camera moves
        mRootNode->_setDerivedPosition(mCamera->getParentSceneNode()->_getDerivedPosition());
    }
    
    void MWSkyManager::enable()
    {
        mRootNode->setVisible(true);
    }
    
    void MWSkyManager::disable()
    {
        mRootNode->setVisible(false);
    }
    
    
    //
    // Implements a Caelum sky with default settings.
    //
    // Note: this is intended as a temporary solution to provide some form of 
    // sky rendering.  This code will obviously need significant tailoring to
    // support fidelity with Morrowind's rendering.  Before doing major work
    // on this class, more research should be done to determine whether
    // Caelum or another plug-in such as SkyX would be best for the long-term.
    //
    class CaelumManager : public SkyManager
    {
    protected:
        Caelum::CaelumSystem*   mpCaelumSystem;

    public:
                 CaelumManager (Ogre::RenderWindow* pRenderWindow, 
                                   Ogre::Camera* pCamera,
                                   const boost::filesystem::path& resDir);
        virtual ~CaelumManager ();
        
        virtual void update(float duration) {}
        
        virtual void enable() {}
        
        virtual void disable() {}
        
        virtual void setHour (double hour) {}
        ///< will be called even when sky is disabled.
        
        virtual void setDate (int day, int month) {}
        ///< will be called even when sky is disabled.
        
        virtual int getMasserPhase() const { return 0; }
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon
        
        virtual int getSecundaPhase() const { return 0; }
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon
        
        virtual void setMoonColour (bool red) {}
    };

    CaelumManager::CaelumManager (Ogre::RenderWindow* pRenderWindow, 
                                  Ogre::Camera* pCamera,
                                  const boost::filesystem::path& resDir)
        : mpCaelumSystem        (NULL)
    {
        using namespace Caelum;

        assert(pCamera);
        assert(pRenderWindow);

        // Load the Caelum resources
        //
        ResourceGroupManager::getSingleton().addResourceLocation((resDir / "caelum").string(), "FileSystem", "Caelum");
        ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

        // Load the Caelum resources
        //
        Ogre::SceneManager* pScene = pCamera->getSceneManager();
        Caelum::CaelumSystem::CaelumComponent componentMask = CaelumSystem::CAELUM_COMPONENTS_DEFAULT;
        mpCaelumSystem = new Caelum::CaelumSystem (Root::getSingletonPtr(), pScene, componentMask);
        
        // Set time acceleration.
        mpCaelumSystem->getUniversalClock()->setTimeScale(128);       

        // Disable fog since OpenMW is handling OGRE fog elsewhere
        mpCaelumSystem->setManageSceneFog(false);

        // Change the camera far distance to make sure the sky is not clipped
        pCamera->setFarClipDistance(50000);

        // Register Caelum as an OGRE listener
        pRenderWindow->addListener(mpCaelumSystem);
        Root::getSingletonPtr()->addFrameListener(mpCaelumSystem);
    }

    CaelumManager::~CaelumManager() 
    {
        if (mpCaelumSystem) 
            mpCaelumSystem->shutdown (false);
    }

    /// Creates and connects the sky rendering component to OGRE.
    ///
    /// \return NULL on failure.
    /// 
    SkyManager* SkyManager::create (Ogre::RenderWindow* pRenderWindow, 
                                    Ogre::Camera*       pCamera,
                                    Ogre::SceneNode* pMwRoot,
                                    const boost::filesystem::path& resDir)
    {
        SkyManager* pSkyManager = NULL;

        try
        {
            //pSkyManager = new CaelumManager(pRenderWindow, pCamera, resDir);
            pSkyManager = new MWSkyManager(pMwRoot, pCamera);
        }
        catch (Ogre::Exception& e)
        {
            std::cout << "\nOGRE Exception when attempting to add sky: " 
                << e.getFullDescription().c_str() << std::endl;
        }
        catch (std::exception& e)
        {
            std::cout << "\nException when attempting to add sky: " 
                << e.what() << std::endl;
        }

        return pSkyManager;
    }
} 
