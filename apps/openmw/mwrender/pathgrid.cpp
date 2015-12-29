#include "pathgrid.hpp"

#include <cassert>

#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/Group>

#include <components/esm/loadstat.hpp>
#include <components/esm/loadpgrd.hpp>

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

static const int POINT_MESH_BASE = 35;

osg::ref_ptr<osg::Geometry> Pathgrid::createPathgridLines(const ESM::Pathgrid *pathgrid)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;

    for(ESM::Pathgrid::EdgeList::const_iterator it = pathgrid->mEdges.begin();
        it != pathgrid->mEdges.end();
        ++it)
    {
        const ESM::Pathgrid::Edge &edge = *it;
        const ESM::Pathgrid::Point &p1 = pathgrid->mPoints[edge.mV0], &p2 = pathgrid->mPoints[edge.mV1];

        osg::Vec3f direction = MWMechanics::PathFinder::MakeOsgVec3(p2) - MWMechanics::PathFinder::MakeOsgVec3(p1);
        osg::Vec3f lineDisplacement = (direction^osg::Vec3f(0,0,1));
        lineDisplacement.normalize();

        lineDisplacement = lineDisplacement * POINT_MESH_BASE +
                                osg::Vec3f(0, 0, 10); // move lines up a little, so they will be less covered by meshes/landscape

        vertices->push_back(MWMechanics::PathFinder::MakeOsgVec3(p1) + lineDisplacement);
        vertices->push_back(MWMechanics::PathFinder::MakeOsgVec3(p2) + lineDisplacement);
    }

    geom->setVertexArray(vertices);

    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.f, 1.f, 0.f, 1.f));
    geom->setColorArray(colors, osg::Array::BIND_OVERALL);

    return geom;
}

osg::ref_ptr<osg::Geometry> Pathgrid::createPathgridPoints(const ESM::Pathgrid *pathgrid)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

    const float height = POINT_MESH_BASE * sqrtf(2);

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::UShortArray> indices = new osg::UShortArray;

    bool first = true;
    unsigned short startIndex = 0;
    for(ESM::Pathgrid::PointList::const_iterator it = pathgrid->mPoints.begin();
        it != pathgrid->mPoints.end();
        ++it, startIndex += 6)
    {
        osg::Vec3f pointPos(MWMechanics::PathFinder::MakeOsgVec3(*it));

        if (!first)
        {
            // degenerate triangle from previous octahedron
            indices->push_back(startIndex - 4); // 2nd point of previous octahedron
            indices->push_back(startIndex); // start point of current octahedron
        }

        float pointMeshBase = static_cast<float>(POINT_MESH_BASE);

        vertices->push_back(pointPos + osg::Vec3f(0, 0, height)); // 0
        vertices->push_back(pointPos + osg::Vec3f(-pointMeshBase, -pointMeshBase, 0)); // 1
        vertices->push_back(pointPos + osg::Vec3f(pointMeshBase, -pointMeshBase, 0)); // 2
        vertices->push_back(pointPos + osg::Vec3f(pointMeshBase, pointMeshBase, 0)); // 3
        vertices->push_back(pointPos + osg::Vec3f(-pointMeshBase, pointMeshBase, 0)); // 4
        vertices->push_back(pointPos + osg::Vec3f(0, 0, -height)); // 5

        indices->push_back(startIndex + 0);
        indices->push_back(startIndex + 1);
        indices->push_back(startIndex + 2);
        indices->push_back(startIndex + 5);
        indices->push_back(startIndex + 3);
        indices->push_back(startIndex + 4);
        // degenerates
        indices->push_back(startIndex + 4);
        indices->push_back(startIndex + 5);
        indices->push_back(startIndex + 5);
        // end degenerates
        indices->push_back(startIndex + 1);
        indices->push_back(startIndex + 4);
        indices->push_back(startIndex + 0);
        indices->push_back(startIndex + 3);
        indices->push_back(startIndex + 2);

        first = false;
    }

    geom->setVertexArray(vertices);

    geom->addPrimitiveSet(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, indices->size(), &(*indices)[0]));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.f, 0.f, 0.f, 1.f));
    geom->setColorArray(colors, osg::Array::BIND_OVERALL);

    return geom;
}

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

    osg::ref_ptr<osg::Geode> lineGeode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> lines = createPathgridLines(pathgrid);
    lineGeode->addDrawable(lines);

    osg::ref_ptr<osg::Geode> pointGeode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> points = createPathgridPoints(pathgrid);
    pointGeode->addDrawable(points);

    cellPathGrid->addChild(lineGeode);
    cellPathGrid->addChild(pointGeode);

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
