#include "path.hpp"

#include <boost/filesystem.hpp>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <stdlib.h> //getenv
#endif


std::string OMW::Path::getPath(PathTypeEnum parType, const std::string parApp, const std::string parFile)
{
    std::string theBasePath;
    if(parType == GLOBAL_CFG_PATH)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        theBasePath = Ogre::macBundlePath() + "/Contents/MacOS/"; //FIXME do we have global/local with OSX?
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        theBasePath = "/etc/"+parApp+"/";
#else
        theBasePath = "";
#endif

    }
    else
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        theBasePath = Ogre::macBundlePath() + "/Contents/MacOS/"; //FIXME do we have global/local with OSX?
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        const char* theDir;
        if ((theDir = getenv("OPENMW_HOME")) != NULL)
        {
            theBasePath = std::string(theDir)+"/";
        }
        else
        {
            if ((theDir = getenv("XDG_CONFIG_HOME")))
            {
                theBasePath = std::string(theDir)+"/"+parApp+"/";
            }
            else
            {
                if ((theDir = getenv("HOME")) == NULL)
                    return parFile;
                theBasePath = std::string(theDir)+"/.config/"+parApp+"/";
            }
        }
        boost::filesystem::create_directories(boost::filesystem::path(theBasePath));
#else
        theBasePath = "";
#endif
    }

    theBasePath.append(parFile);
    return theBasePath;
}

