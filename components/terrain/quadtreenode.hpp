#ifndef COMPONENTS_TERRAIN_QUADTREENODE_H
#define COMPONENTS_TERRAIN_QUADTREENODE_H

#include <OgreAxisAlignedBox.h>
#include <OgreVector2.h>
#include <OgreTexture.h>

#include <components/loadinglistener/loadinglistener.hpp>

namespace Ogre
{
    class Rectangle2D;
}

namespace Terrain
{
    class World;
    class Chunk;
    class MaterialGenerator;

    enum Direction
    {
        North = 0,
        East = 1,
        South = 2,
        West = 3
    };

    enum ChildDirection
    {
        NW = 0,
        NE = 1,
        SW = 2,
        SE = 3,
        Root
    };

    /**
     * @brief A node in the quad tree for our terrain. Depending on LOD,
     *        a node can either choose to render itself in one batch (merging its children),
     *        or delegate the render process to its children, rendering each child in at least one batch.
     */
    class QuadTreeNode
    {
    public:
        /// @param terrain
        /// @param dir relative to parent, or Root if we are the root node
        /// @param size size (in *cell* units!)
        /// @param center center (in *cell* units!)
        /// @param parent parent node
        QuadTreeNode (World* terrain, ChildDirection dir, float size, const Ogre::Vector2& center, QuadTreeNode* parent);
        ~QuadTreeNode();

        void setVisible(bool visible);

        /// Rebuild all materials
        void applyMaterials();

        /// Initialize neighbours - do this after the quadtree is built
        void initNeighbours();
        /// Initialize bounding boxes of non-leafs by merging children bounding boxes.
        /// Do this after the quadtree is built - note that leaf bounding boxes
        /// need to be set first via setBoundingBox!
        void initAabb();

        /// @note takes ownership of \a child
        void createChild (ChildDirection id, float size, const Ogre::Vector2& center);

        /// Mark this node as a dummy node. This can happen if the terrain size isn't a power of two.
        /// For the QuadTree to work, we need to round the size up to a power of two, which means we'll
        /// end up with empty nodes that don't actually render anything.
        void markAsDummy() { mIsDummy = true; }
        bool isDummy() { return mIsDummy; }

        QuadTreeNode* getParent() { return mParent; }

        Ogre::SceneNode* getSceneNode() { return mSceneNode; }

        int getSize() { return mSize; }
        Ogre::Vector2 getCenter() { return mCenter; }

        bool hasChildren() { return mChildren[0] != 0; }
        QuadTreeNode* getChild(ChildDirection dir) { return mChildren[dir]; }

        /// Get neighbour node in this direction
        QuadTreeNode* getNeighbour (Direction dir);

        /// Returns our direction relative to the parent node, or Root if we are the root node.
        ChildDirection getDirection() { return mDirection; }

        /// Set bounding box in local coordinates. Should be done at load time for leaf nodes.
        /// Other nodes can merge AABB of child nodes.
        void setBoundingBox (const Ogre::AxisAlignedBox& box);

        /// Get bounding box in local coordinates
        const Ogre::AxisAlignedBox& getBoundingBox();

        World* getTerrain() { return mTerrain; }

        /// Adjust LODs for the given camera position, possibly splitting up chunks or merging them.
        void update (const Ogre::Vector3& cameraPos, Loading::Listener* loadingListener);

        /// Adjust index buffers of chunks to stitch together chunks of different LOD, so that cracks are avoided.
        /// Call after QuadTreeNode::update!
        void updateIndexBuffers();

        /// Destroy chunks rendered by this node *and* its children (if param is true)
        void destroyChunks(bool children);

        /// Get the effective LOD level if this node was rendered in one chunk
        /// with Storage::getCellVertices^2 vertices
        size_t getNativeLodLevel() { return mLodLevel; }

        /// Get the effective current LOD level used by the chunk rendering this node
        size_t getActualLodLevel();

        /// Is this node currently configured to render itself?
        bool hasChunk();

        /// Add a textured quad to a specific 2d area in the composite map scenemanager.
        /// Only nodes with size <= 1 can be rendered with alpha blending, so larger nodes will simply
        /// call this method on their children.
        /// @param area area in image space to put the quad
        /// @param quads collect quads here so they can be deleted later
        void prepareForCompositeMap(Ogre::TRect<float> area);

    private:
        // Stored here for convenience in case we need layer list again
        MaterialGenerator* mMaterialGenerator;

        /// Is this node (or any of its child nodes) currently configured to render itself?
        /// (only relevant when distant land is disabled, otherwise whole terrain is always rendered)
        bool mIsActive;

        bool mIsDummy;
        float mSize;
        size_t mLodLevel; // LOD if we were to render this node in one chunk
        Ogre::AxisAlignedBox mBounds;
        Ogre::AxisAlignedBox mWorldBounds;
        ChildDirection mDirection;
        Ogre::Vector2 mCenter;

        Ogre::SceneNode* mSceneNode;

        QuadTreeNode* mParent;
        QuadTreeNode* mChildren[4];
        QuadTreeNode* mNeighbours[4];

        Chunk* mChunk;

        World* mTerrain;

        Ogre::TexturePtr mCompositeMap;

        void ensureLayerInfo();
        void ensureCompositeMap();
    };

}

#endif
