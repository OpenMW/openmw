#ifndef GAME_RENDER_CELLBORDER
#define GAME_RENDER_CELLBORDER

#include <map>
#include <osg/Group>

namespace Terrain
{
    class World;
}

namespace MWRender
{
    /**
     * @Brief Handles the debug cell borders.
     */
    class CellBorder
    {
    public:
        typedef std::map<std::pair<int, int>, osg::ref_ptr<osg::Node> > CellGrid; 

        CellBorder(Terrain::World *world, osg::Group *root, int borderMask);

        void createCellBorderGeometry(int x, int y);
        void destroyCellBorderGeometry(int x, int y);

        /**
          Destroys the geometry for all borders.
        */
        void destroyCellBorderGeometry();

    protected:
        Terrain::World *mWorld;
        osg::Group *mRoot;

        CellGrid mCellBorderNodes;
        int mBorderMask;
    };
}

#endif
