#ifndef GAME_RENDER_MWSCENE_H
#define GAME_RENDER_MWSCENE_H

#include <map>
#include <utility>
#include <vector>

#include <osg/Group>
#include <osg/ref_ptr>

namespace MWWorld
{
    class CellStore;
}

namespace MWRender
{
    class Pathgrid
    {
        bool mPathgridEnabled;

        void togglePathgrid();

        typedef std::vector<const MWWorld::CellStore*> CellList;
        CellList mActiveCells;

        osg::ref_ptr<osg::Group> mRootNode;

        osg::ref_ptr<osg::Group> mPathGridRoot;

        typedef std::map<std::pair<int, int>, osg::ref_ptr<osg::Group>> ExteriorPathgridNodes;
        ExteriorPathgridNodes mExteriorPathgridNodes;
        osg::ref_ptr<osg::Group> mInteriorPathgridNode;

        void enableCellPathgrid(const MWWorld::CellStore* store);
        void disableCellPathgrid(const MWWorld::CellStore* store);

    public:
        Pathgrid(osg::ref_ptr<osg::Group> root);
        ~Pathgrid();
        bool toggleRenderMode(int mode);

        void addCell(const MWWorld::CellStore* store);
        void removeCell(const MWWorld::CellStore* store);
    };

}

#endif
