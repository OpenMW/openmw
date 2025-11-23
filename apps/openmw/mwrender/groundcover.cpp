#include "groundcover.hpp"

#include <span>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/VertexAttribDivisor>
#include <osgUtil/CullVisitor>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/misc/convert.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/settings/values.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/terrain/quadtreenode.hpp>

#include "../mwworld/groundcoverstore.hpp"

#include "vismask.hpp"

namespace MWRender
{
    namespace
    {
        using value_type = osgUtil::CullVisitor::value_type;

        // From OSG's CullVisitor.cpp
        inline value_type distance(const osg::Vec3& coord, const osg::Matrix& matrix)
        {
            return -((value_type)coord[0] * (value_type)matrix(0, 2) + (value_type)coord[1] * (value_type)matrix(1, 2)
                + (value_type)coord[2] * (value_type)matrix(2, 2) + matrix(3, 2));
        }

        inline osg::Matrix computeInstanceMatrix(
            const Groundcover::GroundcoverEntry& entry, const osg::Vec3& chunkPosition)
        {
            return osg::Matrix::scale(entry.mScale, entry.mScale, entry.mScale)
                * osg::Matrix(Misc::Convert::makeOsgQuat(entry.mPos))
                * osg::Matrix::translate(entry.mPos.asVec3() - chunkPosition);
        }

        class InstancedComputeNearFarCullCallback : public osg::DrawableCullCallback
        {
        public:
            explicit InstancedComputeNearFarCullCallback(std::span<const Groundcover::GroundcoverEntry> instances,
                const osg::Vec3& chunkPosition, const osg::BoundingBox& instanceBounds)
                : mInstanceMatrices()
                , mInstanceBounds(instanceBounds)
            {
                mInstanceMatrices.reserve(instances.size());
                for (const Groundcover::GroundcoverEntry& instance : instances)
                    mInstanceMatrices.emplace_back(computeInstanceMatrix(instance, chunkPosition));
            }

