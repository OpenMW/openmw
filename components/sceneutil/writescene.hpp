#ifndef OPENMW_COMPONENTS_WRITESCENE_H
#define OPENMW_COMPONENTS_WRITESCENE_H

#include <string>

namespace osg
{
    class Node;
}

namespace SceneUtil
{

    void writeScene(osg::Node* node, const std::string& filename, const std::string& format);

}

#endif
