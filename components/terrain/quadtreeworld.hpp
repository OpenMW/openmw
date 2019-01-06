#ifndef COMPONENTS_TERRAIN_QUADTREEWORLD_H
#define COMPONENTS_TERRAIN_QUADTREEWORLD_H

#include "world.hpp"
#include "terraingrid.hpp"

#include <OpenThreads/Mutex>

namespace osg
{
    class NodeVisitor;
}

namespace Terrain
{
    class RootNode;
    class ViewDataMap;

    /// @brief Terrain implementation that loads cells into a Quad Tree, with geometry LOD and texture LOD.
    /// @note The geometry LOD relies on Stitching to connect different LODs. This gives a seamless appearance but with Sticthing you cant use more advanced LOD techniques because siblings must have the same LOD for the connections to work.. I think its a bad choice but i wont rewrite it for now
    class QuadTreeWorld : public TerrainGrid // note: derived from TerrainGrid is only to render default cells (see loadCell)
    {
    public:
        QuadTreeWorld(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask, int borderMask, int compositeMapResolution, float compositeMapLevel, float lodFactor, bool waitForCompositeMaps, int vertexLodMod);
        ~QuadTreeWorld();

        void accept(osg::NodeVisitor& nv);

        virtual void enable(bool enabled);

        virtual void setViewDistance(float distance) { mViewDistance = distance; }

        void cacheCell(View *view, int x, int y);
        /// @note Not thread safe.
        virtual void loadCell(int x, int y);
        /// @note Not thread safe.
        virtual void unloadCell(int x, int y);

        View* createView();
        void preload(View* view, const osg::Vec3f& viewPoint, volatile bool& abort);
        void storeView(const View* view, double referenceTime);

        void reportStats(unsigned int frameNumber, osg::Stats* stats);

    private:
        void ensureQuadTreeBuilt();

        osg::ref_ptr<RootNode> mRootNode;

        osg::ref_ptr<ViewDataMap> mViewDataMap;

        OpenThreads::Mutex mQuadTreeMutex;
        bool mQuadTreeBuilt;
        float mLodFactor;
        bool mWaitForCompositeMaps;
        float mViewDistance;
        int mVertexLodMod;
    };

}

#endif