            bool cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const override
            {
                osgUtil::CullVisitor& cullVisitor = *nv->asCullVisitor();
                osg::CullSettings::ComputeNearFarMode cnfMode = cullVisitor.getComputeNearFarMode();
                const osg::BoundingBox& boundingBox = drawable->getBoundingBox();
                osg::RefMatrix& matrix = *cullVisitor.getModelViewMatrix();

                if (cnfMode != osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES
                    && cnfMode != osg::CullSettings::COMPUTE_NEAR_USING_PRIMITIVES)
                    return false;

                if (drawable->isCullingActive() && cullVisitor.isCulled(boundingBox))
                    return true;

                osg::Vec3 lookVector = cullVisitor.getLookVectorLocal();
                unsigned int bbCornerFar
                    = (lookVector.x() >= 0 ? 1 : 0) | (lookVector.y() >= 0 ? 2 : 0) | (lookVector.z() >= 0 ? 4 : 0);
                unsigned int bbCornerNear = (~bbCornerFar) & 7;
                value_type dNear = distance(boundingBox.corner(bbCornerNear), matrix);
                value_type dFar = distance(boundingBox.corner(bbCornerFar), matrix);

                if (dNear > dFar)
                    std::swap(dNear, dFar);

                if (dFar < 0)
                    return true;

                value_type computedZNear = cullVisitor.getCalculatedNearPlane();
                value_type computedZFar = cullVisitor.getCalculatedFarPlane();

                if (dNear < computedZNear || dFar > computedZFar)
                {
                    osg::Polytope frustum;
                    osg::Polytope::ClippingMask resultMask
                        = cullVisitor.getCurrentCullingSet().getFrustum().getResultMask();
                    if (resultMask)
                    {
                        // Other objects are likely cheaper and should let us skip all but a few groundcover instances
                        cullVisitor.computeNearPlane();
                        computedZNear = cullVisitor.getCalculatedNearPlane();
                        computedZFar = cullVisitor.getCalculatedFarPlane();

                        if (dNear < computedZNear)
                        {
                            dNear = computedZNear;
                            for (const auto& instanceMatrix : mInstanceMatrices)
                            {
                                osg::Matrix fullMatrix = instanceMatrix * matrix;
                                osg::Vec3d instanceLookVector(-fullMatrix(0, 2), -fullMatrix(1, 2), -fullMatrix(2, 2));
                                unsigned int instanceBbCornerFar = (instanceLookVector.x() >= 0 ? 1 : 0)
                                    | (instanceLookVector.y() >= 0 ? 2 : 0) | (instanceLookVector.z() >= 0 ? 4 : 0);
                                unsigned int instanceBbCornerNear = (~instanceBbCornerFar) & 7;
                                value_type instanceDNear
                                    = distance(mInstanceBounds.corner(instanceBbCornerNear), fullMatrix);
                                value_type instanceDFar
                                    = distance(mInstanceBounds.corner(instanceBbCornerFar), fullMatrix);

                                if (instanceDNear > instanceDFar)
                                    std::swap(instanceDNear, instanceDFar);

                                if (instanceDFar < 0 || instanceDNear > dNear)
                                    continue;

                                frustum.setAndTransformProvidingInverse(
                                    cullVisitor.getProjectionCullingStack().back().getFrustum(), fullMatrix);
                                osg::Polytope::PlaneList planes;
                                osg::Polytope::ClippingMask selectorMask = 0x1;
                                for (const auto& plane : frustum.getPlaneList())
                                {
                                    if (resultMask & selectorMask)
                                        planes.push_back(plane);
                                    selectorMask <<= 1;
                                }

                                value_type newNear
                                    = cullVisitor.computeNearestPointInFrustum(fullMatrix, planes, *drawable);
                                dNear = std::min(dNear, newNear);
                            }
                            if (dNear < computedZNear)
                                cullVisitor.setCalculatedNearPlane(dNear);
                        }

                        if (cnfMode == osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES && dFar > computedZFar)
                        {
                            dFar = computedZFar;
                            for (const auto& instanceMatrix : mInstanceMatrices)
                            {
                                osg::Matrix fullMatrix = instanceMatrix * matrix;
                                osg::Vec3d instanceLookVector(-fullMatrix(0, 2), -fullMatrix(1, 2), -fullMatrix(2, 2));
                                unsigned int instanceBbCornerFar = (instanceLookVector.x() >= 0 ? 1 : 0)
                                    | (instanceLookVector.y() >= 0 ? 2 : 0) | (instanceLookVector.z() >= 0 ? 4 : 0);
                                unsigned int instanceBbCornerNear = (~instanceBbCornerFar) & 7;
                                value_type instanceDNear
                                    = distance(mInstanceBounds.corner(instanceBbCornerNear), fullMatrix);
                                value_type instanceDFar
                                    = distance(mInstanceBounds.corner(instanceBbCornerFar), fullMatrix);

                                if (instanceDNear > instanceDFar)
                                    std::swap(instanceDNear, instanceDFar);

                                if (instanceDFar < 0 || instanceDFar < dFar)
                                    continue;

                                frustum.setAndTransformProvidingInverse(
                                    cullVisitor.getProjectionCullingStack().back().getFrustum(), fullMatrix);
                                osg::Polytope::PlaneList planes;
                                osg::Polytope::ClippingMask selectorMask = 0x1;
                                for (const auto& plane : frustum.getPlaneList())
                                {
                                    if (resultMask & selectorMask)
                                        planes.push_back(plane);
                                    selectorMask <<= 1;
                                }

                                value_type newFar = cullVisitor.computeFurthestPointInFrustum(
                                    instanceMatrix * matrix, planes, *drawable);
                                dFar = std::max(dFar, newFar);
                            }
                            if (dFar > computedZFar)
                                cullVisitor.setCalculatedFarPlane(dFar);
                        }
                    }
                }

                return false;
            }

        private:
            std::vector<osg::Matrix> mInstanceMatrices;
            osg::BoundingBox mInstanceBounds;
        };

        class InstancingVisitor : public osg::NodeVisitor
        {
        public:
            explicit InstancingVisitor(
                std::span<const Groundcover::GroundcoverEntry> instances, osg::Vec3f& chunkPosition)
                : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
                , mInstances(instances)
                , mChunkPosition(chunkPosition)
            {
            }

            void apply(osg::Group& group) override
            {
                for (unsigned int i = 0; i < group.getNumChildren();)
                {
                    if (group.getChild(i)->asDrawable() && !group.getChild(i)->asGeometry())
                        group.removeChild(i);
                    else
                        ++i;
                }
                traverse(group);
            }

            void apply(osg::Geometry& geom) override
            {
                for (unsigned int i = 0; i < geom.getNumPrimitiveSets(); ++i)
                {
                    geom.getPrimitiveSet(i)->setNumInstances(static_cast<int>(mInstances.size()));
                }

                osg::ref_ptr<osg::Vec4Array> transforms = new osg::Vec4Array(static_cast<unsigned>(mInstances.size()));
                osg::BoundingBox box;
                osg::BoundingBox originalBox = geom.getBoundingBox();
                float radius = originalBox.radius();
                for (unsigned int i = 0; i < transforms->getNumElements(); i++)
                {
                    osg::Vec3f pos(mInstances[i].mPos.asVec3());
                    osg::Vec3f relativePos = pos - mChunkPosition;
                    (*transforms)[i] = osg::Vec4f(relativePos, mInstances[i].mScale);

                    // Use an additional margin due to groundcover animation
                    float instanceRadius = radius * mInstances[i].mScale * 1.1f;
                    osg::BoundingSphere instanceBounds(relativePos, instanceRadius);
                    box.expandBy(instanceBounds);
                }

                geom.setInitialBound(box);

                osg::ref_ptr<osg::Vec3Array> rotations = new osg::Vec3Array(static_cast<unsigned>(mInstances.size()));
                for (unsigned int i = 0; i < rotations->getNumElements(); i++)
                {
                    (*rotations)[i] = mInstances[i].mPos.asRotationVec3();
                }

                // Display lists do not support instancing in OSG 3.4
                geom.setUseDisplayList(false);
                geom.setUseVertexBufferObjects(true);

                geom.setVertexAttribArray(6, transforms.get(), osg::Array::BIND_PER_VERTEX);
                geom.setVertexAttribArray(7, rotations.get(), osg::Array::BIND_PER_VERTEX);

                geom.addCullCallback(new InstancedComputeNearFarCullCallback(mInstances, mChunkPosition, originalBox));
            }

