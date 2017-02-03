#ifndef OPENMW_COMPONENTS_SCENEUTIL_ATTACH_H
#define OPENMW_COMPONENTS_SCENEUTIL_ATTACH_H

#include <string>

#include <osg/ref_ptr>

namespace osg
{
    class Node;
    class Group;
}

namespace SceneUtil
{

    /// Attach parts of the \a toAttach scenegraph to the \a master scenegraph, using the specified filter and attachment node.
    /// If the \a toAttach scene graph contains skinned objects, we will attach only those (filtered by the \a filter).
    /// Otherwise, just attach all of the toAttach scenegraph to the attachment node on the master scenegraph, with no filtering.
    /// @note The master scene graph is expected to include a skeleton.
    /// @return A newly created node that is directly attached to the master scene graph
    osg::ref_ptr<osg::Node> attach(osg::ref_ptr<osg::Node> toAttach, osg::Node* master, const std::string& filter, osg::Group* attachNode);

}

#endif
