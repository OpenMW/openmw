#include "objectpaging.hpp"

#include <unordered_map>
#include <vector>

#include <osg/LOD>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/Sequence>
#include <osg/Switch>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osgAnimation/BasicAnimationManager>
#include <osgUtil/IncrementalCompileOperation>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadstat.hpp>

#include <components/esm3/readerscache.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/optimizer.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/util.hpp>
#include <components/vfs/manager.hpp>

#include <osgParticle/ParticleProcessor>
#include <osgParticle/ParticleSystemUpdater>

#include <components/misc/rng.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/morphgeometry.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/sceneutil/riggeometryosgaextension.hpp>
#include <components/settings/settings.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"

#include "vismask.hpp"

#include <condition_variable>

#include <components/shader/shadermanager.hpp>
#include "apps/openmw/mwworld/groundcoverstore.hpp"

namespace MWRender
{

    bool typeFilter(int type, bool far)
    {
        switch (type)
        {
            case ESM::REC_STAT:
            case ESM::REC_ACTI:
            case ESM::REC_DOOR:
                return true;
            case ESM::REC_CONT:
                return !far;

            default:
                return false;
        }
    }

    std::string getModel(int type, const ESM::RefId& id, const MWWorld::ESMStore& store)
    {
        switch (type)
        {
            case ESM::REC_STAT:
                return store.get<ESM::Static>().searchStatic(id)->mModel;
            case ESM::REC_ACTI:
                return store.get<ESM::Activator>().searchStatic(id)->mModel;
            case ESM::REC_DOOR:
                return store.get<ESM::Door>().searchStatic(id)->mModel;
            case ESM::REC_CONT:
                return store.get<ESM::Container>().searchStatic(id)->mModel;
            default:
                return {};
        }
    }

    osg::ref_ptr<osg::Node> ObjectPaging::getChunk(float size, const osg::Vec2f& center, unsigned char lod,
        unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile)
    {
        lod = static_cast<unsigned char>(lodFlags >> (4 * 4));
        if (activeGrid && !mActiveGrid)
            return nullptr;

        ChunkId id = std::make_tuple(center, size, activeGrid);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
        if (obj)
            return static_cast<osg::Node*>(obj.get());
        else
        {
            osg::ref_ptr<osg::Node> node = createChunk(size, center, activeGrid, viewPoint, compile, lod);
            mCache->addEntryToObjectCache(id, node.get());
            return node;
        }
    }

    class CanOptimizeCallback : public SceneUtil::Optimizer::IsOperationPermissibleForObjectCallback
    {
    public:
        bool isOperationPermissibleForObjectImplementation(
            const SceneUtil::Optimizer* optimizer, const osg::Drawable* node, unsigned int option) const override
        {
            return true;
        }
        bool isOperationPermissibleForObjectImplementation(
            const SceneUtil::Optimizer* optimizer, const osg::Node* node, unsigned int option) const override
        {
            return (node->getDataVariance() != osg::Object::DYNAMIC);
        }
    };

    namespace
    {
        using LODRange = osg::LOD::MinMaxPair;

        LODRange intersection(const LODRange& left, const LODRange& right)
        {
            return { std::max(left.first, right.first), std::min(left.second, right.second) };
        }

        bool empty(const LODRange& r)
        {
            return r.first >= r.second;
        }

        LODRange operator/(const LODRange& r, float div)
        {
            return { r.first / div, r.second / div };
        }
    }

    class CopyOp : public osg::CopyOp
    {
    public:
        bool mOptimizeBillboards = true;
        LODRange mDistances = { 0.f, 0.f };
        osg::Vec3f mViewVector;
        bool mGroundcover = false;
        osg::Node::NodeMask mCopyMask = ~0u;
        mutable std::vector<const osg::Node*> mNodePath;

        void copy(const osg::Node* toCopy, osg::Group* attachTo)
        {
            const osg::Group* groupToCopy = toCopy->asGroup();
            if (toCopy->getStateSet() || toCopy->asTransform() || !groupToCopy)
                attachTo->addChild(operator()(toCopy));
            else
            {
                for (unsigned int i = 0; i < groupToCopy->getNumChildren(); ++i)
                    attachTo->addChild(operator()(groupToCopy->getChild(i)));
            }
        }

