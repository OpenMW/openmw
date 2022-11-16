#ifndef OPENMW_COMPONENTS_WRITESCENE_H
#define OPENMW_COMPONENTS_WRITESCENE_H

#include <filesystem>
#include <string>

namespace osg
{
    class Node;
}

namespace SceneUtil
{

    void writeScene(osg::Node* node, const std::filesystem::path& filename, const std::string& format);

}

#endif
