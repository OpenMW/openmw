#include "cellborder.hpp"

#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/Geometry>
#include <osg/Geode>

#include "world.hpp"
#include "../esm/loadland.hpp"

#include <components/resource/scenemanager.hpp>
#include <components/terrain/storage.hpp>

namespace Terrain
{

CellBorder::CellBorder(Terrain::World *world, osg::Group *root, int borderMask, Resource::SceneManager* sceneManager)
    : mWorld(world)
    , mSceneManager(sceneManager)
    , mRoot(root)
    , mBorderMask(borderMask)
{
}

osg::ref_ptr<osg::Geode> CellBorder::createBorderGeometry(float x, float y, float size, Terrain::Storage* terrain, Resource::SceneManager* sceneManager, int mask,
    float offset, osg::Vec4f color)
{
    const int cellSize = ESM::Land::REAL_SIZE;
    const int borderSegments = 40;

    osg::Vec3 cellCorner = osg::Vec3(x * cellSize,y * cellSize,0);
    size *= cellSize;

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;

    normals->push_back(osg::Vec3(0.0f,-1.0f, 0.0f));

    float borderStep = size / ((float)borderSegments);

    for (int i = 0; i <= 2 * borderSegments; ++i)
    {
        osg::Vec3f pos = i < borderSegments ?
            osg::Vec3(i * borderStep,0.0f,0.0f) :
            osg::Vec3(size, (i - borderSegments) * borderStep,0.0f);

        pos += cellCorner;
        pos += osg::Vec3f(0,0, terrain->getHeightAt(pos) + offset);

        vertices->push_back(pos);

        osg::Vec4f col = i % 2 == 0 ?
            osg::Vec4f(0,0,0,1) :
            color;

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
    osg::ref_ptr<osg::Material> material (new osg::Material);
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    stateSet->setAttribute(material);

    osg::PolygonMode* polygonmode = new osg::PolygonMode;
    polygonmode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateSet->setAttributeAndModes(polygonmode,osg::StateAttribute::ON);

    sceneManager->recreateShaders(borderGeode, "debug");
    borderGeode->setNodeMask(mask);

    return borderGeode;
}

void CellBorder::createCellBorderGeometry(int x, int y)
{
    auto borderGeode = createBorderGeometry(x, y, 1.f, mWorld->getStorage(), mSceneManager, mBorderMask);
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
