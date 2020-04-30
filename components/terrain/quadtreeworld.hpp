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
    class LodCallback;

    /// @brief Terrain implementation that loads cells into a Quad Tree, with geometry LOD and texture LOD.
    class QuadTreeWorld : public TerrainGrid // note: derived from TerrainGrid is only to render default cells (see loadCell)
    {
    public:
        QuadTreeWorld(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask, int borderMask, int compMapResolution, float comMapLevel, float lodFactor, int vertexLodMod, float maxCompGeometrySize);

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
        void preload(View* view, const osg::Vec3f& eyePoint, const osg::Vec4i &cellgrid, std::atomic<bool>& abort);
        void storeView(const View* view, double referenceTime);

        void reportStats(unsigned int frameNumber, osg::Stats* stats);

        class ChunkManager
        {
        public:
            virtual ~ChunkManager(){}
            virtual osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags, bool far, const osg::Vec3f& viewPoint, bool compile) = 0;
            virtual unsigned int getNodeMask() { return 0; }
        };
        void addChunkManager(ChunkManager*);

    private:
        void ensureQuadTreeBuilt();

        osg::ref_ptr<RootNode> mRootNode;

        osg::ref_ptr<ViewDataMap> mViewDataMap;

        std::vector<ChunkManager*> mChunkManagers;

        OpenThreads::Mutex mQuadTreeMutex;
        bool mQuadTreeBuilt;
        float mLodFactor;
        int mVertexLodMod;
        float mViewDistance;
    };

}

#endif
