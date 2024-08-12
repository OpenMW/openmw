#ifndef COMPONENTS_TERRAIN_TERRAINGRID_H
#define COMPONENTS_TERRAIN_TERRAINGRID_H

#include <map>

#include <osg/Vec2f>

#include "world.hpp"

namespace osg
{
    class Group;
    class Stats;
}

namespace Resource
{
    class ResourceSystem;
}

namespace Terrain
{
    class Storage;

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD. Only requested cells are loaded.
    class TerrainGrid : public Terrain::World
    {
    public:
        explicit TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem,
            Storage* storage, unsigned int nodeMask, ESM::RefId worldspace, double expiryDelay,
            unsigned int preCompileMask = ~0u, unsigned int borderMask = 0);
        TerrainGrid(osg::Group* parent, Storage* storage, ESM::RefId worldspace, unsigned int nodeMask = ~0u);
        ~TerrainGrid();

        void cacheCell(View* view, int x, int y) override;

        /// @note Not thread safe.
        void loadCell(int x, int y) override;

        /// @note Not thread safe.
        void unloadCell(int x, int y) override;

        View* createView() override;

    protected:
        bool isGridEmpty() const { return mGrid.empty(); }

    private:
        // quad is meant to be used for ESM4 terrain only; if -1 it is ignored, should be [0..3]
        osg::ref_ptr<osg::Node> buildTerrain(osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter, int quad = -1);
        void updateWaterCulling();

        // split each ESM::Cell into mNumSplits*mNumSplits terrain chunks
        unsigned int mNumSplits;

        CellBorder::CellGrid mGrid;
    };
}

#endif
