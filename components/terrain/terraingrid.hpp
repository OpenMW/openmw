#ifndef COMPONENTS_TERRAIN_TERRAINGRID_H
#define COMPONENTS_TERRAIN_TERRAINGRID_H

#include <map>

#include <osg/Vec2f>

#include "world.hpp"

namespace Terrain
{

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD. Only requested cells are loaded.
    class TerrainGrid : public Terrain::World
    {
    public:
        TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask=~0, int borderMask=0);
        ~TerrainGrid();

        virtual void cacheCell(View* view, int x, int y);

        /// @note Not thread safe.
        virtual void loadCell(int x, int y);

        /// @note Not thread safe.
        virtual void unloadCell(int x, int y);

        View* createView();

    protected:
        bool isGridEmpty() const { return mGrid.empty(); }

    private:
        osg::ref_ptr<osg::Node> buildTerrain (osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter);
        void updateWaterCulling();

        // split each ESM::Cell into mNumSplits*mNumSplits terrain chunks
        unsigned int mNumSplits;

        CellBorder::CellGrid mGrid;
    };
}

#endif
