#include "quadtreeworld.hpp"

#include <osgUtil/CullVisitor>
#include <osg/ShapeDrawable>
#include <osg/PolygonMode>

#include <limits>
#include <sstream>

#include <components/misc/constants.hpp>
#include <components/sceneutil/mwshadowtechnique.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "quadtreenode.hpp"
#include "storage.hpp"
#include "viewdata.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"
#include "terraindrawable.hpp"

namespace
{

    bool isPowerOfTwo(int x)
    {
        return ( (x > 0) && ((x & (x - 1)) == 0) );
    }

    int nextPowerOfTwo (int v)
    {
        if (isPowerOfTwo(v)) return v;
        int depth=0;
        while(v)
        {
            v >>= 1;
            depth++;
        }
        return 1 << depth;
    }

    int Log2( unsigned int n )
    {
        int targetlevel = 0;
        while (n >>= 1) ++targetlevel;
        return targetlevel;
    }

}

namespace Terrain
{

class DefaultLodCallback : public LodCallback
{
public:
    DefaultLodCallback(float factor, float minSize, float viewDistance, const osg::Vec4i& grid)
        : mFactor(factor)
        , mMinSize(minSize)
        , mViewDistance(viewDistance)
        , mActiveGrid(grid)
    {
    }

    ReturnValue isSufficientDetail(QuadTreeNode* node, float dist) override
    {
        const osg::Vec2f& center = node->getCenter();
        bool activeGrid = (center.x() > mActiveGrid.x() && center.y() > mActiveGrid.y() && center.x() < mActiveGrid.z() && center.y() < mActiveGrid.w());
        if (dist > mViewDistance && !activeGrid) // for Scene<->ObjectPaging sync the activegrid must remain loaded
            return StopTraversal;
        if (node->getSize()>1)
        {
            float halfSize = node->getSize()/2;
            osg::Vec4i nodeBounds (static_cast<int>(center.x() - halfSize), static_cast<int>(center.y() - halfSize), static_cast<int>(center.x() + halfSize), static_cast<int>(center.y() + halfSize));
            bool intersects = (std::max(nodeBounds.x(), mActiveGrid.x()) < std::min(nodeBounds.z(), mActiveGrid.z()) && std::max(nodeBounds.y(), mActiveGrid.y()) < std::min(nodeBounds.w(), mActiveGrid.w()));
            // to prevent making chunks who will cross the activegrid border
            if (intersects)
                return Deeper;
        }

        int nativeLodLevel = Log2(static_cast<unsigned int>(node->getSize()/mMinSize));
        int lodLevel = Log2(static_cast<unsigned int>(dist/(Constants::CellSizeInUnits*mMinSize*mFactor)));

        return nativeLodLevel <= lodLevel ? StopTraversalAndUse : Deeper;
    }

private:
    float mFactor;
    float mMinSize;
    float mViewDistance;
    osg::Vec4i mActiveGrid;
};

class RootNode : public QuadTreeNode
{
public:
    RootNode(float size, const osg::Vec2f& center)
        : QuadTreeNode(nullptr, Root, size, center)
        , mWorld(nullptr)
    {
    }

    void setWorld(QuadTreeWorld* world)
    {
        mWorld = world;
    }

    void accept(osg::NodeVisitor &nv) override
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
    QuadTreeBuilder(Terrain::Storage* storage, float minSize)
        : mStorage(storage)
        , mMinX(0.f), mMaxX(0.f), mMinY(0.f), mMaxY(0.f)
        , mMinSize(minSize)
    {
    }

    void build()
    {
        mStorage->getBounds(mMinX, mMaxX, mMinY, mMaxY);

        int origSizeX = static_cast<int>(mMaxX - mMinX);
        int origSizeY = static_cast<int>(mMaxY - mMinY);

        // Dividing a quad tree only works well for powers of two, so round up to the nearest one
        int size = nextPowerOfTwo(std::max(origSizeX, origSizeY));

        float centerX = (mMinX+mMaxX)/2.f + (size-origSizeX)/2.f;
        float centerY = (mMinY+mMaxY)/2.f + (size-origSizeY)/2.f;

        mRootNode = new RootNode(size, osg::Vec2f(centerX, centerY));
        addChildren(mRootNode);

        mRootNode->initNeighbours();
        float cellWorldSize = mStorage->getCellWorldSize();
        mRootNode->setInitialBound(osg::BoundingSphere(osg::BoundingBox(osg::Vec3(mMinX*cellWorldSize, mMinY*cellWorldSize, 0), osg::Vec3(mMaxX*cellWorldSize, mMaxY*cellWorldSize, 0))));
    }

