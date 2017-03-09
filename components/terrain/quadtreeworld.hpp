#ifndef COMPONENTS_TERRAIN_QUADTREEWORLD_H
#define COMPONENTS_TERRAIN_QUADTREEWORLD_H

#include <map>

#include "world.hpp"

namespace SceneUtil
{
    class WorkQueue;
}

namespace osg
{
    class NodeVisitor;
}

namespace Terrain
{
    class RootNode;
    class BuildQuadTreeItem;
    class ViewDataMap;

    /// @brief Terrain implementation that loads cells into a Quad Tree, with geometry LOD and texture LOD. The entire world is displayed at all times.
    class QuadTreeWorld : public Terrain::World
    {
    public:
        QuadTreeWorld(SceneUtil::WorkQueue* workQueue, osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask=~0);
        ~QuadTreeWorld();

        void accept(osg::NodeVisitor& nv);

        virtual void loadCell(int x, int y);
        virtual osg::ref_ptr<osg::Node> cacheCell(int x, int y);

        virtual void enable(bool enabled);

    private:
        osg::ref_ptr<RootNode> mRootNode;

        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;

        osg::ref_ptr<BuildQuadTreeItem> mWorkItem;

        osg::ref_ptr<ViewDataMap> mViewDataMap;

        bool mQuadTreeBuilt;
    };

}

#endif