        private:
            std::span<const Groundcover::GroundcoverEntry> mInstances;
            osg::Vec3f mChunkPosition;
        };

        class DensityCalculator
        {
        public:
            DensityCalculator(float density)
                : mDensity(density)
            {
            }

            bool isInstanceEnabled()
            {
                if (mDensity >= 1.f)
                    return true;

                mCurrentGroundcover += mDensity;
                if (mCurrentGroundcover < 1.f)
                    return false;

                mCurrentGroundcover -= 1.f;

                return true;
            }
            void reset() { mCurrentGroundcover = 0.f; }

        private:
            float mCurrentGroundcover = 0.f;
            float mDensity = 0.f;
        };

        class ViewDistanceCallback : public SceneUtil::NodeCallback<ViewDistanceCallback>
        {
        public:
            ViewDistanceCallback(float dist, const osg::BoundingBox& box)
                : mViewDistance(dist)
                , mBox(box)
            {
            }
            void operator()(osg::Node* node, osg::NodeVisitor* nv)
            {
                if (Terrain::distance(mBox, nv->getEyePoint()) <= mViewDistance)
                    traverse(node, nv);
            }

        private:
            float mViewDistance;
            osg::BoundingBox mBox;
        };

        inline bool isInChunkBorders(ESM::CellRef& ref, osg::Vec2f& minBound, osg::Vec2f& maxBound)
        {
            osg::Vec2f size = maxBound - minBound;
            if (size.x() >= 1 && size.y() >= 1)
                return true;

            osg::Vec3f pos = ref.mPos.asVec3();
            osg::Vec3f cellPos = pos / ESM::Land::REAL_SIZE;
            if ((minBound.x() > std::floor(minBound.x()) && cellPos.x() < minBound.x())
                || (minBound.y() > std::floor(minBound.y()) && cellPos.y() < minBound.y())
                || (maxBound.x() < std::ceil(maxBound.x()) && cellPos.x() >= maxBound.x())
                || (maxBound.y() < std::ceil(maxBound.y()) && cellPos.y() >= maxBound.y()))
                return false;

            return true;
        }
    }

    osg::ref_ptr<osg::Node> Groundcover::getChunk(float size, const osg::Vec2f& center, unsigned char lod,
        unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile)
    {
        if (lod > getMaxLodLevel())
            return nullptr;
        GroundcoverChunkId id = std::make_tuple(center, size);
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
        if (obj)
            return static_cast<osg::Node*>(obj.get());
        else
        {
            InstanceMap instances;
            collectInstances(instances, size, center);
            osg::ref_ptr<osg::Node> node = createChunk(instances, center);
            mCache->addEntryToObjectCache(id, node.get());
            return node;
        }
    }

