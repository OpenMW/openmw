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
osg::ref_ptr<osg::Node> newNode = 
  new osg::ShapeDrawable(
    new osg::Sphere (
      osg::Vec3f(x * 8192 + 4000,y * 8192 + 4000,1000),100.0));

mTerrainRoot->addChild(newNode);

mCellBorderNodes[std::make_pair(x,y)] = newNode;

std::cout << "aaaaa" << std::endl;
}

void World::unloadCell(int x, int y)
{
CellGrid::iterator it = mCellBorderNodes.find(std::make_pair(x,y));

if (it == mCellBorderNodes.end())
    return;

osg::ref_ptr<osg::Node> borderNode = it->second;
mTerrainRoot->removeChild(borderNode);

mCellBorderNodes.erase(it);

std::cout << "bbbbb" << std::endl;
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