        osg::Node* operator()(const osg::Node* node) const override
        {
            if (!(node->getNodeMask() & mCopyMask))
                return nullptr;

//            if (const osg::Drawable* d = node->asDrawable())
//                return operator()(d);

            if (const osg::Drawable* d = node->asDrawable())
            {
                osg::Node* clone = operator()(d);
                osg::Geometry* geom = clone ? clone->asGeometry() : nullptr;
                if (!mGroundcover || !geom) return clone;

                osg::Array* vertexArray = geom->getVertexArray();
                if (!vertexArray || vertexArray->getType() != osg::Array::Vec3ArrayType) return clone;
                osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(vertexArray);

                // We should keep an original vertex array to animate groundcover page properly
                osg::ref_ptr<osg::FloatArray> attrs = new osg::FloatArray(vertices->getNumElements());
                for (unsigned int i = 0; i < vertices->getNumElements(); i++)
                {
                    (*attrs)[i] = (*vertices)[i].z();
                }

                //if (needvbo(geom))
                    attrs->setVertexBufferObject(new osg::VertexBufferObject);

                geom->setVertexAttribArray(1, attrs, osg::Array::BIND_PER_VERTEX);

                return geom;
            }

            if (dynamic_cast<const osgParticle::ParticleProcessor*>(node))
                return nullptr;
            if (dynamic_cast<const osgParticle::ParticleSystemUpdater*>(node))
                return nullptr;

            if (const osg::Switch* sw = node->asSwitch())
            {
                osg::Group* n = new osg::Group;
                for (unsigned int i = 0; i < sw->getNumChildren(); ++i)
                    if (sw->getValue(i))
                        n->addChild(operator()(sw->getChild(i)));
                n->setDataVariance(osg::Object::STATIC);
                return n;
            }
            if (const osg::LOD* lod = dynamic_cast<const osg::LOD*>(node))
            {
                std::vector<std::pair<osg::ref_ptr<osg::Node>, LODRange>> children;
                for (unsigned int i = 0; i < lod->getNumChildren(); ++i)
                    if (const auto r = intersection(lod->getRangeList()[i], mDistances); !empty(r))
                        children.emplace_back(operator()(lod->getChild(i)), lod->getRangeList()[i]);
                if (children.empty())
                    return nullptr;

                if (children.size() == 1)
                    return children.front().first.release();
                else
                {
                    osg::LOD* n = new osg::LOD;
                    for (const auto& [child, range] : children)
                        n->addChild(child, range.first, range.second);
                    n->setDataVariance(osg::Object::STATIC);
                    return n;
                }
            }
            if (const osg::Sequence* sq = dynamic_cast<const osg::Sequence*>(node))
            {
                osg::Group* n = new osg::Group;
                n->addChild(operator()(sq->getChild(sq->getValue() != -1 ? sq->getValue() : 0)));
                n->setDataVariance(osg::Object::STATIC);
                return n;
            }

            mNodePath.push_back(node);

            osg::Node* cloned = static_cast<osg::Node*>(node->clone(*this));
            cloned->setDataVariance(osg::Object::STATIC);
            cloned->setUserDataContainer(nullptr);
            cloned->setName("");

            mNodePath.pop_back();

            handleCallbacks(node, cloned);

            return cloned;
        }
        void handleCallbacks(const osg::Node* node, osg::Node* cloned) const
        {
            for (const osg::Callback* callback = node->getCullCallback(); callback != nullptr;
                 callback = callback->getNestedCallback())
            {
                if (callback->className() == std::string("BillboardCallback"))
                {
                    if (mOptimizeBillboards)
                    {
                        handleBillboard(cloned);
                        continue;
                    }
                    else
                        cloned->setDataVariance(osg::Object::DYNAMIC);
                }

                if (node->getCullCallback()->getNestedCallback())
                {
                    osg::Callback* clonedCallback = osg::clone(callback, osg::CopyOp::SHALLOW_COPY);
                    clonedCallback->setNestedCallback(nullptr);
                    cloned->addCullCallback(clonedCallback);
                }
                else
                    cloned->addCullCallback(const_cast<osg::Callback*>(callback));
            }
        }
        void handleBillboard(osg::Node* node) const
        {
            osg::Transform* transform = node->asTransform();
            if (!transform)
                return;
            osg::MatrixTransform* matrixTransform = transform->asMatrixTransform();
            if (!matrixTransform)
                return;

            osg::Matrix worldToLocal = osg::Matrix::identity();
            for (auto pathNode : mNodePath)
                if (const osg::Transform* t = pathNode->asTransform())
                    t->computeWorldToLocalMatrix(worldToLocal, nullptr);
            worldToLocal = osg::Matrix::orthoNormal(worldToLocal);

            osg::Matrix billboardMatrix;
            osg::Vec3f viewVector = -(mViewVector + worldToLocal.getTrans());
            viewVector.normalize();
            osg::Vec3f right = viewVector ^ osg::Vec3f(0, 0, 1);
            right.normalize();
            osg::Vec3f up = right ^ viewVector;
            up.normalize();
            billboardMatrix.makeLookAt(osg::Vec3f(0, 0, 0), viewVector, up);
            billboardMatrix.invert(billboardMatrix);

            const osg::Matrix& oldMatrix = matrixTransform->getMatrix();
            float mag[3]; // attempt to preserve scale
            for (int i = 0; i < 3; ++i)
                mag[i] = std::sqrt(oldMatrix(0, i) * oldMatrix(0, i) + oldMatrix(1, i) * oldMatrix(1, i)
                    + oldMatrix(2, i) * oldMatrix(2, i));
            osg::Matrix newMatrix;
            worldToLocal.setTrans(0, 0, 0);
            newMatrix *= worldToLocal;
            newMatrix.preMult(billboardMatrix);
            newMatrix.preMultScale(osg::Vec3f(mag[0], mag[1], mag[2]));
            newMatrix.setTrans(oldMatrix.getTrans());

            matrixTransform->setMatrix(newMatrix);
        }
        osg::Drawable* operator()(const osg::Drawable* drawable) const override
        {
            if (!(drawable->getNodeMask() & mCopyMask))
                return nullptr;

            if (dynamic_cast<const osgParticle::ParticleSystem*>(drawable))
                return nullptr;

            if (dynamic_cast<const SceneUtil::OsgaRigGeometry*>(drawable))
                return nullptr;
            if (const SceneUtil::RigGeometry* rig = dynamic_cast<const SceneUtil::RigGeometry*>(drawable))
                return operator()(rig->getSourceGeometry());
            if (const SceneUtil::MorphGeometry* morph = dynamic_cast<const SceneUtil::MorphGeometry*>(drawable))
                return operator()(morph->getSourceGeometry());

            if (getCopyFlags() & DEEP_COPY_DRAWABLES)
            {
                osg::Drawable* d = static_cast<osg::Drawable*>(drawable->clone(*this));
                d->setDataVariance(osg::Object::STATIC);
                d->setUserDataContainer(nullptr);
                d->setName("");
                return d;
            }
            else
                return const_cast<osg::Drawable*>(drawable);
        }
        osg::Callback* operator()(const osg::Callback* callback) const override { return nullptr; }
    };

