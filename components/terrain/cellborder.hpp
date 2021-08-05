#ifndef GAME_RENDER_CELLBORDER
#define GAME_RENDER_CELLBORDER

#include <map>
#include <osg/Group>

namespace Resource
{
    class SceneManager;
}

namespace Terrain
{
    class World;

    /**
     * @Brief Handles the debug cell borders.
     */
    class CellBorder
    {
    public:
        typedef std::map<std::pair<int, int>, osg::ref_ptr<osg::Node> > CellGrid; 

        CellBorder(Terrain::World *world, osg::Group *root, int borderMask, Resource::SceneManager* sceneManager);

        void createCellBorderGeometry(int x, int y);
        void destroyCellBorderGeometry(int x, int y);

        /**
          Destroys the geometry for all borders.
        */
        void destroyCellBorderGeometry();

    protected:
        Terrain::World *mWorld;
        Resource::SceneManager* mSceneManager;
        osg::Group *mRoot;

        CellGrid mCellBorderNodes;
        int mBorderMask;
    };
}

#endif
