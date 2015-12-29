#ifndef COMPONENTS_TERRAIN_TERRAINGRID_H
#define COMPONENTS_TERRAIN_TERRAINGRID_H

#include <osg/Vec2f>

#include "world.hpp"
#include "material.hpp"

namespace osg
{
    class KdTreeBuilder;
}

namespace Terrain
{

    class GridElement;

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD
    class TerrainGrid : public Terrain::World
    {
    public:
        TerrainGrid(osg::Group* parent, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
              Storage* storage, int nodeMask);
        ~TerrainGrid();

        virtual void loadCell(int x, int y);
        virtual void unloadCell(int x, int y);

    private:
        osg::ref_ptr<osg::Node> buildTerrain (osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter);

        // split each ESM::Cell into mNumSplits*mNumSplits terrain chunks
        unsigned int mNumSplits;

        typedef std::map<std::pair<int, int>, GridElement*> Grid;
        Grid mGrid;

        osg::ref_ptr<osg::KdTreeBuilder> mKdTreeBuilder;
    };

}

#endif
