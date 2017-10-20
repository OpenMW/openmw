#ifndef OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H

#include <osg/NodeVisitor>

// Commonly used scene graph visitors
namespace SceneUtil
{

    // Find a Group by name, case-insensitive
    // If not found, mFoundNode will be NULL
    class FindByNameVisitor : public osg::NodeVisitor
    {
    public:
        FindByNameVisitor(const std::string& nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
            , mFoundNode(NULL)
        {
        }

        virtual void apply(osg::Group& group);
        virtual void apply(osg::MatrixTransform& node);
        virtual void apply(osg::Geometry& node);

        bool checkGroup(osg::Group& group);

        std::string mNameToFind;
        osg::Group* mFoundNode;
    };

    class FindByClassVisitor : public osg::NodeVisitor
    {
    public:
        FindByClassVisitor(const std::string& nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
        {
        }

        virtual void apply(osg::Node &node);

        std::string mNameToFind;
        std::vector<osg::Node *> mFoundNodes;
    };

    // Disable freezeOnCull for all visited particlesystems
    class DisableFreezeOnCullVisitor : public osg::NodeVisitor
    {
    public:
        DisableFreezeOnCullVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        virtual void apply(osg::MatrixTransform& node);

        virtual void apply(osg::Drawable& drw);
    };

}

#endif
