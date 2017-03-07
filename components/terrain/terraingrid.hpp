#ifndef COMPONENTS_TERRAIN_TERRAINGRID_H
#define COMPONENTS_TERRAIN_TERRAINGRID_H

#include <map>

#include <osg/Vec2f>

#include "world.hpp"

namespace osg
{
    class Texture2D;
}

namespace Terrain
{

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD
    class TerrainGrid : public Terrain::World
    {
    public:
        TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask=~0);
        ~TerrainGrid();

        /// Load a terrain cell and store it in cache for later use.
        /// @note The returned ref_ptr should be kept by the caller to ensure that the terrain stays in cache for as long as needed.
        /// @note Thread safe.
        virtual osg::ref_ptr<osg::Node> cacheCell(int x, int y);

        /// @note Not thread safe.
        virtual void loadCell(int x, int y);

        /// @note Not thread safe.
        virtual void unloadCell(int x, int y);

    private:
        osg::ref_ptr<osg::Node> buildTerrain (osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter);

        // split each ESM::Cell into mNumSplits*mNumSplits terrain chunks
        unsigned int mNumSplits;

        typedef std::map<std::pair<int, int>, osg::ref_ptr<osg::Node> > Grid;
        Grid mGrid;
    };

}

#endif
