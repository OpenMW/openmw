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

        virtual void apply(osg::Group& group)
        {
            if (group.getName() == mNameToFind)
            {
                mFoundNode = &group;
                return;
            }
            traverse(group);
        }

        std::string mNameToFind;
        osg::Group* mFoundNode;
    };

}

#endif
