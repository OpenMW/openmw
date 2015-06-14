#ifndef COMPONENTS_TERRAIN_TERRAINGRID_H
#define COMPONENTS_TERRAIN_TERRAINGRID_H

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
        typedef std::map<std::pair<int, int>, GridElement*> Grid;
        Grid mGrid;

        osg::ref_ptr<osg::KdTreeBuilder> mKdTreeBuilder;
    };

}

#endif
