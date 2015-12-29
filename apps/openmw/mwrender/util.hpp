#ifndef OPENMW_MWRENDER_UTIL_H
#define OPENMW_MWRENDER_UTIL_H

#include <osg/ref_ptr>
#include <string>

namespace osg
{
    class Node;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{

    void overrideTexture(const std::string& texture, Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Node> node);

}

#endif
