#ifndef OPENMW_MWRENDER_UTIL_H
#define OPENMW_MWRENDER_UTIL_H

#include <osg/NodeCallback>
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
    // Overrides the texture of nodes in the mesh that had the same NiTexturingProperty as the first NiTexturingProperty of the .NIF file's root node,
    // if it had a NiTexturingProperty. Used for applying "particle textures" to magic effects.
    void overrideFirstRootTexture(const std::string &texture, Resource::ResourceSystem *resourceSystem, osg::ref_ptr<osg::Node> node);

    void overrideTexture(const std::string& texture, Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Node> node);

    // Node callback to entirely skip the traversal.
    class NoTraverseCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            // no traverse()
        }
    };
}

#endif
