#include "quadtreeworld.hpp"

#include <osgUtil/CullVisitor>

#include <sstream>

#include <components/misc/constants.hpp>
#include <components/sceneutil/mwshadowtechnique.hpp>
#include <components/sceneutil/vismask.hpp>

#include "quadtreenode.hpp"
#include "storage.hpp"
#include "viewdata.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

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
    DefaultLodCallback(float factor, float minSize)
        : mFactor(factor)
        , mMinSize(minSize)
    {
    }

    virtual bool isSufficientDetail(QuadTreeNode* node, float dist)
    {
        int nativeLodLevel = Log2(static_cast<unsigned int>(node->getSize()/mMinSize));
        int lodLevel = Log2(static_cast<unsigned int>(dist/(Constants::CellSizeInUnits*mMinSize*mFactor)));

        return nativeLodLevel <= lodLevel;
    }

private:
    float mFactor;
    float mMinSize;
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

    virtual void accept(osg::NodeVisitor &nv)
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
            float minZ = -std::numeric_limits<float>::max();
            float maxZ = std::numeric_limits<float>::max();
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
    osg::ref_ptr<LodCallback> mLodCallback;
};

QuadTreeWorld::QuadTreeWorld(osg::Group *parent, osg::Group *compileRoot, Resource::ResourceSystem *resourceSystem, Storage *storage, int compMapResolution, float compMapLevel, float lodFactor, int vertexLodMod, float maxCompGeometrySize)
    : TerrainGrid(parent, compileRoot, resourceSystem, storage)
    , mViewDataMap(new ViewDataMap)
    , mQuadTreeBuilt(false)
    , mLodFactor(lodFactor)
    , mVertexLodMod(vertexLodMod)
    , mViewDistance(std::numeric_limits<float>::max())
{
    mChunkManager->setCompositeMapSize(compMapResolution);
    mChunkManager->setCompositeMapLevel(compMapLevel);
    mChunkManager->setMaxCompositeGeometrySize(maxCompGeometrySize);
}

QuadTreeWorld::~QuadTreeWorld()
{
    mViewDataMap->clear();
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
unsigned int getLodFlags(QuadTreeNode* node, int ourLod, int vertexLodMod, ViewData* vd)
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

void loadRenderingNode(ViewData::Entry& entry, ViewData* vd, int vertexLodMod, ChunkManager* chunkManager)
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
        entry.mRenderingNode = chunkManager->getChunk(entry.mNode->getSize(), entry.mNode->getCenter(), ourLod, entry.mLodFlags);
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

    bool needsUpdate = true;
    ViewData* vd = nullptr;
    if (isCullVisitor)
        vd = mViewDataMap->getViewData(static_cast<osgUtil::CullVisitor*>(&nv)->getCurrentCamera(), nv.getViewPoint(), needsUpdate);
    else
    {
        static ViewData sIntersectionViewData;
        vd = &sIntersectionViewData;
    }

    if (needsUpdate)
    {
        vd->reset();
        if (isCullVisitor)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);

            osg::UserDataContainer* udc = cv->getCurrentCamera()->getUserDataContainer();
            if (udc && udc->getNumDescriptions() >= 2 && udc->getDescriptions()[0] == "NoTerrainLod")
            {
                std::istringstream stream(udc->getDescriptions()[1]);
                int x,y;
                stream >> x;
                stream >> y;
                mRootNode->traverseTo(vd, 1, osg::Vec2f(x+0.5,y+0.5));
            }
            else
                mRootNode->traverseNodes(vd, cv->getViewPoint(), mLodCallback, mViewDistance);
        }
        else
        {
            osgUtil::IntersectionVisitor* iv = static_cast<osgUtil::IntersectionVisitor*>(&nv);
            osgUtil::LineSegmentIntersector* lineIntersector = dynamic_cast<osgUtil::LineSegmentIntersector*>(iv->getIntersector());
            if (!lineIntersector)
                throw std::runtime_error("Cannot update QuadTreeWorld: node visitor is not LineSegmentIntersector");

            if (lineIntersector->getCoordinateFrame() == osgUtil::Intersector::CoordinateFrame::MODEL && iv->getModelMatrix() == 0)
            {
                TerrainLineIntersector terrainIntersector(lineIntersector);
                mRootNode->intersect(vd, terrainIntersector);
            }
            else
            {
                osg::Matrix matrix(lineIntersector->getTransformation(*iv, lineIntersector->getCoordinateFrame()));
                TerrainLineIntersector terrainIntersector(lineIntersector, matrix);
                mRootNode->intersect(vd, terrainIntersector);
            }
        }
    }

    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);

        loadRenderingNode(entry, vd, mVertexLodMod, mChunkManager.get());

        entry.mRenderingNode->accept(nv);
    }

    if (!isCullVisitor)
        vd->clear(); // we can't reuse intersection views in the next frame because they only contain what is touched by the intersection ray.

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
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mQuadTreeMutex);
    if (mQuadTreeBuilt)
        return;

    const float minSize = 1/8.f;
    mLodCallback = new DefaultLodCallback(mLodFactor, minSize);
    QuadTreeBuilder builder(mStorage, minSize);
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
        mRootNode->setNodeMask(enabled ? SceneUtil::Mask_Default : SceneUtil::Mask_Disabled);
}

