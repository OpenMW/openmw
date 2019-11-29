#include "cellborder.hpp"

#include <osg/PolygonMode>
#include <osg/Geometry>
#include <osg/Geode>

#include "world.hpp"
#include "../esm/loadland.hpp"

namespace Terrain
{

CellBorder::CellBorder(Terrain::World *world, osg::Group *root, int borderMask):
    mWorld(world),
    mRoot(root),
    mBorderMask(borderMask)
{
}

void CellBorder::createCellBorderGeometry(int x, int y)
{
    const int cellSize = ESM::Land::REAL_SIZE;
    const int borderSegments = 40;
    const float offset = 10.0;

    osg::Vec3 cellCorner = osg::Vec3(x * cellSize,y * cellSize,0);

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;

    normals->push_back(osg::Vec3(0.0f,-1.0f, 0.0f));

    float borderStep = cellSize / ((float) borderSegments);

    for (int i = 0; i <= 2 * borderSegments; ++i)
    {
        osg::Vec3f pos = i < borderSegments ?
            osg::Vec3(i * borderStep,0.0f,0.0f) :
            osg::Vec3(cellSize,(i - borderSegments) * borderStep,0.0f);

        pos += cellCorner;
        pos += osg::Vec3f(0,0,mWorld->getHeightAt(pos) + offset);

        vertices->push_back(pos);

        osg::Vec4f col = i % 2 == 0 ?
            osg::Vec4f(0,0,0,1) :
            osg::Vec4f(1,1,0,1);

        colors->push_back(col);
    }

    osg::ref_ptr<osg::Geometry> border = new osg::Geometry;
    border->setVertexArray(vertices.get());
    border->setNormalArray(normals.get());
    border->setNormalBinding(osg::Geometry::BIND_OVERALL);
    border->setColorArray(colors.get());
    border->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    border->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP,0,vertices->size()));

    osg::ref_ptr<osg::Geode> borderGeode = new osg::Geode;
    borderGeode->addDrawable(border.get());

    osg::StateSet *stateSet = borderGeode->getOrCreateStateSet();

    osg::PolygonMode* polygonmode = new osg::PolygonMode;
    polygonmode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateSet->setAttributeAndModes(polygonmode,osg::StateAttribute::ON);

    borderGeode->setNodeMask(mBorderMask);

    mRoot->addChild(borderGeode);

    mCellBorderNodes[std::make_pair(x,y)] = borderGeode;
}

void CellBorder::destroyCellBorderGeometry(int x, int y)
{
    CellGrid::iterator it = mCellBorderNodes.find(std::make_pair(x,y));

    if (it == mCellBorderNodes.end())
        return;

    osg::ref_ptr<osg::Node> borderNode = it->second;
    mRoot->removeChild(borderNode);

    mCellBorderNodes.erase(it);
}

void CellBorder::destroyCellBorderGeometry()
{
    for (const auto& v : mCellBorderNodes)
        mRoot->removeChild(v.second);
    mCellBorderNodes.clear();
}

}
