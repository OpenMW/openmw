#include "terraingrid.hpp"

#include <memory>

#include <osg/Group>

#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"
#include "components/settings/settings.hpp"
#include "apps/openmw/mwrender/vismask.hpp"

namespace Terrain
{

class MyView : public View
{
public:
    osg::ref_ptr<osg::Node> mLoaded;

    virtual void reset() {}
};

TerrainGrid::TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage,unsigned int nodeMask, int preCompileMask, int borderMask)
    : Terrain::World(parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask, borderMask)
    , mNumSplits(4)
{
    resetSettings();
}

void TerrainGrid::resetSettings()
{
    mOQNSettings.enable = Settings::Manager::getBool("octree occlusion queries enable", "OcclusionQueries") && Settings::Manager::getBool("terrain OQN enable", "OcclusionQueries");
    mOQNSettings.debugDisplay = Settings::Manager::getBool("debug occlusion queries", "OcclusionQueries");
    mOQNSettings.querypixelcount = Settings::Manager::getInt("visibility threshold", "OcclusionQueries");
    mOQNSettings.queryframecount = Settings::Manager::getInt("queries frame count", "OcclusionQueries");
    mOQNSettings.maxCellSize = Settings::Manager::getFloat("max cell size", "OcclusionQueries");
    mOQNSettings.minOQNSize = Settings::Manager::getFloat("min node size", "OcclusionQueries");
    mOQNSettings.maxDrawablePerOQN = Settings::Manager::getInt("max node drawables", "OcclusionQueries");
    mOQNSettings.querymargin = Settings::Manager::getFloat("queries margin", "OcclusionQueries");
    mOQNSettings.maxBVHOQLevelCount = Settings::Manager::getInt("max BVH OQ level count", "OcclusionQueries");
    mOQNSettings.securepopdistance = Settings::Manager::getFloat("min pop in distance", "OcclusionQueries");
}
TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }
}

void TerrainGrid::cacheCell(View* view, int x, int y)
{
    osg::Vec2f center(x+0.5f, y+0.5f);

    static_cast<MyView*>(view)->mLoaded = buildTerrain(nullptr, 1.f, center);;
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
        osg::ref_ptr<osg::Node> node = mChunkManager->getChunk(chunkSize, chunkCenter, 0, 0);
        if (!node)
            return nullptr;

        node->setNodeMask(mTerrainNodeMask);

        if(mOQNSettings.enable){

            SceneUtil::StaticOcclusionQueryNode* qnode = new SceneUtil::StaticOcclusionQueryNode;
            qnode->setDebugDisplay(mOQNSettings.debugDisplay);
            qnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            qnode->setQueryFrameCount(mOQNSettings.queryframecount);
            qnode->setQueryMargin(mOQNSettings.querymargin);
            qnode->setDistancePreventingPopin(mOQNSettings.securepopdistance);

            qnode->addChild(node);
            qnode->getBound();

            qnode->getQueryGeometry()->setNodeMask(MWRender::Mask_OcclusionQuery);
            qnode->getDebugGeometry()->setNodeMask(MWRender::Mask_OcclusionQuery);
            node = qnode;
        }
        if (parent)
            parent->addChild(node);
        return node;
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
}

View *TerrainGrid::createView()
{
    return new MyView;
}

}