void QuadTreeWorld::cacheCell(View *view, int x, int y)
{
    ensureQuadTreeBuilt();
    ViewData* vd = static_cast<ViewData*>(view);
    mRootNode->traverseTo(vd, 1, osg::Vec2f(x+0.5f,y+0.5f));

    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        loadRenderingNode(entry, vd, mVertexLodMod, mChunkManager.get());
    }
}

View* QuadTreeWorld::createView()
{
    return new ViewData;
}

void QuadTreeWorld::preload(View *view, const osg::Vec3f &viewPoint, std::atomic<bool> &abort)
{
    ensureQuadTreeBuilt();

    ViewData* vd = static_cast<ViewData*>(view);
    vd->setViewPoint(viewPoint);
    mRootNode->traverseNodes(vd, viewPoint, mLodCallback, mViewDistance);

    for (unsigned int i=0; i<vd->getNumEntries() && !abort; ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        loadRenderingNode(entry, vd, mVertexLodMod, mChunkManager.get());
    }
    vd->markUnchanged();
}

void QuadTreeWorld::storeView(const View* view, double referenceTime)
{
    osg::ref_ptr<osg::Object> dummy = new osg::DummyObject;
    const ViewData* vd = static_cast<const ViewData*>(view);
    bool needsUpdate = false;
    ViewData* stored = mViewDataMap->getViewData(dummy, vd->getViewPoint(), needsUpdate);
    stored->copyFrom(*vd);
    stored->setLastUsageTimeStamp(referenceTime);
}

void QuadTreeWorld::reportStats(unsigned int frameNumber, osg::Stats *stats)
{
    stats->setAttribute(frameNumber, "Composite", mCompositeMapRenderer->getCompileSetSize());
}

void QuadTreeWorld::loadCell(int x, int y)
{
    // fallback behavior only for undefined cells (every other is already handled in quadtree)
    float dummy;
    if (!mStorage->getMinMaxHeights(1, osg::Vec2f(x+0.5, y+0.5), dummy, dummy))
        TerrainGrid::loadCell(x,y);
    else
        World::loadCell(x,y);
}

void QuadTreeWorld::unloadCell(int x, int y)
{
    // fallback behavior only for undefined cells (every other is already handled in quadtree)
    float dummy;
    if (!mStorage->getMinMaxHeights(1, osg::Vec2f(x+0.5, y+0.5), dummy, dummy))
        TerrainGrid::unloadCell(x,y);
    else
        World::unloadCell(x,y);
}


}
