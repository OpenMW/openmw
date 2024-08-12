#include "terraingrid.hpp"

#include <memory>

#include <osg/ComputeBoundsVisitor>
#include <osg/Group>

#include "chunkmanager.hpp"
#include "heightcull.hpp"
#include "storage.hpp"
#include "view.hpp"
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/esm/util.hpp>

namespace Terrain
{

    class MyView : public View
    {
    public:
        osg::ref_ptr<osg::Node> mLoaded;

        void reset() override {}
    };

    TerrainGrid::TerrainGrid(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem,
        Storage* storage, unsigned int nodeMask, ESM::RefId worldspace, double expiryDelay, unsigned int preCompileMask,
        unsigned int borderMask)
        : Terrain::World(
            parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask, borderMask, worldspace, expiryDelay)
        , mNumSplits(4)
    {
    }

    TerrainGrid::TerrainGrid(osg::Group* parent, Storage* storage, ESM::RefId worldspace, unsigned int nodeMask)
        : Terrain::World(parent, storage, nodeMask, worldspace)
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
        osg::Vec2f center(x + 0.5f, y + 0.5f);
        static_cast<MyView*>(view)->mLoaded = buildTerrain(nullptr, 1.f, center);
    }

    // I think this should be where we decide to split the land into 4 quads.
    // Alternatively, we can do it in loadCell() and call a different kind of buildTerrain().
    //
    // Need to do some experiments to see if the existing code will produe the correct quads or
    // special code needs to be added (depens on column/row start and ends).  But I think we
    // still need to pass more info to getChunk() because we need to know which quadrant for
    // the textures?
    osg::ref_ptr<osg::Node> TerrainGrid::buildTerrain(
        osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter, int quad)
    {
        if (ESM::isEsm4Ext(mWorldspace) && chunkSize == 1.f && mNumSplits == 4) // WARN: hard coded values for ESM4
        {
            osg::ref_ptr<osg::Group> group(new osg::Group);
            if (parent)
                parent->addChild(group); // should never happen

            float newChunkSize = chunkSize / 2.f;
            {
                buildTerrain(group, // top right
                        newChunkSize, chunkCenter + osg::Vec2f(newChunkSize / 2.f, newChunkSize / 2.f), 3);
                buildTerrain(group, // top left
                        newChunkSize, chunkCenter + osg::Vec2f(newChunkSize / 2.f, -newChunkSize / 2.f), 1);
                buildTerrain(group, // bottom right
                        newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize / 2.f, newChunkSize / 2.f), 2);
                buildTerrain(group, // bottom left
                        newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize / 2.f, -newChunkSize / 2.f), 0);
            }
            return group;
        }
        else if (!ESM::isEsm4Ext(mWorldspace) && chunkSize * mNumSplits > 1.f) // FIXME: needs better logic
        {
            // keep splitting
            osg::ref_ptr<osg::Group> group(new osg::Group);
            if (parent)
                parent->addChild(group);

            float newChunkSize = chunkSize / 2.f;
            buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize / 2.f, newChunkSize / 2.f));
            buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize / 2.f, -newChunkSize / 2.f));
            buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize / 2.f, newChunkSize / 2.f));
            buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize / 2.f, -newChunkSize / 2.f));
            return group;
        }
        else
        {
            // FIXME: not sure which is worse, this mess or adding a parameter to Terrain::QuadTreeWorld::getChunk()
            osg::ref_ptr<osg::Node> node = ESM::isEsm4Ext(mWorldspace)
                ? mChunkManager->getChunk(chunkSize, chunkCenter, 0, 0, false, osg::Vec3f(), true, quad)
                : mChunkManager->getChunk(chunkSize, chunkCenter, 0, 0, false, osg::Vec3f(), true);
            if (!node)
                return nullptr;

            const float cellWorldSize = mStorage->getCellWorldSize(mWorldspace);
            osg::ref_ptr<SceneUtil::PositionAttitudeTransform> pat = new SceneUtil::PositionAttitudeTransform;
            pat->setPosition(osg::Vec3f(chunkCenter.x() * cellWorldSize, chunkCenter.y() * cellWorldSize, 0.f));
            pat->addChild(node);
            if (parent)
                parent->addChild(pat);
            return pat;
        }
    }

    // Use ESM::isEsm4Ext(World::getWorldspace())
    // or just ESM::isEsm4Ext(mWorldspace) since mWorldspace is declared as protected.
    void TerrainGrid::loadCell(int x, int y)
    {
        if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
            return; // already loaded

        osg::Vec2f center(x + 0.5f, y + 0.5f);
        osg::ref_ptr<osg::Node> terrainNode = buildTerrain(nullptr, 1.f, center);
        if (!terrainNode)
            return; // no terrain defined

        TerrainGrid::World::loadCell(x, y);

        mTerrainRoot->addChild(terrainNode);

        mGrid[std::make_pair(x, y)] = terrainNode;
        updateWaterCulling();
    }

    void TerrainGrid::unloadCell(int x, int y)
    {
        CellBorder::CellGrid::iterator it = mGrid.find(std::make_pair(x, y));
        if (it == mGrid.end())
            return;

        Terrain::World::unloadCell(x, y);

        osg::ref_ptr<osg::Node> terrainNode = it->second;
        mTerrainRoot->removeChild(terrainNode);

        mGrid.erase(it);
        updateWaterCulling();
    }

    void TerrainGrid::updateWaterCulling()
    {
        if (!mHeightCullCallback)
            return;

        osg::ComputeBoundsVisitor computeBoundsVisitor;
        mTerrainRoot->accept(computeBoundsVisitor);
        float lowZ = computeBoundsVisitor.getBoundingBox()._min.z();
        mHeightCullCallback->setLowZ(lowZ);
    }

    View* TerrainGrid::createView()
    {
        return new MyView;
    }

}
