#ifndef OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H

#include <osg/NodeVisitor>

// Commonly used scene graph visitors
namespace SceneUtil
{

    class FindByNameVisitor : public osg::NodeVisitor
    {
    public:
        FindByNameVisitor(const std::string& nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
            , mFoundNode(NULL)
        {
        }

        virtual void apply(osg::Node &node)
        {
            osg::Group* group = node.asGroup();
            if (group && node.getName() == mNameToFind)
            {
                mFoundNode = group;
                return;
            }
            traverse(node);
        }

        std::string mNameToFind;
        osg::Group* mFoundNode;
    };

}

#endif
