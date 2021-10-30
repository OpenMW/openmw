#ifndef COMPONENTS_TERRAIN_QUADTREEWORLD_H
#define COMPONENTS_TERRAIN_QUADTREEWORLD_H

#include "world.hpp"
#include "terraingrid.hpp"

#include <mutex>
#include <memory>

namespace osg
{
    class NodeVisitor;
}

namespace Terrain
{
    class RootNode;
    class ViewDataMap;
    class ViewData;
    struct ViewDataEntry;
    
    class DebugChunkManager;

    /// @brief Terrain implementation that loads cells into a Quad Tree, with geometry LOD and texture LOD.
    class QuadTreeWorld : public TerrainGrid // note: derived from TerrainGrid is only to render default cells (see loadCell)
    {
    public:
        QuadTreeWorld(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, unsigned int nodeMask, unsigned int preCompileMask, unsigned int borderMask, int compMapResolution, float comMapLevel, float lodFactor, int vertexLodMod, float maxCompGeometrySize, bool debugChunks);

        ~QuadTreeWorld();

        void accept(osg::NodeVisitor& nv);

        void enable(bool enabled) override;

        void setViewDistance(float distance) override;

        void cacheCell(View *view, int x, int y) override {}
        /// @note Not thread safe.
        void loadCell(int x, int y) override;
        /// @note Not thread safe.
        void unloadCell(int x, int y) override;

        View* createView() override;
        void preload(View* view, const osg::Vec3f& eyePoint, const osg::Vec4i &cellgrid, std::atomic<bool>& abort, Loading::Reporter& reporter) override;
        void rebuildViews() override;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) override;

        class ChunkManager
        {
        public:
            virtual ~ChunkManager(){}
            virtual osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile) = 0;
            virtual unsigned int getNodeMask() { return 0; }

            void setViewDistance(float viewDistance) { mViewDistance = viewDistance; }
            float getViewDistance() const { return mViewDistance; }
        private:
            float mViewDistance = 0.f;
        };
        void addChunkManager(ChunkManager*);

    private:
        void ensureQuadTreeBuilt();
        void loadRenderingNode(ViewDataEntry& entry, ViewData* vd, float cellWorldSize, const osg::Vec4i &gridbounds, bool compile, float reuseDistance);

        osg::ref_ptr<RootNode> mRootNode;

        osg::ref_ptr<ViewDataMap> mViewDataMap;

        std::vector<ChunkManager*> mChunkManagers;

        std::mutex mQuadTreeMutex;
        bool mQuadTreeBuilt;
        float mLodFactor;
        int mVertexLodMod;
        float mViewDistance;
        float mMinSize;
        bool mDebugTerrainChunks;
        std::unique_ptr<DebugChunkManager> mDebugChunkManager;
        float mRevalidateDistance;
    };

}

#endif
