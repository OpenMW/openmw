#include "terraingrid.hpp"

#include <memory>

#include <osg/Group>
#include <osg/ComputeBoundsVisitor>

#include <components/sceneutil/positionattitudetransform.hpp>
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"
#include "storage.hpp"
namespace Terrain
{

class MyView : public View
{
public:
    osg::ref_ptr<osg::Node> mLoaded;

    virtual void reset() {}
};

TerrainGrid::TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask, int borderMask)
    : Terrain::World(parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask, borderMask)
    , mNumSplits(4)
{
}

TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        TerrainGrid::unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }
}

void TerrainGrid::cacheCell(View* view, int x, int y)
{
    osg::Vec2f center(x+0.5f, y+0.5f);
    static_cast<MyView*>(view)->mLoaded =  buildTerrain(nullptr, 1.f, center);
}

osg::ref_ptr<osg::Node> TerrainGrid::buildTerrain (osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter)
{
    if (chunkSize * mNumSplits > 1.f)
    {
        // keep splitting
        osg::ref_ptr<osg::Group> group (new osg::Group);
        if (parent)
            parent->addChild(group);

        float newChunkSize = chunkSize/2.f;
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize/2.f, newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize/2.f, -newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize/2.f, newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize/2.f, -newChunkSize/2.f));
        return group;
    }
    else
    {
        osg::ref_ptr<osg::Node> node = mChunkManager->getChunk(chunkSize, chunkCenter, 0, 0, false, osg::Vec3f());
        if (!node)
            return nullptr;

        const float cellWorldSize = mStorage->getCellWorldSize();
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> pat = new SceneUtil::PositionAttitudeTransform;
        pat->setPosition(osg::Vec3f(chunkCenter.x()*cellWorldSize, chunkCenter.y()*cellWorldSize, 0.f));
        pat->addChild(node);
        if (parent)
            parent->addChild(pat);
        return pat;
    }
}

void TerrainGrid::loadCell(int x, int y)
{
    if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
        return; // already loaded

    osg::Vec2f center(x+0.5f, y+0.5f);
    osg::ref_ptr<osg::Node> terrainNode = buildTerrain(nullptr, 1.f, center);
    if (!terrainNode)
        return; // no terrain defined

    TerrainGrid::World::loadCell(x,y);

    mTerrainRoot->addChild(terrainNode);

    mGrid[std::make_pair(x,y)] = terrainNode;
    updateWaterCulling();
}

void TerrainGrid::unloadCell(int x, int y)
{
    CellBorder::CellGrid::iterator it = mGrid.find(std::make_pair(x,y));
    if (it == mGrid.end())
        return;

    Terrain::World::unloadCell(x,y);

    osg::ref_ptr<osg::Node> terrainNode = it->second;
    mTerrainRoot->removeChild(terrainNode);

    mGrid.erase(it);
    updateWaterCulling();
}

void TerrainGrid::updateWaterCulling()
{
    osg::ComputeBoundsVisitor computeBoundsVisitor;
    mTerrainRoot->accept(computeBoundsVisitor);
    float lowZ = computeBoundsVisitor.getBoundingBox()._min.z();
    mHeightCullCallback->setLowZ(lowZ);
}

View *TerrainGrid::createView()
{
    return new MyView;
}

}
