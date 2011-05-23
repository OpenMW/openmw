#ifndef COMPONENTS_FILES_PATH_HPP
#define COMPONENTS_FILES_PATH_HPP

#include <string>

namespace Files
{
    enum PathTypeEnum
    {
        Path_ConfigUser,
        Path_ConfigGlobal
    };

    std::string getPath (PathTypeEnum parType, const std::string parApp, const std::string parFile);
}

#endif
