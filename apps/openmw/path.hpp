#ifndef PATH__HPP
#define PATH__HPP

#include <OgrePlatform.h>
#include <string>

#if OGRE_PLATFORM_LINUX
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h> //getenv
#endif

namespace OMW
{
    class Path
    {
        public:
            enum PathTypeEnum
            {
                USER_CFG_PATH,
                GLOBAL_CFG_PATH
            };

            //TODO use application data dir on windows?
            static std::string getPath(PathTypeEnum parType, const std::string parApp, const std::string parFile)
            {
                std::string theBasePath;
                if(parType == GLOBAL_CFG_PATH)
                {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                    theBasePath = macBundlePath() + "/Contents/MacOS/"; //FIXME do we have global/local with OSX?
#elif OGRE_PLATFORM_LINUX
                    theBasePath = "/etc/"+parApp+"/";
#else
                    theBasePath = "";
#endif

                }
                else
                {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                    theBasePath = macBundlePath() + "/Contents/MacOS/"; //FIXME do we have global/local with OSX?
#elif OGRE_PLATFORM_LINUX
                    const char* homedir;
                    if ((homedir = getenv("HOME")) == NULL)
                        return NULL;
                    theBasePath = std::string(homedir)+"/."+parApp+"/";
                    mkdir(theBasePath.c_str(), 0777);
#else
                    theBasePath = "";
#endif
                }

                theBasePath.append(parFile);
                return theBasePath;
            }
    };
}
#endif
