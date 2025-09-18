#include "quadtreeworld.hpp"

#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/ShapeDrawable>
#include <osgUtil/CullVisitor>

#include <limits>

#include <components/esm/util.hpp>
#include <components/loadinglistener/reporter.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/mathutil.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"
#include "heightcull.hpp"
#include "quadtreenode.hpp"
#include "storage.hpp"
#include "terraindrawable.hpp"
#include "viewdata.hpp"

namespace
{
    unsigned int Log2(unsigned int n)
    {
        unsigned int targetlevel = 0;
        while (n >>= 1)
            ++targetlevel;
        return targetlevel;
    }

}

namespace Terrain
{

    class DefaultLodCallback : public LodCallback
    {
    public:
        DefaultLodCallback(float factor, float minSize, float viewDistance, const osg::Vec4i& grid, int cellSizeInUnits,
            float distanceModifier = 0.f)
            : mFactor(factor)
            , mMinSize(minSize)
            , mViewDistance(viewDistance)
            , mActiveGrid(grid)
            , mDistanceModifier(distanceModifier)
            , mCellSizeInUnits(cellSizeInUnits)
        {
        }

        ReturnValue isSufficientDetail(QuadTreeNode* node, float dist) override
        {
            const osg::Vec2f& center = node->getCenter();
            bool activeGrid = (center.x() > mActiveGrid.x() && center.y() > mActiveGrid.y()
                && center.x() < mActiveGrid.z() && center.y() < mActiveGrid.w());

            if (node->getSize() > 1)
            {
                float halfSize = node->getSize() / 2;
                osg::Vec4i nodeBounds(static_cast<int>(center.x() - halfSize), static_cast<int>(center.y() - halfSize),
                    static_cast<int>(center.x() + halfSize), static_cast<int>(center.y() + halfSize));
                bool intersects = (std::max(nodeBounds.x(), mActiveGrid.x()) < std::min(nodeBounds.z(), mActiveGrid.z())
                    && std::max(nodeBounds.y(), mActiveGrid.y()) < std::min(nodeBounds.w(), mActiveGrid.w()));
                // to prevent making chunks who will cross the activegrid border
                if (intersects)
                    return Deeper;
            }
            dist = std::max(0.f, dist + mDistanceModifier);
            if (dist > mViewDistance && !activeGrid) // for Scene<->ObjectPaging sync the activegrid must remain loaded
                return StopTraversal;
            return getNativeLodLevel(node, mMinSize)
                    <= convertDistanceToLodLevel(dist, mMinSize, mFactor, mCellSizeInUnits)
                ? StopTraversalAndUse
                : Deeper;
        }
        static unsigned int getNativeLodLevel(const QuadTreeNode* node, float minSize)
        {
            return Log2(static_cast<unsigned int>(node->getSize() / minSize));
        }
        static unsigned int convertDistanceToLodLevel(float dist, float minSize, float factor, int cellSize)
        {
            return Log2(static_cast<unsigned int>(dist / (cellSize * minSize * factor)));
        }

    private:
        float mFactor;
        float mMinSize;
        float mViewDistance;
        osg::Vec4i mActiveGrid;
        float mDistanceModifier;
        int mCellSizeInUnits;
    };

    class RootNode : public QuadTreeNode
    {
    public:
        RootNode(float size, const osg::Vec2f& center)
            : QuadTreeNode(nullptr, Root, size, center)
            , mWorld(nullptr)
        {
        }

        void setWorld(QuadTreeWorld* world) { mWorld = world; }

        void accept(osg::NodeVisitor& nv) override
        {
            if (!nv.validNodeMask(*this))
                return;
            nv.pushOntoNodePath(this);
            mWorld->accept(nv);
            nv.popFromNodePath();
        }

    private:
        QuadTreeWorld* mWorld;
    };

    class QuadTreeBuilder
    {
    public:
        QuadTreeBuilder(Terrain::Storage* storage, float minSize, ESM::RefId worldspace)
            : mStorage(storage)
            , mMinX(0.f)
            , mMaxX(0.f)
            , mMinY(0.f)
            , mMaxY(0.f)
            , mMinSize(minSize)
            , mWorldspace(worldspace)
        {
        }