    void addChildren(QuadTreeNode* parent)
    {
        float halfSize = parent->getSize()/2.f;
        osg::BoundingBox boundingBox;
        for (unsigned int i=0; i<4; ++i)
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
        float halfSize = size/2.f;
        osg::Vec2f center;
        switch (direction)
        {
        case SW:
            center = parent->getCenter() + osg::Vec2f(-halfSize,-halfSize);
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

        if (center.x() - halfSize > mMaxX
                || center.x() + halfSize < mMinX
                || center.y() - halfSize > mMaxY
                || center.y() + halfSize < mMinY )
            // Out of bounds of the actual terrain - this will happen because
            // we rounded the size up to the next power of two
        {
            // Still create and return an empty node so as to not break the assumption that each QuadTreeNode has either 4 or 0 children.
            return node;
        }

        // Do not add child nodes for default cells without data.
        // size = 1 means that the single shape covers the whole cell.
        if (node->getSize() == 1 && !mStorage->hasData(center.x()-0.5, center.y()-0.5))
            return node;

        if (node->getSize() <= mMinSize)
        {
            // We arrived at a leaf.
            // Since the tree is used for LOD level selection instead of culling, we do not need to load the actual height data here.
            constexpr float minZ = -std::numeric_limits<float>::max();
            constexpr float maxZ = std::numeric_limits<float>::max();
            float cellWorldSize = mStorage->getCellWorldSize();
            osg::BoundingBox boundingBox(osg::Vec3f((center.x()-halfSize)*cellWorldSize, (center.y()-halfSize)*cellWorldSize, minZ),
                                    osg::Vec3f((center.x()+halfSize)*cellWorldSize, (center.y()+halfSize)*cellWorldSize, maxZ));
            node->setBoundingBox(boundingBox);
            return node;
        }
        else
        {
            addChildren(node);
            return node;
        }
    }

    osg::ref_ptr<RootNode> getRootNode()
    {
        return mRootNode;
    }

private:
    Terrain::Storage* mStorage;

    float mMinX, mMaxX, mMinY, mMaxY;
    float mMinSize;

    osg::ref_ptr<RootNode> mRootNode;
};

QuadTreeWorld::QuadTreeWorld(osg::Group *parent, osg::Group *compileRoot, Resource::ResourceSystem *resourceSystem, Storage *storage, unsigned int nodeMask, unsigned int preCompileMask, unsigned int borderMask, int compMapResolution, float compMapLevel, float lodFactor, int vertexLodMod, float maxCompGeometrySize)
    : TerrainGrid(parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask, borderMask)
    , mViewDataMap(new ViewDataMap)
    , mQuadTreeBuilt(false)
    , mLodFactor(lodFactor)
    , mVertexLodMod(vertexLodMod)
    , mViewDistance(std::numeric_limits<float>::max())
    , mMinSize(1/8.f)
{
    mChunkManager->setCompositeMapSize(compMapResolution);
    mChunkManager->setCompositeMapLevel(compMapLevel);
    mChunkManager->setMaxCompositeGeometrySize(maxCompGeometrySize);
    mChunkManagers.push_back(mChunkManager.get());
}

QuadTreeWorld::QuadTreeWorld(osg::Group *parent, Storage *storage, unsigned int nodeMask, float lodFactor, float chunkSize)
    : TerrainGrid(parent, storage, nodeMask)
    , mViewDataMap(new ViewDataMap)
    , mQuadTreeBuilt(false)
    , mLodFactor(lodFactor)
    , mVertexLodMod(0)
    , mViewDistance(std::numeric_limits<float>::max())
    , mMinSize(chunkSize)
{
}

QuadTreeWorld::~QuadTreeWorld()
{
}

/// get the level of vertex detail to render this node at, expressed relative to the native resolution of the data set.
unsigned int getVertexLod(QuadTreeNode* node, int vertexLodMod)
{
    int lod = Log2(int(node->getSize()));
    if (vertexLodMod > 0)
    {
        lod = std::max(0, lod-vertexLodMod);
    }
    else if (vertexLodMod < 0)
    {
        float size = node->getSize();
        // Stop to simplify at this level since with size = 1 the node already covers the whole cell and has getCellVertices() vertices.
        while (size < 1)
        {
            size *= 2;
            vertexLodMod = std::min(0, vertexLodMod+1);
        }
        lod += std::abs(vertexLodMod);
    }
    return lod;
}

/// get the flags to use for stitching in the index buffer so that chunks of different LOD connect seamlessly
unsigned int getLodFlags(QuadTreeNode* node, int ourLod, int vertexLodMod, const ViewData* vd)
{
    unsigned int lodFlags = 0;
    for (unsigned int i=0; i<4; ++i)
    {
        QuadTreeNode* neighbour = node->getNeighbour(static_cast<Direction>(i));

        // If the neighbour isn't currently rendering itself,
        // go up until we find one. NOTE: We don't need to go down,
        // because in that case neighbour's detail would be higher than
        // our detail and the neighbour would handle stitching by itself.
        while (neighbour && !vd->contains(neighbour))
            neighbour = neighbour->getParent();
        int lod = 0;
        if (neighbour)
            lod = getVertexLod(neighbour, vertexLodMod);

        if (lod <= ourLod) // We only need to worry about neighbours less detailed than we are -
            lod = 0;         // neighbours with more detail will do the stitching themselves
        // Use 4 bits for each LOD delta
        if (lod > 0)
        {
            lodFlags |= static_cast<unsigned int>(lod - ourLod) << (4*i);
        }
    }
    return lodFlags;
}

void loadRenderingNode(ViewData::Entry& entry, ViewData* vd, int vertexLodMod, float cellWorldSize, const osg::Vec4i &gridbounds, const std::vector<QuadTreeWorld::ChunkManager*>& chunkManagers, bool compile)
{
    if (!vd->hasChanged() && entry.mRenderingNode)
        return;

    int ourLod = getVertexLod(entry.mNode, vertexLodMod);

    if (vd->hasChanged())
    {
        // have to recompute the lodFlags in case a neighbour has changed LOD.
        unsigned int lodFlags = getLodFlags(entry.mNode, ourLod, vertexLodMod, vd);
        if (lodFlags != entry.mLodFlags)
        {
            entry.mRenderingNode = nullptr;
            entry.mLodFlags = lodFlags;
        }
    }

    if (!entry.mRenderingNode)
    {
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> pat = new SceneUtil::PositionAttitudeTransform;
        pat->setPosition(osg::Vec3f(entry.mNode->getCenter().x()*cellWorldSize, entry.mNode->getCenter().y()*cellWorldSize, 0.f));

        const osg::Vec2f& center = entry.mNode->getCenter();
        bool activeGrid = (center.x() > gridbounds.x() && center.y() > gridbounds.y() && center.x() < gridbounds.z() && center.y() < gridbounds.w());

        for (QuadTreeWorld::ChunkManager* m : chunkManagers)
        {
            osg::ref_ptr<osg::Node> n = m->getChunk(entry.mNode->getSize(), entry.mNode->getCenter(), ourLod, entry.mLodFlags, activeGrid, vd->getViewPoint(), compile);
            if (n) pat->addChild(n);
        }
        entry.mRenderingNode = pat;
    }
}

void updateWaterCullingView(HeightCullCallback* callback, ViewData* vd, osgUtil::CullVisitor* cv, float cellworldsize, bool outofworld)
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
    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        osg::BoundingBox bb = static_cast<TerrainDrawable*>(entry.mRenderingNode->asGroup()->getChild(0))->getWaterBoundingBox();
        if (!bb.valid())
            continue;
        osg::Vec3f ofs (entry.mNode->getCenter().x()*cellworldsize, entry.mNode->getCenter().y()*cellworldsize, 0.f);
        bb._min += ofs; bb._max += ofs;
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
        static osg::ref_ptr<osg::StateSet> stateset = nullptr;
        if (!stateset)
        {
            stateset = new osg::StateSet;
            stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
            stateset->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), osg::StateAttribute::ON);
            osg::Material* m = new osg::Material;
            m->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,1,1));
            m->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,1));
            m->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,1));
            stateset->setAttributeAndModes(m, osg::StateAttribute::ON);
            stateset->setRenderBinDetails(100,"RenderBin");
        }
        drw->setStateSet(stateset);
        drw->accept(*cv);
    }
    callback->setLowZ(lowZ);
    cv->popCurrentMask();
}