    class RefnumSet : public osg::Object
    {
    public:
        RefnumSet() {}
        RefnumSet(const RefnumSet& copy, const osg::CopyOp&)
            : mRefnums(copy.mRefnums)
        {
        }
        META_Object(MWRender, RefnumSet)
        std::vector<ESM::RefNum> mRefnums;
    };

    class AnalyzeVisitor : public osg::NodeVisitor
    {
    public:
        AnalyzeVisitor(osg::Node::NodeMask analyzeMask)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mCurrentStateSet(nullptr)
        {
            setTraversalMask(analyzeMask);
        }

        typedef std::unordered_map<osg::StateSet*, unsigned int> StateSetCounter;
        struct Result
        {
            StateSetCounter mStateSetCounter;
            unsigned int mNumVerts = 0;
        };

        void apply(osg::Node& node) override
        {
            if (node.getStateSet())
                mCurrentStateSet = node.getStateSet();

            if (osg::Switch* sw = node.asSwitch())
            {
                for (unsigned int i = 0; i < sw->getNumChildren(); ++i)
                    if (sw->getValue(i))
                        traverse(*sw->getChild(i));
                return;
            }
            if (osg::LOD* lod = dynamic_cast<osg::LOD*>(&node))
            {
                for (unsigned int i = 0; i < lod->getNumChildren(); ++i)
                    if (const auto r = intersection(lod->getRangeList()[i], mDistances); !empty(r))
                        traverse(*lod->getChild(i));
                return;
            }
            if (osg::Sequence* sq = dynamic_cast<osg::Sequence*>(&node))
            {
                traverse(*sq->getChild(sq->getValue() != -1 ? sq->getValue() : 0));
                return;
            }

            traverse(node);
        }
        void apply(osg::Geometry& geom) override
        {
            if (osg::Array* array = geom.getVertexArray())
                mResult.mNumVerts += array->getNumElements();

            ++mResult.mStateSetCounter[mCurrentStateSet];
            ++mGlobalStateSetCounter[mCurrentStateSet];
        }
        Result retrieveResult()
        {
            Result result = mResult;
            mResult = Result();
            mCurrentStateSet = nullptr;
            return result;
        }
        void addInstance(const Result& result)
        {
            for (auto pair : result.mStateSetCounter)
                mGlobalStateSetCounter[pair.first] += pair.second;
        }
        float getMergeBenefit(const Result& result)
        {
            if (result.mStateSetCounter.empty())
                return 1;
            float mergeBenefit = 0;
            for (auto pair : result.mStateSetCounter)
            {
                mergeBenefit += mGlobalStateSetCounter[pair.first];
            }
            mergeBenefit /= result.mStateSetCounter.size();
            return mergeBenefit;
        }

        Result mResult;
        osg::StateSet* mCurrentStateSet;
        StateSetCounter mGlobalStateSetCounter;
        LODRange mDistances = { 0.f, 0.f };
    };