        void build()
        {
            mStorage->getBounds(mMinX, mMaxX, mMinY, mMaxY, mWorldspace);

            int origSizeX = static_cast<int>(mMaxX - mMinX);
            int origSizeY = static_cast<int>(mMaxY - mMinY);

            // Dividing a quad tree only works well for powers of two, so round up to the nearest one
            int size = Misc::nextPowerOfTwo(std::max(origSizeX, origSizeY));

            float centerX = (mMinX + mMaxX) / 2.f + (size - origSizeX) / 2.f;
            float centerY = (mMinY + mMaxY) / 2.f + (size - origSizeY) / 2.f;

            mRootNode = new RootNode(static_cast<float>(size), osg::Vec2f(centerX, centerY));
            addChildren(mRootNode);

            mRootNode->initNeighbours();
            float cellWorldSize = mStorage->getCellWorldSize(mWorldspace);
            mRootNode->setInitialBound(
                osg::BoundingSphere(osg::BoundingBox(osg::Vec3(mMinX * cellWorldSize, mMinY * cellWorldSize, 0),
                    osg::Vec3(mMaxX * cellWorldSize, mMaxY * cellWorldSize, 0))));
        }

        void addChildren(QuadTreeNode* parent)
        {
            float halfSize = parent->getSize() / 2.f;
            osg::BoundingBox boundingBox;
            for (unsigned int i = 0; i < 4; ++i)
            {
                osg::ref_ptr<QuadTreeNode> child = addChild(parent, static_cast<ChildDirection>(i), halfSize);
                if (child)
                {
                    boundingBox.expandBy(child->getBoundingBox());
                    parent->addChildNode(child);
                }
            }

            if (!boundingBox.valid())
                parent->removeChildren(0, 4);
            else
                parent->setBoundingBox(boundingBox);
        }

        osg::ref_ptr<QuadTreeNode> addChild(QuadTreeNode* parent, ChildDirection direction, float size)
        {
            float halfSize = size / 2.f;
            osg::Vec2f center;
            switch (direction)
            {
                case SW:
                    center = parent->getCenter() + osg::Vec2f(-halfSize, -halfSize);
                    break;
                case SE:
                    center = parent->getCenter() + osg::Vec2f(halfSize, -halfSize);
                    break;
                case NW:
                    center = parent->getCenter() + osg::Vec2f(-halfSize, halfSize);
                    break;
                case NE:
                    center = parent->getCenter() + osg::Vec2f(halfSize, halfSize);
                    break;
                default:
                    break;
            }

            osg::ref_ptr<QuadTreeNode> node = new QuadTreeNode(parent, direction, size, center);

            if (center.x() - halfSize > mMaxX || center.x() + halfSize < mMinX || center.y() - halfSize > mMaxY
                || center.y() + halfSize < mMinY)
            // Out of bounds of the actual terrain - this will happen because
            // we rounded the size up to the next power of two
            {
                // Still create and return an empty node so as to not break the assumption that each QuadTreeNode has
                // either 4 or 0 children.
                return node;
            }

            // Do not add child nodes for default cells without data.
            // size = 1 means that the single shape covers the whole cell.
            if (node->getSize() == 1
                && !mStorage->hasData(ESM::ExteriorCellLocation(
                    static_cast<int>(center.x() - 0.5f), static_cast<int>(center.y() - 0.5f), mWorldspace)))
                return node;

            if (node->getSize() <= mMinSize)
            {
                // We arrived at a leaf.
                // Since the tree is used for LOD level selection instead of culling, we do not need to load the actual
                // height data here.
                constexpr float minZ = -std::numeric_limits<float>::max();
                constexpr float maxZ = std::numeric_limits<float>::max();
                float cellWorldSize = mStorage->getCellWorldSize(mWorldspace);
                osg::BoundingBox boundingBox(
                    osg::Vec3f((center.x() - halfSize) * cellWorldSize, (center.y() - halfSize) * cellWorldSize, minZ),
                    osg::Vec3f((center.x() + halfSize) * cellWorldSize, (center.y() + halfSize) * cellWorldSize, maxZ));
                node->setBoundingBox(boundingBox);
                return node;
            }
            else
            {
                addChildren(node);
                return node;
            }
        }