void QuadTreeWorld::accept(osg::NodeVisitor &nv)
{
    bool isCullVisitor = nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR;
    if (!isCullVisitor && nv.getVisitorType() != osg::NodeVisitor::INTERSECTION_VISITOR)
    {
        if (nv.getName().find("AcceptedByComponentsTerrainQuadTreeWorld") != std::string::npos)
        {
            if (nv.getName().find("SceneUtil::MWShadowTechnique::ComputeLightSpaceBounds") != std::string::npos)
            {
                SceneUtil::MWShadowTechnique::ComputeLightSpaceBounds* clsb = static_cast<SceneUtil::MWShadowTechnique::ComputeLightSpaceBounds*>(&nv);
                clsb->apply(*this);
            }
            else
                nv.apply(*mRootNode);
        }
        return;
    }

    osg::Object * viewer = isCullVisitor ? static_cast<osgUtil::CullVisitor*>(&nv)->getCurrentCamera() : nullptr;
    bool needsUpdate = true;
    ViewData *vd = mViewDataMap->getViewData(viewer, nv.getViewPoint(), mActiveGrid, needsUpdate);

    if (needsUpdate)
    {
        vd->reset();
        DefaultLodCallback lodCallback(mLodFactor, mMinSize, mViewDistance, mActiveGrid);
        mRootNode->traverseNodes(vd, nv.getViewPoint(), &lodCallback);
    }

    const float cellWorldSize = mStorage->getCellWorldSize();

    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        loadRenderingNode(entry, vd, mVertexLodMod, cellWorldSize, mActiveGrid, mChunkManagers, false);
        entry.mRenderingNode->accept(nv);
    }

    if (mHeightCullCallback && isCullVisitor)
        updateWaterCullingView(mHeightCullCallback, vd, static_cast<osgUtil::CullVisitor*>(&nv), mStorage->getCellWorldSize(), !isGridEmpty());

    vd->markUnchanged();

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

    QuadTreeBuilder builder(mStorage, mMinSize);
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

    if (mRootNode)
        mRootNode->setNodeMask(enabled ? ~0 : 0);
}

