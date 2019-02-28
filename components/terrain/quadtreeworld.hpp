#ifndef COMPONENTS_TERRAIN_QUADTREEWORLD_H
#define COMPONENTS_TERRAIN_QUADTREEWORLD_H

#include "world.hpp"

#include <OpenThreads/Mutex>

namespace osg
{
    class NodeVisitor;
}

namespace Terrain
{
    class RootNode;
    class ViewDataMap;

    /// @brief Terrain implementation that loads cells into a Quad Tree, with geometry LOD and texture LOD. The entire world is displayed at all times.
    class QuadTreeWorld : public Terrain::World
    {
    public:
        QuadTreeWorld(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask, int borderMask, int compMapResolution, float comMapLevel, float lodFactor, int vertexLodMod, float maxCompGeometrySize);

        ~QuadTreeWorld();

        void accept(osg::NodeVisitor& nv);

        virtual void enable(bool enabled);

        void cacheCell(View *view, int x, int y);

        View* createView();
        void preload(View* view, const osg::Vec3f& eyePoint);

        void reportStats(unsigned int frameNumber, osg::Stats* stats);

        virtual void setDefaultViewer(osg::Object* obj);

    private:
        void ensureQuadTreeBuilt();

        osg::ref_ptr<RootNode> mRootNode;

        osg::ref_ptr<ViewDataMap> mViewDataMap;

        OpenThreads::Mutex mQuadTreeMutex;
        bool mQuadTreeBuilt;
        float mLodFactor;
        int mVertexLodMod;
    };

}

#endif