        osg::ref_ptr<RootNode> getRootNode() { return mRootNode; }

    private:
        Terrain::Storage* mStorage;

        float mMinX, mMaxX, mMinY, mMaxY;
        float mMinSize;

        osg::ref_ptr<RootNode> mRootNode;
        ESM::RefId mWorldspace;
    };

    class DebugChunkManager : public QuadTreeWorld::ChunkManager
    {
    public:
        DebugChunkManager(
            Resource::SceneManager* sceneManager, Storage* storage, unsigned int nodeMask, ESM::RefId worldspace)
            : QuadTreeWorld::ChunkManager(worldspace)
            , mSceneManager(sceneManager)
            , mStorage(storage)
            , mNodeMask(nodeMask)
        {
        }
        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& chunkCenter, unsigned char /*lod*/,
            unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile)
        {
            osg::Vec3f center = { chunkCenter.x(), chunkCenter.y(), 0 };
            auto chunkBorder = CellBorder::createBorderGeometry(center.x() - size / 2.f, center.y() - size / 2.f, size,
                mStorage, mSceneManager, mNodeMask, mWorldspace, 5.f, { 1, 0, 0, 0 });
            osg::ref_ptr<SceneUtil::PositionAttitudeTransform> pat = new SceneUtil::PositionAttitudeTransform;
            pat->setPosition(-center * static_cast<float>(ESM::getCellSize(mWorldspace)));
            pat->addChild(chunkBorder);
            return pat;
        }
        unsigned int getNodeMask() { return mNodeMask; }

