#ifndef OPENMW_COMPONENTS_TERRAIN_QUADTREENODE_H
#define OPENMW_COMPONENTS_TERRAIN_QUADTREENODE_H

#include <osg/Group>

#include "defs.hpp"

namespace Terrain
{

    enum ChildDirection
    {
        NW = 0,
        NE = 1,
        SW = 2,
        SE = 3,
        Root
    };

    class QuadTreeNode;
    class LodCallback : public osg::Referenced
    {
    public:
        virtual ~LodCallback() {}

        enum ReturnValue
        {
            Deeper,
            StopTraversal,
            StopTraversalAndUse
        };
        virtual ReturnValue isSufficientDetail(QuadTreeNode *node, float dist) = 0;
    };

    class ViewData;

    class QuadTreeNode : public osg::Group
    {
    public:
        QuadTreeNode(QuadTreeNode* parent, ChildDirection dir, float size, const osg::Vec2f& center);
        virtual ~QuadTreeNode();

        inline QuadTreeNode* getParent() { return mParent; }
        inline QuadTreeNode* getChild(unsigned int i) { return static_cast<QuadTreeNode*>(Group::getChild(i)); }
        inline unsigned int getNumChildren() const override { return _children.size(); }

        // osg::Group::addChild() does a lot of unrelated stuff, but we just really want to add a child node.
        void addChildNode(QuadTreeNode* child)
        {
            // QuadTree node should not contain more than 4 child nodes.
            // Reserve enough space if this node is supposed to have child nodes.
            _children.reserve(4);
            _children.push_back(child);
            child->addParent(this);
        };

        float distance(const osg::Vec3f& v) const;

        /// Returns our direction relative to the parent node, or Root if we are the root node.
        ChildDirection getDirection() { return mDirection; }

        /// Get neighbour node in this direction
        QuadTreeNode* getNeighbour (Direction dir);

        /// Initialize neighbours - do this after the quadtree is built
        void initNeighbours();

        void setBoundingBox(const osg::BoundingBox& boundingBox);
        const osg::BoundingBox& getBoundingBox() const;
        bool hasValidBounds() const { return mValidBounds; }

        /// size in cell coordinates
        float getSize() const;

        /// center in cell coordinates
        const osg::Vec2f& getCenter() const;

        /// Traverse nodes according to LOD selection.
        void traverseNodes(ViewData* vd, const osg::Vec3f& viewPoint, LodCallback* lodCallback);

    private:
        QuadTreeNode* mParent;

        QuadTreeNode* mNeighbours[4];

        ChildDirection mDirection;

        osg::BoundingBox mBoundingBox;
        bool mValidBounds;
        float mSize;
        osg::Vec2f mCenter;
    };

}

#endif
