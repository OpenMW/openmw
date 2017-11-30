#include "world.hpp"

#include <osg/Group>
#include <osg/Material>
#include <osg/Camera>

#include <components/resource/resourcesystem.hpp>

#include "storage.hpp"
#include "texturemanager.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

#include <iostream>
#include <osg/ShapeDrawable>
#include <osg/PolygonMode>
#include <osg/Geometry>
#include <osg/Geode>

namespace Terrain
{

World::World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask)
    : mStorage(storage)
    , mParent(parent)
    , mResourceSystem(resourceSystem)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);
    mTerrainRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    osg::ref_ptr<osg::Material> material (new osg::Material);
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    mTerrainRoot->getOrCreateStateSet()->setAttributeAndModes(material, osg::StateAttribute::ON);

    mTerrainRoot->setName("Terrain Root");

    osg::ref_ptr<osg::Camera> compositeCam = new osg::Camera;
    compositeCam->setRenderOrder(osg::Camera::PRE_RENDER, -1);
    compositeCam->setProjectionMatrix(osg::Matrix::identity());
    compositeCam->setViewMatrix(osg::Matrix::identity());
    compositeCam->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    compositeCam->setClearMask(0);
    compositeCam->setNodeMask(preCompileMask);
    mCompositeMapCamera = compositeCam;

    compileRoot->addChild(compositeCam);

    mCompositeMapRenderer = new CompositeMapRenderer;
    compositeCam->addChild(mCompositeMapRenderer);

    mParent->addChild(mTerrainRoot);

    mTextureManager.reset(new TextureManager(mResourceSystem->getSceneManager()));
    mChunkManager.reset(new ChunkManager(mStorage, mResourceSystem->getSceneManager(), mTextureManager.get(), mCompositeMapRenderer));

    mResourceSystem->addResourceManager(mChunkManager.get());
    mResourceSystem->addResourceManager(mTextureManager.get());
}

void World::loadCell(int x, int y)
{
    const int cellSize = 8192;
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
        pos += osg::Vec3f(0,0,getHeightAt(pos) + offset);

        vertices->push_back(pos);
        colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
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

    mTerrainRoot->addChild(borderGeode);

    mCellBorderNodes[std::make_pair(x,y)] = borderGeode;
}

void World::unloadCell(int x, int y)
{
    CellGrid::iterator it = mCellBorderNodes.find(std::make_pair(x,y));

    if (it == mCellBorderNodes.end())
        return;

    osg::ref_ptr<osg::Node> borderNode = it->second;
    mTerrainRoot->removeChild(borderNode);

    mCellBorderNodes.erase(it);
}

World::~World()
{
    mResourceSystem->removeResourceManager(mChunkManager.get());
    mResourceSystem->removeResourceManager(mTextureManager.get());

    mParent->removeChild(mTerrainRoot);

    mCompositeMapCamera->removeChild(mCompositeMapRenderer);
    mCompositeMapCamera->getParent(0)->removeChild(mCompositeMapCamera);

    delete mStorage;
}

float World::getHeightAt(const osg::Vec3f &worldPos)
{
    return mStorage->getHeightAt(worldPos);
}

void World::updateTextureFiltering()
{
    mTextureManager->updateTextureFiltering();
}

void World::clearAssociatedCaches()
{
    mChunkManager->clearCache();
}

}
