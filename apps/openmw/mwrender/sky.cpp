#include "sky.hpp"

#include "sky_impl.hpp"

#include <OgreCamera.h>
#include <OgreRenderWindow.h>
#include <OgreSceneNode.h>

namespace MWRender
{
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
} // namespace
