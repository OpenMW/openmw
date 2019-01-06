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

        virtual bool isSufficientDetail(QuadTreeNode *node, float dist) = 0;
    };

    class ViewDataMap;
    class ViewData;

    class QuadTreeNode : public osg::Group
    {
    public:
        QuadTreeNode(QuadTreeNode* parent, ChildDirection dir, float size, const osg::Vec2f& center);
        virtual ~QuadTreeNode();

        inline QuadTreeNode* getParent() { return mParent; }
        inline QuadTreeNode* getChild(unsigned int i) { return static_cast<QuadTreeNode*>(Group::getChild(i)); }
        inline unsigned int getNumChildren() const { return _children.size(); }

        /// Faster version of osg::Group::addChild
        void addChild(QuadTreeNode* child)
        {
            _children.reserve(4);
            _children.push_back(child);
            child->addParent(this);
        };

        float distance(const osg::Vec3f& v) const;

        /// Returns our direction relative to the parent node, or Root if we are the root node.
        ChildDirection getDirection() { return mDirection; }

        /// Get neighbour node in this direction
        QuadTreeNode* getNeighbour (Direction dir);

        void setBoundingBox(const osg::BoundingBox& boundingBox);
        const osg::BoundingBox& getBoundingBox() const { return mBoundingBox; }
        bool hasValidBounds() const { return mValidBounds; }

        virtual osg::BoundingSphere computeBound() const;

        /// size in cell coordinates
        float getSize() const { return mSize; }

        /// center in cell coordinates
        const osg::Vec2f& getCenter() const { return mCenter; }

        /// Traverse nodes according to LOD selection.
        void traverse(ViewData* vd, const osg::Vec3f& viewPoint, LodCallback* lodCallback, float maxDist);

        /// Traverse to a specific node and add only that node.
        void traverseTo(ViewData* vd, float size, const osg::Vec2f& center);

        /// Adds all leaf nodes which intersect the line from start to end
        void intersect(ViewData* vd, const osg::Vec3f& start, const osg::Vec3f& end);

    private:
        QuadTreeNode* mParent;

        ChildDirection mDirection;

        osg::BoundingBox mBoundingBox;
        bool mValidBounds;
        float mSize;
        osg::Vec2f mCenter;
    };

}

#endif
