#ifndef PATH__HPP
#define PATH__HPP

#include <OgrePlatform.h>
#include <string>

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

            static std::string getPath(PathTypeEnum parType, const std::string parApp, const std::string parFile);
    };
}
#endif
