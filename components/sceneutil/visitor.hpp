#ifndef OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_VISITOR_H

#include <osg/MatrixTransform>
#include <osg/NodeVisitor>

#include <string_view>
#include <unordered_map>

#include <components/misc/strings/algorithm.hpp>

// Commonly used scene graph visitors
namespace SceneUtil
{

    // Find a Group by name, case-insensitive
    // If not found, mFoundNode will be nullptr
    class FindByNameVisitor : public osg::NodeVisitor
    {
    public:
        FindByNameVisitor(std::string_view nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
            , mFoundNode(nullptr)
        {
        }

        void apply(osg::Group& group) override;
        void apply(osg::MatrixTransform& node) override;
        void apply(osg::Geometry& node) override;

        bool checkGroup(osg::Group& group);

        std::string_view mNameToFind;
        osg::Group* mFoundNode;
    };

    class FindByClassVisitor : public osg::NodeVisitor
    {
    public:
        FindByClassVisitor(std::string_view nameToFind)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mNameToFind(nameToFind)
        {
        }

        void apply(osg::Node& node) override;

        std::string_view mNameToFind;
        std::vector<osg::Node*> mFoundNodes;
    };

    typedef std::unordered_map<std::string, osg::ref_ptr<osg::MatrixTransform>, Misc::StringUtils::CiHash,
        Misc::StringUtils::CiEqual>
        NodeMap;

    /// Maps names to nodes
    class NodeMapVisitor : public osg::NodeVisitor
    {
    public:
        NodeMapVisitor(NodeMap& map)
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
            , mMap(map)
        {
        }

        void apply(osg::MatrixTransform& trans) override;

    private:
        NodeMap& mMap;
    };

    /// Maps names to bone nodes
    class NodeMapVisitorBoneOnly : public osg::NodeVisitor
    {
    public:
        NodeMapVisitorBoneOnly(NodeMap& map)
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
            , mMap(map)
        {
        }

        void apply(osg::MatrixTransform& trans) override;

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
        typedef std::vector<std::pair<osg::Node*, osg::Group*>> RemoveVec;
        std::vector<std::pair<osg::Node*, osg::Group*>> mToRemove;
    };

    // Removes all drawables from a graph.
    class CleanObjectRootVisitor : public RemoveVisitor
    {
    public:
        void apply(osg::Drawable& drw) override;
        void apply(osg::Group& node) override;
        void apply(osg::MatrixTransform& node) override;
        void apply(osg::Node& node) override;

        void applyNode(osg::Node& node);
        void applyDrawable(osg::Node& node);
    };

    class RemoveTriBipVisitor : public RemoveVisitor
    {
    public:
        void apply(osg::Drawable& drw) override;
        void apply(osg::Group& node) override;
        void apply(osg::MatrixTransform& node) override;

        void applyImpl(osg::Node& node);
    };
}

#endif