    Groundcover::Groundcover(
        Resource::SceneManager* sceneManager, float density, float viewDistance, const MWWorld::GroundcoverStore& store)
        : GenericResourceManager<GroundcoverChunkId>(nullptr, Settings::cells().mCacheExpiryDelay)
        , mSceneManager(sceneManager)
        , mDensity(density)
        , mStateset(new osg::StateSet)
        , mGroundcoverStore(store)
    {
        setViewDistance(viewDistance);
        // MGE uses default alpha settings for groundcover, so we can not rely on alpha properties
        // Force a unified alpha handling instead of data from meshes
        osg::ref_ptr<osg::AlphaFunc> alpha = new osg::AlphaFunc(osg::AlphaFunc::GEQUAL, 128.f / 255.f);
        mStateset->setAttributeAndModes(alpha.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        mStateset->setAttributeAndModes(new osg::BlendFunc, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        mStateset->setRenderBinDetails(0, "RenderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
        mStateset->setAttribute(new osg::VertexAttribDivisor(6, 1));
        mStateset->setAttribute(new osg::VertexAttribDivisor(7, 1));

        mProgramTemplate = mSceneManager->getShaderManager().getProgramTemplate()
            ? Shader::ShaderManager::cloneProgram(mSceneManager->getShaderManager().getProgramTemplate())
            : osg::ref_ptr<osg::Program>(new osg::Program);
        mProgramTemplate->addBindAttribLocation("aOffset", 6);
        mProgramTemplate->addBindAttribLocation("aRotation", 7);
    }

    Groundcover::~Groundcover() = default;

    void Groundcover::collectInstances(InstanceMap& instances, float size, const osg::Vec2f& center)
    {
        if (mDensity <= 0.f)
            return;

        osg::Vec2f minBound = (center - osg::Vec2f(size / 2.f, size / 2.f));
        osg::Vec2f maxBound = (center + osg::Vec2f(size / 2.f, size / 2.f));
        DensityCalculator calculator(mDensity);
        ESM::ReadersCache readers;
        osg::Vec2i startCell = osg::Vec2i(static_cast<int>(std::floor(center.x() - size / 2.f)),
            static_cast<int>(std::floor(center.y() - size / 2.f)));
        for (int cellX = startCell.x(); cellX < startCell.x() + size; ++cellX)
        {
            for (int cellY = startCell.y(); cellY < startCell.y() + size; ++cellY)
            {
                ESM::Cell cell;
                mGroundcoverStore.initCell(cell, cellX, cellY);
                if (cell.mContextList.empty())
                    continue;

                calculator.reset();
                std::map<ESM::RefNum, ESM::CellRef> refs;
                for (size_t i = 0; i < cell.mContextList.size(); ++i)
                {
                    const std::size_t index = static_cast<std::size_t>(cell.mContextList[i].index);
                    const ESM::ReadersCache::BusyItem reader = readers.get(index);
                    cell.restore(*reader, i);
                    ESM::CellRef ref;
                    bool deleted = false;
                    while (cell.getNextRef(*reader, ref, deleted))
                    {
                        if (!deleted && refs.find(ref.mRefNum) == refs.end() && !calculator.isInstanceEnabled())
                            deleted = true;
                        if (!deleted && !isInChunkBorders(ref, minBound, maxBound))
                            deleted = true;

                        if (deleted)
                        {
                            refs.erase(ref.mRefNum);
                            continue;
                        }
                        refs[ref.mRefNum] = std::move(ref);
                    }
                }

                for (auto& [refNum, cellRef] : refs)
                {
                    const VFS::Path::NormalizedView model = mGroundcoverStore.getGroundcoverModel(cellRef.mRefID);
                    if (model.empty())
                        continue;
                    auto it = instances.find(model);
                    if (it == instances.end())
                        it = instances.emplace_hint(it, VFS::Path::Normalized(model), std::vector<GroundcoverEntry>());
                    it->second.emplace_back(std::move(cellRef));
                }
            }
        }
    }

    osg::ref_ptr<osg::Node> Groundcover::createChunk(InstanceMap& instances, const osg::Vec2f& center)
    {
        osg::ref_ptr<osg::Group> group = new osg::Group;
        osg::Vec3f worldCenter = osg::Vec3f(center.x(), center.y(), 0) * ESM::Land::REAL_SIZE;
        for (const auto& [model, entries] : instances)
        {
            const osg::Node* temp = mSceneManager->getTemplate(model);
            osg::ref_ptr<osg::Node> node = static_cast<osg::Node*>(temp->clone(osg::CopyOp::DEEP_COPY_NODES
                | osg::CopyOp::DEEP_COPY_DRAWABLES | osg::CopyOp::DEEP_COPY_USERDATA | osg::CopyOp::DEEP_COPY_ARRAYS
                | osg::CopyOp::DEEP_COPY_PRIMITIVES));

            // Keep link to original mesh to keep it in cache
            group->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(temp));

            InstancingVisitor visitor(entries, worldCenter);
            node->accept(visitor);
            group->addChild(node);
        }

        osg::ComputeBoundsVisitor cbv;
        group->accept(cbv);
        osg::BoundingBox box = cbv.getBoundingBox();
        group->addCullCallback(new ViewDistanceCallback(getViewDistance(), box));

        group->setStateSet(mStateset);
        group->setNodeMask(Mask_Groundcover);
        if (mSceneManager->getLightingMethod() != SceneUtil::LightingMethod::FFP)
            group->addCullCallback(new SceneUtil::LightListCallback);
        mSceneManager->recreateShaders(group, "groundcover", true, mProgramTemplate);
        mSceneManager->shareState(group);
        group->getBound();
        return group;
    }

    unsigned int Groundcover::getNodeMask()
    {
        return Mask_Groundcover;
    }

    void Groundcover::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Groundcover Chunk", frameNumber, mCache->getStats(), *stats);
    }
}
