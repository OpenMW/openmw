#ifndef OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H

#include <osg/MatrixTransform>
#include <osg/NodeVisitor>

// Commonly used scene graph visitors
namespace SceneUtil
{

    // Find a Group by name, case-insensitive
    // If not found, mFoundNode will be nullptr
    class FindByNameVisitor : public osg::NodeVisitor
    {
    public:
        FindByNameVisitor(const std::string& nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
            , mFoundNode(nullptr)
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

    /// Maps names to nodes
    class NodeMapVisitor : public osg::NodeVisitor
    {
    public:
        typedef std::map<std::string, osg::ref_ptr<osg::MatrixTransform> > NodeMap;

        NodeMapVisitor(NodeMap& map)
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
            , mMap(map)
        {
        }

        void apply(osg::MatrixTransform& trans);

    private:
        NodeMap& mMap;
    };

    /// @brief Base class for visitors that remove nodes from a scene graph.
    /// Subclasses need to fill the mToRemove vector.
    /// To use, node->accept(removeVisitor); removeVisitor.remove();
    class RemoveVisitor : public osg::NodeVisitor
    {
    public:
        RemoveVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void remove();

    protected:
        // <node to remove, parent node to remove it from>
        typedef std::vector<std::pair<osg::Node*, osg::Group*> > RemoveVec;
        std::vector<std::pair<osg::Node*, osg::Group*> > mToRemove;
    };

    // Removes all drawables from a graph.
    class CleanObjectRootVisitor : public RemoveVisitor
    {
    public:
        virtual void apply(osg::Drawable& drw);
        virtual void apply(osg::Group& node);
        virtual void apply(osg::MatrixTransform& node);
        virtual void apply(osg::Node& node);

        void applyNode(osg::Node& node);
        void applyDrawable(osg::Node& node);
    };

    class RemoveTriBipVisitor : public RemoveVisitor
    {
    public:
        virtual void apply(osg::Drawable& drw);
        virtual void apply(osg::Group& node);
        virtual void apply(osg::MatrixTransform& node);

        void applyImpl(osg::Node& node);
    };
}

#endif