    private:
        Resource::SceneManager* mSceneManager;
        Storage* mStorage;
        unsigned int mNodeMask;
    };

    QuadTreeWorld::QuadTreeWorld(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem,
        Storage* storage, unsigned int nodeMask, unsigned int preCompileMask, unsigned int borderMask,
        int compMapResolution, float compMapLevel, float lodFactor, int vertexLodMod, float maxCompGeometrySize,
        bool debugChunks, ESM::RefId worldspace, double expiryDelay)
        : TerrainGrid(parent, compileRoot, resourceSystem, storage, nodeMask, worldspace, expiryDelay, preCompileMask,
              borderMask)
        , mViewDataMap(new ViewDataMap)
        , mQuadTreeBuilt(false)
        , mLodFactor(lodFactor)
        , mVertexLodMod(vertexLodMod)
        , mViewDistance(std::numeric_limits<float>::max())
        , mMinSize(ESM::isEsm4Ext(worldspace) ? 1 / 2.f : 1 / 8.f)
        , mDebugTerrainChunks(debugChunks)
    {
        mChunkManager->setCompositeMapSize(compMapResolution);
        mChunkManager->setCompositeMapLevel(
            ESM::isEsm4Ext(worldspace) ? compMapLevel * 2 /*because cells are twice smaller*/ : compMapLevel);
        mChunkManager->setMaxCompositeGeometrySize(maxCompGeometrySize);
        mChunkManagers.push_back(mChunkManager.get());

        if (mDebugTerrainChunks)
        {
            mDebugChunkManager = std::make_unique<DebugChunkManager>(
                mResourceSystem->getSceneManager(), mStorage, borderMask, mWorldspace);
            addChunkManager(mDebugChunkManager.get());
        }
    }

    QuadTreeWorld::~QuadTreeWorld() {}

    /// get the level of vertex detail to render this node at, expressed relative to the native resolution of the vertex
    /// data set, NOT relative to mMinSize as is the case with node LODs.
    unsigned int getVertexLod(QuadTreeNode* node, int vertexLodMod)
    {
        unsigned int vertexLod = DefaultLodCallback::getNativeLodLevel(node, 1);
        if (vertexLodMod > 0)
        {
            vertexLod = static_cast<unsigned int>(std::max(0, static_cast<int>(vertexLod) - vertexLodMod));
        }
        else if (vertexLodMod < 0)
        {
            float size = node->getSize();
            // Stop to simplify at this level since with size = 1 the node already covers the whole cell and has
            // getCellVertices() vertices.
            while (size < 1)
            {
                size *= 2;
                vertexLodMod = std::min(0, vertexLodMod + 1);
            }
            vertexLod += std::abs(vertexLodMod);
        }
        return vertexLod;
    }

    /// get the flags to use for stitching in the index buffer so that chunks of different LOD connect seamlessly
    unsigned int getLodFlags(QuadTreeNode* node, unsigned int ourVertexLod, int vertexLodMod, const ViewData* vd)
    {
        unsigned int lodFlags = 0;
        for (unsigned int i = 0; i < 4; ++i)
        {
            QuadTreeNode* neighbour = node->getNeighbour(static_cast<Direction>(i));

            // If the neighbour isn't currently rendering itself,
            // go up until we find one. NOTE: We don't need to go down,
            // because in that case neighbour's detail would be higher than
            // our detail and the neighbour would handle stitching by itself.
            while (neighbour && !vd->contains(neighbour))
                neighbour = neighbour->getParent();
            unsigned int lod = 0;
            if (neighbour)
                lod = getVertexLod(neighbour, vertexLodMod);

            if (lod <= ourVertexLod) // We only need to worry about neighbours less detailed than we are -
                lod = 0; // neighbours with more detail will do the stitching themselves
            // Use 4 bits for each LOD delta
            if (lod > 0)
            {
                lodFlags |= (lod - ourVertexLod) << (4 * i);
            }
        }
        // Use the remaining bits for our vertex LOD
        lodFlags |= (ourVertexLod << (4 * 4));
        return lodFlags;
    }

    void QuadTreeWorld::loadRenderingNode(
        ViewDataEntry& entry, ViewData* vd, float cellWorldSize, const osg::Vec4i& gridbounds, bool compile)
    {
        if (!vd->hasChanged() && entry.mRenderingNode)
            return;

        if (vd->hasChanged())
        {
            vd->buildNodeIndex();

            unsigned int ourVertexLod = getVertexLod(entry.mNode, mVertexLodMod);
            // have to recompute the lodFlags in case a neighbour has changed LOD.
            unsigned int lodFlags = getLodFlags(entry.mNode, ourVertexLod, mVertexLodMod, vd);
            if (lodFlags != entry.mLodFlags)
            {
                entry.mRenderingNode = nullptr;
                entry.mLodFlags = lodFlags;
            }
        }

        if (!entry.mRenderingNode)
        {
            osg::ref_ptr<SceneUtil::PositionAttitudeTransform> pat = new SceneUtil::PositionAttitudeTransform;
            pat->setPosition(osg::Vec3f(
                entry.mNode->getCenter().x() * cellWorldSize, entry.mNode->getCenter().y() * cellWorldSize, 0.f));

            const osg::Vec2f& center = entry.mNode->getCenter();
            bool activeGrid = (center.x() > gridbounds.x() && center.y() > gridbounds.y() && center.x() < gridbounds.z()
                && center.y() < gridbounds.w());

            for (QuadTreeWorld::ChunkManager* m : mChunkManagers)
            {
                osg::ref_ptr<osg::Node> n = m->getChunk(entry.mNode->getSize(), entry.mNode->getCenter(),
                    static_cast<unsigned char>(DefaultLodCallback::getNativeLodLevel(entry.mNode, mMinSize)),
                    entry.mLodFlags, activeGrid, vd->getViewPoint(), compile);
                if (n)
                    pat->addChild(n);
            }
            entry.mRenderingNode = pat;
        }
    }

    namespace
    {
        osg::ref_ptr<osg::StateSet> makeStateSet()
        {
            osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
            stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
            stateSet->setAttributeAndModes(
                new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE),
                osg::StateAttribute::ON);
            osg::ref_ptr<osg::Material> material = new osg::Material;
            material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 1, 1));
            material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 1));
            material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 1));
            stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
            stateSet->setRenderBinDetails(100, "RenderBin");
            return stateSet;
        }

        void updateWaterCullingView(
            HeightCullCallback* callback, ViewData* vd, osgUtil::CullVisitor* cv, float cellworldsize, bool outofworld)
        {
            if (!(cv->getTraversalMask() & callback->getCullMask()))
                return;
            float lowZ = std::numeric_limits<float>::max();
            float highZ = callback->getHighZ();
            if (cv->getEyePoint().z() <= highZ || outofworld)
            {
                callback->setLowZ(-std::numeric_limits<float>::max());
                return;
            }
            cv->pushCurrentMask();
            static bool debug = getenv("OPENMW_WATER_CULLING_DEBUG") != nullptr;
            for (unsigned int i = 0; i < vd->getNumEntries(); ++i)
            {
                ViewDataEntry& entry = vd->getEntry(i);
                osg::BoundingBox bb = static_cast<TerrainDrawable*>(entry.mRenderingNode->asGroup()->getChild(0))
                                          ->getWaterBoundingBox();
                if (!bb.valid())
                    continue;
                osg::Vec3f ofs(
                    entry.mNode->getCenter().x() * cellworldsize, entry.mNode->getCenter().y() * cellworldsize, 0.f);
                bb._min += ofs;
                bb._max += ofs;
                bb._min.z() = highZ;
                bb._max.z() = highZ;
                if (cv->isCulled(bb))
                    continue;
                lowZ = bb._min.z();

                if (!debug)
                    break;
                osg::Box* b = new osg::Box;
                b->set(bb.center(), bb._max - bb.center());
                osg::ShapeDrawable* drw = new osg::ShapeDrawable(b);
                static const osg::ref_ptr<osg::StateSet> stateset = makeStateSet();
                drw->setStateSet(stateset);
                drw->accept(*cv);
            }
            callback->setLowZ(lowZ);
            cv->popCurrentMask();
        }
    }

    void QuadTreeWorld::accept(osg::NodeVisitor& nv)
    {
        bool isCullVisitor = nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR;
        if (!isCullVisitor && nv.getVisitorType() != osg::NodeVisitor::INTERSECTION_VISITOR)
            return;

        osg::Object* viewer = isCullVisitor ? static_cast<osgUtil::CullVisitor*>(&nv)->getCurrentCamera() : nullptr;
        bool needsUpdate = true;
        osg::Vec3f viewPoint = viewer ? nv.getViewPoint() : nv.getEyePoint();
        ViewData* vd = mViewDataMap->getViewData(viewer, viewPoint, mActiveGrid, needsUpdate);
        if (needsUpdate)
        {
            vd->reset();
            DefaultLodCallback lodCallback(
                mLodFactor, mMinSize, mViewDistance, mActiveGrid, ESM::getCellSize(mWorldspace));
            mRootNode->traverseNodes(vd, viewPoint, &lodCallback);
        }

        const float cellWorldSize = static_cast<float>(ESM::getCellSize(mWorldspace));

        for (unsigned int i = 0; i < vd->getNumEntries(); ++i)
        {
            ViewDataEntry& entry = vd->getEntry(i);
            loadRenderingNode(entry, vd, cellWorldSize, mActiveGrid, false);
            entry.mRenderingNode->accept(nv);
        }

        if (mHeightCullCallback && isCullVisitor)
            updateWaterCullingView(mHeightCullCallback, vd, static_cast<osgUtil::CullVisitor*>(&nv),
                mStorage->getCellWorldSize(mWorldspace), !isGridEmpty());

        vd->resetChanged();

        double referenceTime = nv.getFrameStamp() ? nv.getFrameStamp()->getReferenceTime() : 0.0;
        if (referenceTime != 0.0)
        {
            vd->setLastUsageTimeStamp(referenceTime);
            mViewDataMap->clearUnusedViews(referenceTime);
        }
    }

    void QuadTreeWorld::ensureQuadTreeBuilt()
    {
        std::lock_guard<std::mutex> lock(mQuadTreeMutex);
        if (mQuadTreeBuilt)
            return;

        QuadTreeBuilder builder(mStorage, mMinSize, mWorldspace);
        builder.build();

        mRootNode = builder.getRootNode();
        mRootNode->setWorld(this);
        mQuadTreeBuilt = true;
    }

    void QuadTreeWorld::enable(bool enabled)
    {
        if (enabled)
        {
            ensureQuadTreeBuilt();

            if (!mRootNode->getNumParents())
                mTerrainRoot->addChild(mRootNode);
        }
        else if (mRootNode)
            mTerrainRoot->removeChild(mRootNode);
    }

    View* QuadTreeWorld::createView()
    {
        return mViewDataMap->createIndependentView();
    }

    void QuadTreeWorld::preload(View* view, const osg::Vec3f& viewPoint, const osg::Vec4i& grid,
        std::atomic<bool>& abort, Loading::Reporter& reporter)
    {
        ensureQuadTreeBuilt();
        const float cellWorldSize = mStorage->getCellWorldSize(mWorldspace);

        ViewData* vd = static_cast<ViewData*>(view);
        vd->setViewPoint(viewPoint);
        vd->setActiveGrid(grid);

        DefaultLodCallback lodCallback(mLodFactor, mMinSize, mViewDistance, grid, static_cast<int>(cellWorldSize));
        mRootNode->traverseNodes(vd, viewPoint, &lodCallback);

        reporter.addTotal(vd->getNumEntries());

        for (unsigned int i = 0, n = vd->getNumEntries(); i < n && !abort; ++i)
        {
            ViewDataEntry& entry = vd->getEntry(i);
            loadRenderingNode(entry, vd, cellWorldSize, grid, true);
            reporter.addProgress(1);
        }
    }

    void QuadTreeWorld::reportStats(unsigned int frameNumber, osg::Stats* stats)
    {
        if (mCompositeMapRenderer)
            stats->setAttribute(
                frameNumber, "Composite", static_cast<double>(mCompositeMapRenderer->getCompileSetSize()));
    }

    void QuadTreeWorld::loadCell(int x, int y)
    {
        // fallback behavior only for undefined cells (every other is already handled in quadtree)
        float dummy;
        if (mChunkManager && !mStorage->getMinMaxHeights(1, osg::Vec2f(x + 0.5f, y + 0.5f), mWorldspace, dummy, dummy))
            TerrainGrid::loadCell(x, y);
        else
            World::loadCell(x, y);
    }

    void QuadTreeWorld::unloadCell(int x, int y)
    {
        // fallback behavior only for undefined cells (every other is already handled in quadtree)
        float dummy;
        if (mChunkManager && !mStorage->getMinMaxHeights(1, osg::Vec2f(x + 0.5f, y + 0.5f), mWorldspace, dummy, dummy))
            TerrainGrid::unloadCell(x, y);
        else
            World::unloadCell(x, y);
    }

    void QuadTreeWorld::addChunkManager(QuadTreeWorld::ChunkManager* m)
    {
        mChunkManagers.push_back(m);
        mTerrainRoot->setNodeMask(mTerrainRoot->getNodeMask() | m->getNodeMask());
        if (m->getViewDistance())
            m->setMaxLodLevel(
                DefaultLodCallback::convertDistanceToLodLevel(m->getViewDistance() + mViewDataMap->getReuseDistance(),
                    mMinSize, mLodFactor, ESM::getCellSize(mWorldspace)));
    }

    void QuadTreeWorld::rebuildViews()
    {
        mViewDataMap->rebuildViews();
    }

    void QuadTreeWorld::setViewDistance(float viewDistance)
    {
        if (mViewDistance == viewDistance)
            return;
        mViewDistance = viewDistance;
        mViewDataMap->rebuildViews();
    }

}