    class DebugVisitor : public osg::NodeVisitor
    {
    public:
        DebugVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }
        void apply(osg::Drawable& node) override
        {
            osg::ref_ptr<osg::Material> m(new osg::Material);
            osg::Vec4f color(
                Misc::Rng::rollProbability(), Misc::Rng::rollProbability(), Misc::Rng::rollProbability(), 0.f);
            color.normalize();
            m->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.1f, 0.1f, 0.1f, 1.f));
            m->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.1f, 0.1f, 0.1f, 1.f));
            m->setColorMode(osg::Material::OFF);
            m->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(color));
            osg::ref_ptr<osg::StateSet> stateset
                = node.getStateSet() ? osg::clone(node.getStateSet(), osg::CopyOp::SHALLOW_COPY) : new osg::StateSet;
            stateset->setAttribute(m);
            stateset->addUniform(new osg::Uniform("colorMode", 0));
            stateset->addUniform(new osg::Uniform("emissiveMult", 1.f));
            stateset->addUniform(new osg::Uniform("specStrength", 1.f));
            node.setStateSet(stateset);
        }
    };

    class AddRefnumMarkerVisitor : public osg::NodeVisitor
    {
    public:
        AddRefnumMarkerVisitor(const ESM::RefNum& refnum)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mRefnum(refnum)
        {
        }
        ESM::RefNum mRefnum;
        void apply(osg::Geometry& node) override
        {
            osg::ref_ptr<RefnumMarker> marker(new RefnumMarker);
            marker->mRefnum = mRefnum;
            if (osg::Array* array = node.getVertexArray())
                marker->mNumVertices = array->getNumElements();
            node.getOrCreateUserDataContainer()->addUserObject(marker);
        }
    };

    ObjectPaging::ObjectPaging(Resource::SceneManager* sceneManager, bool groundcover, const MWWorld::GroundcoverStore& store)
        : GenericResourceManager<ChunkId>(nullptr)
        , mSceneManager(sceneManager)
        , mGroundcover(groundcover)
        , mGroundcoverStore(store)
        , mRefTrackerLocked(false)
    {
        mActiveGrid = Settings::Manager::getBool("object paging active grid", "Terrain");
        mDebugBatches = Settings::Manager::getBool("debug chunks", "Terrain");
        mMergeFactor = Settings::Manager::getFloat("object paging merge factor", "Terrain");
        mMinSize = Settings::Manager::getFloat("object paging min size", "Terrain");
        mMinSizeMergeFactor = Settings::Manager::getFloat("object paging min size merge factor", "Terrain");
        mMinSizeCostMultiplier = Settings::Manager::getFloat("object paging min size cost multiplier", "Terrain");

        if (mGroundcover)
        {
            mUseGroundcoverStore = Settings::Manager::getBool("auto load", "Groundcover") == true ? false : true;
            mMergeFactor = Settings::Manager::getFloat("merge factor", "Groundcover");
            mGroundcoverDensity = Settings::Manager::getFloat("density", "Groundcover")/100.f;
            mMinSize = 0.01;
        }
    }

    std::map<ESM::RefNum, ESM::CellRef> ObjectPaging::collectRefs(float size, const osg::Vec2f& center)
    {
        std::map<ESM::RefNum, ESM::CellRef> refs;
        ESM::ReadersCache readers;
        const auto& world = MWBase::Environment::get().getWorld();
        const auto& store = world->getStore();

        osg::Vec2i startCell = osg::Vec2i(std::floor(center.x() - size / 2.f), std::floor(center.y() - size / 2.f));

        if (mGroundcover && mUseGroundcoverStore)
        {
            for (int cellX = startCell.x(); cellX < startCell.x() + size; ++cellX)
            {
                for (int cellY = startCell.y(); cellY < startCell.y() + size; ++cellY)
                {
                    ESM::Cell cell;
                    mGroundcoverStore.initCell(cell, cellX, cellY);
                    if (cell.mContextList.empty()) 
                        continue;

                    float currentGroundcover = 0.f;
                    for (size_t i = 0; i < cell.mContextList.size(); ++i)
                    {
                        try
                        {
                            const std::size_t index = static_cast<std::size_t>(cell.mContextList[i].index);
                            const ESM::ReadersCache::BusyItem reader = readers.get(index);
                            cell.restore(*reader, i);
                            ESM::CellRef ref;
                            bool deleted = false;
                            while (cell.getNextRef(*reader, ref, deleted))
                            {
                                if (deleted)
                                {
                                    refs.erase(ref.mRefNum);
                                    continue;
                                }

                                currentGroundcover += mGroundcoverDensity;
                                if (currentGroundcover < 1.f) continue;
                                currentGroundcover -= 1.f;

                                refs[ref.mRefNum] = std::move(ref);
                            }
                        }
                        catch (std::exception&)
                        {
                            continue;
                        }
                    }
                }
            }
        }
        else
        {
            for (int cellX = startCell.x(); cellX < startCell.x() + size; ++cellX)
            {
                for (int cellY = startCell.y(); cellY < startCell.y() + size; ++cellY)
                {
                    const ESM::Cell* cell = store.get<ESM::Cell>().searchStatic(cellX, cellY);
                    if (!cell)
                        continue;
                    float currentGroundcover = 0.f;
                    for (size_t i = 0; i < cell->mContextList.size(); ++i)
                    {
                        try
                        {
                            const std::size_t index = static_cast<std::size_t>(cell->mContextList[i].index);
                            const ESM::ReadersCache::BusyItem reader = readers.get(index);
                            cell->restore(*reader, i);
                            ESM::CellRef ref;
                            ESM::MovedCellRef cMRef;
                            bool deleted = false;
                            bool moved = false;
                            while (ESM::Cell::getNextRef(
                                *reader, ref, deleted, cMRef, moved, ESM::Cell::GetNextRefMode::LoadOnlyNotMoved))
                            {
                                if (moved)
                                    continue;

                                if (std::find(cell->mMovedRefs.begin(), cell->mMovedRefs.end(), ref.mRefNum)
                                    != cell->mMovedRefs.end())
                                    continue;

                                int type = store.findStatic(ref.mRefID);
                                if (!typeFilter(type, size >= 2))
                                    continue;
                                if (deleted)
                                {
                                    refs.erase(ref.mRefNum);
                                    continue;
                                }

                                // FIXME: per-instance check requires search
                                if(mGroundcover && type != ESM::REC_STAT)
                                   continue;

                                if(type == ESM::REC_STAT)
                                {
                                    bool isGroundcoverModel = store.get<ESM::Static>().searchStatic(ref.mRefID)->mIsGroundcover;
                                    if(mGroundcover != isGroundcoverModel)
                                       continue;

                                    if (mGroundcover)
                                    {
                                        currentGroundcover += mGroundcoverDensity;
                                        if (currentGroundcover < 1.f) continue;
                                        currentGroundcover -= 1.f;
                                    }
                                }

                                refs[ref.mRefNum] = std::move(ref);
                            }
                        }
                        catch (std::exception&)
                        {
                            continue;
                        }
                    }

                    for (auto [ref, deleted] : cell->mLeasedRefs)
                    {
                        if (deleted)
                        {
                            refs.erase(ref.mRefNum);
                            continue;
                        }
                        int type = store.findStatic(ref.mRefID);
                        if (!typeFilter(type, size >= 2))
                            continue;
                        refs[ref.mRefNum] = std::move(ref);
                    }
                }
            }
        }

        return refs;
    }

    osg::ref_ptr<osg::Node> ObjectPaging::createChunk(float size, const osg::Vec2f& center, bool activeGrid,
        const osg::Vec3f& viewPoint, bool compile, unsigned char lod)
    {
        osg::Vec3f worldCenter = osg::Vec3f(center.x(), center.y(), 0) * ESM::Land::REAL_SIZE;
        osg::Vec3f relativeViewPoint = viewPoint - worldCenter;

        std::map<ESM::RefNum, ESM::CellRef> refs = collectRefs(size, center);
        const auto& world = MWBase::Environment::get().getWorld();
        const auto& store = world->getStore();

        if (activeGrid && !mGroundcover)
        {
            std::lock_guard<std::mutex> lock(mRefTrackerMutex);
            for (auto ref : getRefTracker().mBlacklist)
                refs.erase(ref);
        }

        osg::Vec2f minBound = (center - osg::Vec2f(size / 2.f, size / 2.f));
        osg::Vec2f maxBound = (center + osg::Vec2f(size / 2.f, size / 2.f));
        struct InstanceList
        {
            std::vector<const ESM::CellRef*> mInstances;
            AnalyzeVisitor::Result mAnalyzeResult;
            bool mNeedCompile = false;
        };
        typedef std::map<osg::ref_ptr<const osg::Node>, InstanceList> NodeMap;
        NodeMap nodes;
        osg::ref_ptr<RefnumSet> refnumSet = activeGrid ? new RefnumSet : nullptr;

        // Mask_UpdateVisitor is used in such cases in NIF loader:
        // 1. For collision nodes, which is not supposed to be rendered.
        // 2. For nodes masked via Flag_Hidden (VisController can change this flag value at runtime).
        // Since ObjectPaging does not handle VisController, we can just ignore both types of nodes.
        constexpr auto copyMask = ~Mask_UpdateVisitor;

        const auto smallestDistanceToChunk = (size > 1 / 8.f) ? (size * ESM::Land::REAL_SIZE) : 0.f;
        const auto higherDistanceToChunk = [&] {
            if (!activeGrid)
                return smallestDistanceToChunk + 1;
            return ((size < 1) ? 5 : 3) * ESM::Land::REAL_SIZE * size + 1;
        }();

        AnalyzeVisitor analyzeVisitor(copyMask);
        float minSize = mMinSize;
        if (mMinSizeMergeFactor)
            minSize *= mMinSizeMergeFactor;
        for (const auto& pair : refs)
        {
            const ESM::CellRef& ref = pair.second;

            osg::Vec3f pos = ref.mPos.asVec3();
            if (size < 1.f)
            {
                osg::Vec3f cellPos = pos / ESM::Land::REAL_SIZE;
                if ((minBound.x() > std::floor(minBound.x()) && cellPos.x() < minBound.x())
                    || (minBound.y() > std::floor(minBound.y()) && cellPos.y() < minBound.y())
                    || (maxBound.x() < std::ceil(maxBound.x()) && cellPos.x() >= maxBound.x())
                    || (maxBound.y() < std::ceil(maxBound.y()) && cellPos.y() >= maxBound.y()))
                    continue;
            }

            float dSqr = (viewPoint - pos).length2();
            if (!activeGrid)
            {
                std::lock_guard<std::mutex> lock(mSizeCacheMutex);
                SizeCache::iterator found = mSizeCache.find(pair.first);
                if (found != mSizeCache.end() && found->second < dSqr * minSize * minSize)
                    continue;
            }

            if (Misc::ResourceHelpers::isHiddenMarker(ref.mRefID))
                continue;

            int type = store.findStatic(ref.mRefID);
            std::string model;
            if(mGroundcover && mUseGroundcoverStore)
                model = mGroundcoverStore.getGroundcoverModel(ref.mRefID);
            else
                model = getModel(type, ref.mRefID, store);

            if (model.empty())
                continue;

            if(!mGroundcover || (mGroundcover && !mUseGroundcoverStore))
                model = Misc::ResourceHelpers::correctMeshPath(model, mSceneManager->getVFS());

            if (activeGrid && type != ESM::REC_STAT)
            {
                model = Misc::ResourceHelpers::correctActorModelPath(model, mSceneManager->getVFS());
                std::string kfname = Misc::StringUtils::lowerCase(model);
                if (kfname.size() > 4 && kfname.compare(kfname.size() - 4, 4, ".nif") == 0)
                {
                    kfname.replace(kfname.size() - 4, 4, ".kf");
                    if (mSceneManager->getVFS()->exists(kfname))
                        continue;
                }
            }

            if (!activeGrid)
            {
                std::lock_guard<std::mutex> lock(mLODNameCacheMutex);
                LODNameCacheKey key{ model, lod };
                LODNameCache::const_iterator found = mLODNameCache.lower_bound(key);
                if (found != mLODNameCache.end() && found->first == key)
                    model = found->second;
                else
                    model = mLODNameCache
                                .insert(found,
                                    { key,
                                        Misc::ResourceHelpers::getLODMeshName(
                                            world->getESMVersions()[ref.mRefNum.mContentFile], model,
                                            mSceneManager->getVFS(), lod) })
                                ->second;
            }

            osg::ref_ptr<const osg::Node> cnode = mSceneManager->getTemplate(model, false);

            if (!mGroundcover)
            {
                if (activeGrid)
                {
                    if (cnode->getNumChildrenRequiringUpdateTraversal() > 0
                        || SceneUtil::hasUserDescription(cnode, Constants::NightDayLabel)
                        || SceneUtil::hasUserDescription(cnode, Constants::HerbalismLabel)
                        || (cnode->getName() == "Collada visual scene group"
                            && dynamic_cast<const osgAnimation::BasicAnimationManager*>(cnode->getUpdateCallback())))
                        continue;
                    else
                        refnumSet->mRefnums.push_back(pair.first);
                }

                {
                    std::lock_guard<std::mutex> lock(mRefTrackerMutex);
                    if (getRefTracker().mDisabled.count(pair.first))
                        continue;
                }
            }

            float radius2 = cnode->getBound().radius2() * ref.mScale * ref.mScale;
            if (radius2 < dSqr * minSize * minSize && !activeGrid)
            {
                std::lock_guard<std::mutex> lock(mSizeCacheMutex);
                mSizeCache[pair.first] = radius2;
                continue;
            }

            auto emplaced = nodes.emplace(cnode, InstanceList());
            if (emplaced.second)
            {
                analyzeVisitor.mDistances = LODRange{ smallestDistanceToChunk, higherDistanceToChunk } / ref.mScale;
                const_cast<osg::Node*>(cnode.get())
                    ->accept(
                        analyzeVisitor); // const-trickery required because there is no const version of NodeVisitor
                emplaced.first->second.mAnalyzeResult = analyzeVisitor.retrieveResult();
                emplaced.first->second.mNeedCompile = compile && cnode->referenceCount() <= 3;
            }
            else
                analyzeVisitor.addInstance(emplaced.first->second.mAnalyzeResult);
            emplaced.first->second.mInstances.push_back(&ref);
        }

        osg::ref_ptr<osg::Group> group = new osg::Group;
        osg::ref_ptr<osg::Group> mergeGroup = new osg::Group;
        osg::ref_ptr<Resource::TemplateMultiRef> templateRefs = new Resource::TemplateMultiRef;
        osgUtil::StateToCompile stateToCompile(0, nullptr);
        CopyOp copyop;
        copyop.mGroundcover = mGroundcover;
        copyop.mCopyMask = copyMask;
        for (const auto& pair : nodes)
        {
            const osg::Node* cnode = pair.first;

            const AnalyzeVisitor::Result& analyzeResult = pair.second.mAnalyzeResult;

            float mergeCost = analyzeResult.mNumVerts * size;
            float mergeBenefit = analyzeVisitor.getMergeBenefit(analyzeResult) * mMergeFactor;
            bool merge = mergeBenefit > mergeCost;

            float minSizeMerged = mMinSize;
            float factor2 = mergeBenefit > 0 ? std::min(1.f, mergeCost * mMinSizeCostMultiplier / mergeBenefit) : 1;
            float minSizeMergeFactor2 = (1 - factor2) * mMinSizeMergeFactor + factor2;
            if (minSizeMergeFactor2 > 0)
                minSizeMerged *= minSizeMergeFactor2;

            unsigned int numinstances = 0;
            for (auto cref : pair.second.mInstances)
            {
                const ESM::CellRef& ref = *cref;
                osg::Vec3f pos = ref.mPos.asVec3();

                if (!activeGrid && minSizeMerged != minSize
                    && cnode->getBound().radius2() * cref->mScale * cref->mScale
                        < (viewPoint - pos).length2() * minSizeMerged * minSizeMerged)
                    continue;

                osg::Vec3f nodePos = pos - worldCenter;
                osg::Quat nodeAttitude = osg::Quat(ref.mPos.rot[2], osg::Vec3f(0, 0, -1))
                    * osg::Quat(ref.mPos.rot[1], osg::Vec3f(0, -1, 0))
                    * osg::Quat(ref.mPos.rot[0], osg::Vec3f(-1, 0, 0));
                osg::Vec3f nodeScale = osg::Vec3f(ref.mScale, ref.mScale, ref.mScale);

                osg::ref_ptr<osg::Group> trans;
                if (merge)
                {
                    // Optimizer currently supports only MatrixTransforms.
                    osg::Matrixf matrix;
                    matrix.preMultTranslate(nodePos);
                    matrix.preMultRotate(nodeAttitude);
                    matrix.preMultScale(nodeScale);
                    trans = new osg::MatrixTransform(matrix);
                    trans->setDataVariance(osg::Object::STATIC);
                }
                else
                {
                    trans = new SceneUtil::PositionAttitudeTransform;
                    SceneUtil::PositionAttitudeTransform* pat
                        = static_cast<SceneUtil::PositionAttitudeTransform*>(trans.get());
                    pat->setPosition(nodePos);
                    pat->setScale(nodeScale);
                    pat->setAttitude(nodeAttitude);
                }

                // DO NOT COPY AND PASTE THIS CODE. Cloning osg::Geometry without also cloning its contained Arrays is
                // generally unsafe. In this specific case the operation is safe under the following two assumptions:
                // - When Arrays are removed or replaced in the cloned geometry, the original Arrays in their place must
                // outlive the cloned geometry regardless. (ensured by TemplateMultiRef)
                // - Arrays that we add or replace in the cloned geometry must be explicitely forbidden from reusing
                // BufferObjects of the original geometry. (ensured by needvbo() in optimizer.cpp)
                copyop.setCopyFlags(merge ? osg::CopyOp::DEEP_COPY_NODES | osg::CopyOp::DEEP_COPY_DRAWABLES
                                          : osg::CopyOp::DEEP_COPY_NODES);
                copyop.mOptimizeBillboards = (size > 1 / 4.f);
                copyop.mNodePath.push_back(trans);
                copyop.mDistances = LODRange{ smallestDistanceToChunk, higherDistanceToChunk } / ref.mScale;
                copyop.mViewVector = (viewPoint - worldCenter);
                copyop.copy(cnode, trans);
                copyop.mNodePath.pop_back();

                if (activeGrid)
                {
                    if (merge)
                    {
                        AddRefnumMarkerVisitor visitor(ref.mRefNum);
                        trans->accept(visitor);
                    }
                    else
                    {
                        osg::ref_ptr<RefnumMarker> marker = new RefnumMarker;
                        marker->mRefnum = ref.mRefNum;
                        trans->getOrCreateUserDataContainer()->addUserObject(marker);
                    }
                }

                osg::Group* attachTo = merge ? mergeGroup : group;
                attachTo->addChild(trans);
                ++numinstances;
            }
            if (numinstances > 0)
            {
                // add a ref to the original template to help verify the safety of shallow cloning operations
                // in addition, we hint to the cache that it's still being used and should be kept in cache
                templateRefs->addRef(cnode);

                if (pair.second.mNeedCompile)
                {
                    int mode = osgUtil::GLObjectsVisitor::COMPILE_STATE_ATTRIBUTES;
                    if (!merge)
                        mode |= osgUtil::GLObjectsVisitor::COMPILE_DISPLAY_LISTS;
                    stateToCompile._mode = mode;
                    const_cast<osg::Node*>(cnode)->accept(stateToCompile);
                }
            }
        }

        if (mergeGroup->getNumChildren())
        {
            SceneUtil::Optimizer optimizer;
            if (!mGroundcover && size > 1/8.f)
            {
                optimizer.setViewPoint(relativeViewPoint);
                optimizer.setMergeAlphaBlending(true);
            }
/*
            if (mGroundcover)
            {
                optimizer.setRemoveAlphaBlending(true);
            }
*/
            optimizer.setIsOperationPermissibleForObjectCallback(new CanOptimizeCallback);
            unsigned int options = SceneUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS
                | SceneUtil::Optimizer::REMOVE_REDUNDANT_NODES | SceneUtil::Optimizer::MERGE_GEOMETRY;

            optimizer.optimize(mergeGroup, options);

            group->addChild(mergeGroup);

            if (mDebugBatches)
            {
                DebugVisitor dv;
                mergeGroup->accept(dv);
            }
            if (compile)
            {
                stateToCompile._mode = osgUtil::GLObjectsVisitor::COMPILE_DISPLAY_LISTS;
                mergeGroup->accept(stateToCompile);
            }
        }

        auto ico = mSceneManager->getIncrementalCompileOperation();
        if (!stateToCompile.empty() && ico)
        {
            auto compileSet = new osgUtil::IncrementalCompileOperation::CompileSet(group);
            compileSet->buildCompileMap(ico->getContextSet(), stateToCompile);
            ico->add(compileSet, false);
        }

        group->getBound();
        group->setNodeMask(mGroundcover ? Mask_Groundcover : Mask_Static);
        osg::UserDataContainer* udc = group->getOrCreateUserDataContainer();
        if (activeGrid && !mGroundcover)
        {
            std::sort(refnumSet->mRefnums.begin(), refnumSet->mRefnums.end());
            refnumSet->mRefnums.erase(
                std::unique(refnumSet->mRefnums.begin(), refnumSet->mRefnums.end()), refnumSet->mRefnums.end());
            udc->addUserObject(refnumSet);
            group->addCullCallback(new SceneUtil::LightListCallback);
        }
        udc->addUserObject(templateRefs);

        if (mGroundcover)
        {
            if (mSceneManager->getLightingMethod() != SceneUtil::LightingMethod::FFP)
                group->setCullCallback(new SceneUtil::LightListCallback);
/*
            osg::StateSet* stateset = group->getOrCreateStateSet();
            stateset->removeAttribute(osg::StateAttribute::MATERIAL);
            stateset->removeAttribute(osg::StateAttribute::ALPHAFUNC);
            stateset->removeMode(GL_ALPHA_TEST);
            stateset->removeAttribute(osg::StateAttribute::BLENDFUNC);
            stateset->removeMode(GL_BLEND);
            stateset->setRenderBinToInherit();

            osg::ref_ptr<osg::AlphaFunc> alpha = new osg::AlphaFunc(osg::AlphaFunc::GEQUAL, 128.f / 255.f);
            stateset->setAttributeAndModes(alpha.get(), osg::StateAttribute::ON);
*/
            osg::StateSet* stateset = group->getOrCreateStateSet();
            osg::ref_ptr<osg::AlphaFunc> alpha = new osg::AlphaFunc(osg::AlphaFunc::GEQUAL, 128.f / 255.f);
            stateset->setAttributeAndModes(alpha.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateset->setAttributeAndModes(new osg::BlendFunc, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
            stateset->setRenderBinDetails(0, "RenderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);

            mSceneManager->reinstateRemovedState(group);

            static const osg::ref_ptr<osg::Program> programTemplate = mSceneManager->getShaderManager().getProgramTemplate()
                ? Shader::ShaderManager::cloneProgram(mSceneManager->getShaderManager().getProgramTemplate())
                : osg::ref_ptr<osg::Program>(new osg::Program);

            programTemplate->addBindAttribLocation("originalHeight", 1);

            mSceneManager->recreateShaders(group, "groundcover_paging", true, programTemplate);
        }

        return group;
    }

    unsigned int ObjectPaging::getNodeMask()
    {
        return mGroundcover ? Mask_Groundcover : Mask_Static;
    }

    struct ClearCacheFunctor
    {
        void operator()(MWRender::ChunkId id, osg::Object* obj)
        {
            if (intersects(id, mPosition))
                mToClear.insert(id);
        }
        bool intersects(ChunkId id, osg::Vec3f pos)
        {
            if (mActiveGridOnly && !std::get<2>(id))
                return false;
            pos /= ESM::Land::REAL_SIZE;
            clampToCell(pos);
            osg::Vec2f center = std::get<0>(id);
            float halfSize = std::get<1>(id) / 2;
            return pos.x() >= center.x() - halfSize && pos.y() >= center.y() - halfSize
                && pos.x() <= center.x() + halfSize && pos.y() <= center.y() + halfSize;
        }
        void clampToCell(osg::Vec3f& cellPos)
        {
            cellPos.x() = std::clamp<float>(cellPos.x(), mCell.x(), mCell.x() + 1);
            cellPos.y() = std::clamp<float>(cellPos.y(), mCell.y(), mCell.y() + 1);
        }
        osg::Vec3f mPosition;
        osg::Vec2i mCell;
        std::set<MWRender::ChunkId> mToClear;
        bool mActiveGridOnly = false;
    };

    bool ObjectPaging::enableObject(
        int type, const ESM::RefNum& refnum, const osg::Vec3f& pos, const osg::Vec2i& cell, bool enabled)
    {
        if (!typeFilter(type, false))
            return false;

        {
            std::lock_guard<std::mutex> lock(mRefTrackerMutex);
            if (enabled && !getWritableRefTracker().mDisabled.erase(refnum))
                return false;
            if (!enabled && !getWritableRefTracker().mDisabled.insert(refnum).second)
                return false;
            if (mRefTrackerLocked)
                return false;
        }

        ClearCacheFunctor ccf;
        ccf.mPosition = pos;
        ccf.mCell = cell;
        mCache->call(ccf);
        if (ccf.mToClear.empty())
            return false;
        for (const auto& chunk : ccf.mToClear)
            mCache->removeFromObjectCache(chunk);
        return true;
    }

    bool ObjectPaging::blacklistObject(
        int type, const ESM::RefNum& refnum, const osg::Vec3f& pos, const osg::Vec2i& cell)
    {
        if (!typeFilter(type, false))
            return false;

        {
            std::lock_guard<std::mutex> lock(mRefTrackerMutex);
            if (!getWritableRefTracker().mBlacklist.insert(refnum).second)
                return false;
            if (mRefTrackerLocked)
                return false;
        }

        ClearCacheFunctor ccf;
        ccf.mPosition = pos;
        ccf.mCell = cell;
        ccf.mActiveGridOnly = true;
        mCache->call(ccf);
        if (ccf.mToClear.empty())
            return false;
        for (const auto& chunk : ccf.mToClear)
            mCache->removeFromObjectCache(chunk);
        return true;
    }

    void ObjectPaging::clear()
    {
        std::lock_guard<std::mutex> lock(mRefTrackerMutex);
        mRefTrackerNew.mDisabled.clear();
        mRefTrackerNew.mBlacklist.clear();
        mRefTrackerLocked = true;
    }

    bool ObjectPaging::unlockCache()
    {
        if (!mRefTrackerLocked)
            return false;
        {
            std::lock_guard<std::mutex> lock(mRefTrackerMutex);
            mRefTrackerLocked = false;
            if (mRefTracker == mRefTrackerNew)
                return false;
            else
                mRefTracker = mRefTrackerNew;
        }
        mCache->clear();
        return true;
    }

    struct GetRefnumsFunctor
    {
        GetRefnumsFunctor(std::vector<ESM::RefNum>& output)
            : mOutput(output)
        {
        }
        void operator()(MWRender::ChunkId chunkId, osg::Object* obj)
        {
            if (!std::get<2>(chunkId))
                return;
            const osg::Vec2f& center = std::get<0>(chunkId);
            bool activeGrid = (center.x() > mActiveGrid.x() || center.y() > mActiveGrid.y()
                || center.x() < mActiveGrid.z() || center.y() < mActiveGrid.w());
            if (!activeGrid)
                return;

            osg::UserDataContainer* udc = obj->getUserDataContainer();
            if (udc && udc->getNumUserObjects())
            {
                RefnumSet* refnums = dynamic_cast<RefnumSet*>(udc->getUserObject(0));
                if (!refnums)
                    return;
                mOutput.insert(mOutput.end(), refnums->mRefnums.begin(), refnums->mRefnums.end());
            }
        }
        osg::Vec4i mActiveGrid;
        std::vector<ESM::RefNum>& mOutput;
    };

    void ObjectPaging::getPagedRefnums(const osg::Vec4i& activeGrid, std::vector<ESM::RefNum>& out)
    {
        GetRefnumsFunctor grf(out);
        grf.mActiveGrid = activeGrid;
        mCache->call(grf);
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

    void ObjectPaging::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        if (mGroundcover)
            stats->setAttribute(frameNumber, "Groundcover Chunk", mCache->getCacheSize());
        else
            stats->setAttribute(frameNumber, "Object Chunk", mCache->getCacheSize());
    }

    float ObjectPaging::getGroundcoverDensity()
    {
        return mGroundcoverDensity;
    }

    void ObjectPaging::setGroundcoverDensity(float density)
    {
        mGroundcoverDensity = density;
    }

}
