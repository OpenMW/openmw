#include "pathgrid.hpp"

#include <cassert>

#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/Group>

#include <components/esm/loadstat.hpp>
#include <components/esm/loadpgrd.hpp>
#include <components/sceneutil/pathgridutil.hpp>

#include "../mwbase/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/pathfinding.hpp"
#include "../mwmechanics/coordinateconverter.hpp"

#include "vismask.hpp"

namespace MWRender
{

Pathgrid::Pathgrid(osg::ref_ptr<osg::Group> root)
    : mPathgridEnabled(false)
    , mRootNode(root)
    , mPathGridRoot(NULL)
    , mInteriorPathgridNode(NULL)
{
}

Pathgrid::~Pathgrid()
{
    if (mPathgridEnabled)
    {
        togglePathgrid();
    }
}


bool Pathgrid::toggleRenderMode (int mode){
    switch (mode)
    {
        case Render_Pathgrid:
            togglePathgrid();
            return mPathgridEnabled;
        default:
            return false;
    }

    return false;
}

void Pathgrid::addCell(const MWWorld::CellStore *store)
{
    mActiveCells.push_back(store);
    if (mPathgridEnabled)
        enableCellPathgrid(store);
}

void Pathgrid::removeCell(const MWWorld::CellStore *store)
{
    mActiveCells.erase(std::remove(mActiveCells.begin(), mActiveCells.end(), store), mActiveCells.end());
    if (mPathgridEnabled)
        disableCellPathgrid(store);
}

void Pathgrid::togglePathgrid()
{
    mPathgridEnabled = !mPathgridEnabled;
    if (mPathgridEnabled)
    {
        // add path grid meshes to already loaded cells
        mPathGridRoot = new osg::Group;
        mPathGridRoot->setNodeMask(Mask_Debug);
        mRootNode->addChild(mPathGridRoot);

        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); ++it)
        {
            enableCellPathgrid(*it);
        }
    }
    else
    {
        // remove path grid meshes from already loaded cells
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); ++it)
        {
            disableCellPathgrid(*it);
        }

        if (mPathGridRoot)
        {
            mRootNode->removeChild(mPathGridRoot);
            mPathGridRoot = NULL;
        }
    }
}

void Pathgrid::enableCellPathgrid(const MWWorld::CellStore *store)
{
    MWBase::World* world = MWBase::Environment::get().getWorld();
    const ESM::Pathgrid *pathgrid =
        world->getStore().get<ESM::Pathgrid>().search(*store->getCell());
    if (!pathgrid) return;

    osg::Vec3f cellPathGridPos(0, 0, 0);
    MWMechanics::CoordinateConverter(store->getCell()).toWorld(cellPathGridPos);

    osg::ref_ptr<osg::PositionAttitudeTransform> cellPathGrid = new osg::PositionAttitudeTransform;
    cellPathGrid->setPosition(cellPathGridPos);

    osg::ref_ptr<osg::Geometry> geometry = SceneUtil::createPathgridGeometry(*pathgrid);

    cellPathGrid->addChild(geometry);

    mPathGridRoot->addChild(cellPathGrid);

    if (store->getCell()->isExterior())
    {
        mExteriorPathgridNodes[std::make_pair(store->getCell()->getGridX(), store->getCell()->getGridY())] = cellPathGrid;
    }
    else
    {
        assert(mInteriorPathgridNode == NULL);
        mInteriorPathgridNode = cellPathGrid;
    }
}

void Pathgrid::disableCellPathgrid(const MWWorld::CellStore *store)
{
    if (store->getCell()->isExterior())
    {
        ExteriorPathgridNodes::iterator it =
                mExteriorPathgridNodes.find(std::make_pair(store->getCell()->getGridX(), store->getCell()->getGridY()));
        if (it != mExteriorPathgridNodes.end())
        {
            mPathGridRoot->removeChild(it->second);
            mExteriorPathgridNodes.erase(it);
        }
    }
    else
    {
        if (mInteriorPathgridNode)
        {
            mPathGridRoot->removeChild(mInteriorPathgridNode);
            mInteriorPathgridNode = NULL;
        }
    }
}

}