View* QuadTreeWorld::createView()
{
    return mViewDataMap->createIndependentView();
}

void QuadTreeWorld::preload(View *view, const osg::Vec3f &viewPoint, const osg::Vec4i &grid, std::atomic<bool> &abort, std::atomic<int> &progress, int& progressTotal)
{
    ensureQuadTreeBuilt();

    ViewData* vd = static_cast<ViewData*>(view);
    vd->setViewPoint(viewPoint);
    vd->setActiveGrid(grid);
    DefaultLodCallback lodCallback(mLodFactor, mMinSize, mViewDistance, grid);
    mRootNode->traverseNodes(vd, viewPoint, &lodCallback);

    if (!progressTotal)
        for (unsigned int i=0; i<vd->getNumEntries(); ++i)
            progressTotal += vd->getEntry(i).mNode->getSize();

    const float cellWorldSize = mStorage->getCellWorldSize();
    for (unsigned int i=0; i<vd->getNumEntries() && !abort; ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        loadRenderingNode(entry, vd, mVertexLodMod, cellWorldSize, grid, mChunkManagers, true);
        progress += entry.mNode->getSize();
    }
    vd->markUnchanged();
}

bool QuadTreeWorld::storeView(const View* view, double referenceTime)
{
    return mViewDataMap->storeView(static_cast<const ViewData*>(view), referenceTime);
}

void QuadTreeWorld::reportStats(unsigned int frameNumber, osg::Stats *stats)
{
    if (mCompositeMapRenderer)
        stats->setAttribute(frameNumber, "Composite", mCompositeMapRenderer->getCompileSetSize());
}

void QuadTreeWorld::loadCell(int x, int y)
{
    // fallback behavior only for undefined cells (every other is already handled in quadtree)
    float dummy;
    if (mChunkManager && !mStorage->getMinMaxHeights(1, osg::Vec2f(x+0.5, y+0.5), dummy, dummy))
        TerrainGrid::loadCell(x,y);
    else
        World::loadCell(x,y);
}

void QuadTreeWorld::unloadCell(int x, int y)
{
    // fallback behavior only for undefined cells (every other is already handled in quadtree)
    float dummy;
    if (mChunkManager && !mStorage->getMinMaxHeights(1, osg::Vec2f(x+0.5, y+0.5), dummy, dummy))
        TerrainGrid::unloadCell(x,y);
    else
        World::unloadCell(x,y);
}

void QuadTreeWorld::addChunkManager(QuadTreeWorld::ChunkManager* m)
{
    mChunkManagers.push_back(m);
    mTerrainRoot->setNodeMask(mTerrainRoot->getNodeMask()|m->getNodeMask());
}

void QuadTreeWorld::rebuildViews()
{
    mViewDataMap->rebuildViews();
}

}
