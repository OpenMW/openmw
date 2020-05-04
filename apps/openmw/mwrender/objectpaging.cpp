#include "objectpaging.hpp"

#include <unordered_map>

#include <osg/Version>
#include <osg/LOD>
#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osgUtil/IncrementalCompileOperation>

#include <components/esm/esmreader.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/optimizer.hpp>
#include <components/sceneutil/clone.hpp>

#include <osgParticle/ParticleProcessor>
#include <osgParticle/ParticleSystemUpdater>

#include <components/sceneutil/morphgeometry.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/settings/settings.hpp>

#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/world.hpp"

#include "vismask.hpp"

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
          return far ? false : true;
        default:
            return false;
        }
    }

    std::string getModel(int type, const std::string& id, const MWWorld::ESMStore& store)
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
            return std::string();
        }
    }

    osg::ref_ptr<osg::Node> ObjectPaging::getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags, bool far, const osg::Vec3f& viewPoint, bool compile)
    {
        if (!far)return nullptr;
        ChunkId id = std::make_tuple(center, size);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
        if (obj)
            return obj->asNode();
        else
        {
            osg::ref_ptr<osg::Node> node = createChunk(size, center, viewPoint, compile);
            mCache->addEntryToObjectCache(id, node.get());
            return node;
        }
    }

    class CanOptimizeCallback : public SceneUtil::Optimizer::IsOperationPermissibleForObjectCallback
    {
    public:
        virtual bool isOperationPermissibleForObjectImplementation(const SceneUtil::Optimizer* optimizer, const osg::Drawable* node,unsigned int option) const
        {
            return true;
        }
        virtual bool isOperationPermissibleForObjectImplementation(const SceneUtil::Optimizer* optimizer, const osg::Node* node,unsigned int option) const
        {
            return true;
        }
    };

    class CopyOp : public osg::CopyOp
    {
    public:
        CopyOp(bool deep) : mSqrDistance(0.f) {
            unsigned int flags = osg::CopyOp::DEEP_COPY_NODES;
            if (deep)
                flags |= osg::CopyOp::DEEP_COPY_DRAWABLES;
            setCopyFlags(flags);
        }

        float mSqrDistance;
        osg::Vec3f mViewVector;
        mutable std::vector<const osg::Node*> mNodePath;

        virtual osg::Node* operator() (const osg::Node* node) const
        {
            if (dynamic_cast<const osgParticle::ParticleProcessor*>(node))
                return nullptr;
            if (dynamic_cast<const osgParticle::ParticleSystemUpdater*>(node))
                return nullptr;

            if (const osg::Switch* sw = node->asSwitch())
            {
                osg::Group* n = new osg::Group;
                for (unsigned int i=0; i<sw->getNumChildren(); ++i)
                    if (sw->getValue(i))
                        n->addChild(operator()(sw->getChild(i)));
                n->setDataVariance(osg::Object::STATIC);
                return n;
            }
            if (const osg::LOD* lod = dynamic_cast<const osg::LOD*>(node))
            {
                osg::Group* n = new osg::Group;
                for (unsigned int i=0; i<lod->getNumChildren(); ++i)
                    if (lod->getMinRange(i) * lod->getMinRange(i) <= mSqrDistance && mSqrDistance < lod->getMaxRange(i) * lod->getMaxRange(i))
                        n->addChild(operator()(lod->getChild(i)));
                n->setDataVariance(osg::Object::STATIC);
                return n;
            }

            if (const osg::Drawable* d = node->asDrawable())
                return operator()(d);

            mNodePath.push_back(node);

            osg::Node* cloned = osg::clone(node, *this);
            cloned->setDataVariance(osg::Object::STATIC);
            cloned->setUserDataContainer(nullptr);
            cloned->setName("");

            mNodePath.pop_back();

            handleCallbacks(node, cloned);

            return cloned;
        }
        void handleCallbacks(const osg::Node* node, osg::Node *cloned) const
        {
            const osg::Callback* callback = node->getCullCallback();
            while (callback)
            {
                if (callback->className() == std::string("BillboardCallback"))
                    handleBillboard(cloned);
                callback = callback->getNestedCallback();
            }
        }
        void handleBillboard(osg::Node* node) const
        {
            osg::Transform* transform = node->asTransform();
            if (!transform) return;
            osg::MatrixTransform* matrixTransform = transform->asMatrixTransform();
            if (!matrixTransform) return;

            osg::Matrix worldToLocal = osg::Matrix::identity();
            for (auto node : mNodePath)
                if (const osg::Transform* t = node->asTransform())
                    t->computeWorldToLocalMatrix(worldToLocal, nullptr);
            worldToLocal = osg::Matrix::orthoNormal(worldToLocal);

            osg::Matrix billboardMatrix;
            osg::Vec3f viewVector = -(mViewVector + worldToLocal.getTrans());
            viewVector.normalize();
            osg::Vec3f right = viewVector ^ osg::Vec3f(0,0,1);
            right.normalize();
            osg::Vec3f up = right ^ viewVector;
            up.normalize();
            billboardMatrix.makeLookAt(osg::Vec3f(0,0,0), viewVector, up);
            billboardMatrix.invert(billboardMatrix);

            const osg::Matrix& oldMatrix = matrixTransform->getMatrix();
            float mag[3]; // attempt to preserve scale
            for (int i=0;i<3;++i)
                mag[i] = std::sqrt(oldMatrix(0,i) * oldMatrix(0,i) + oldMatrix(1,i) * oldMatrix(1,i) + oldMatrix(2,i) * oldMatrix(2,i));
            osg::Matrix newMatrix;
            worldToLocal.setTrans(0,0,0);
            newMatrix *= worldToLocal;
            newMatrix.preMult(billboardMatrix);
            newMatrix.preMultScale(osg::Vec3f(mag[0], mag[1], mag[2]));
            newMatrix.setTrans(oldMatrix.getTrans());

            matrixTransform->setMatrix(newMatrix);
        }
        virtual osg::Drawable* operator() (const osg::Drawable* drawable) const
        {
            if (dynamic_cast<const osgParticle::ParticleSystem*>(drawable))
                return nullptr;

            if (const SceneUtil::RigGeometry* rig = dynamic_cast<const SceneUtil::RigGeometry*>(drawable))
                return operator()(rig->getSourceGeometry());
            if (const SceneUtil::MorphGeometry* morph = dynamic_cast<const SceneUtil::MorphGeometry*>(drawable))
                return operator()(morph->getSourceGeometry());

            if (getCopyFlags() & DEEP_COPY_DRAWABLES)
            {
                osg::Drawable* d = osg::clone(drawable, *this);
                d->setDataVariance(osg::Object::STATIC);
                d->setUserDataContainer(nullptr);
                d->setName("");
                return d;
            }
            else
                return osg::CopyOp::operator()(drawable);
        }
        virtual osg::Callback* operator() (const osg::Callback* callback) const
        {
            return nullptr;
        }
    };

    class TemplateRef : public osg::Object
    {
    public:
        TemplateRef() {}
        TemplateRef(const TemplateRef& copy, const osg::CopyOp&) : mObjects(copy.mObjects) {}
        META_Object(MWRender, TemplateRef)
        std::vector<osg::ref_ptr<const Object>> mObjects;
    };

    class AnalyzeVisitor : public osg::NodeVisitor
    {
    public:
        AnalyzeVisitor()
         : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
         , mCurrentStateSet(nullptr) {}

        typedef std::unordered_map<osg::StateSet*, unsigned int> StateSetCounter;
        struct Result
        {
            StateSetCounter mStateSetCounter;
            unsigned int mNumVerts = 0;
        };

        virtual void apply(osg::Node& node)
        {
            if (node.getStateSet())
                mCurrentStateSet = node.getStateSet();
            traverse(node);
        }
        virtual void apply(osg::Geometry& geom)
        {
            mResult.mNumVerts += geom.getVertexArray()->getNumElements();
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
        float getMergeBenefit(const Result& result)
        {
            if (result.mStateSetCounter.empty()) return 1;
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
    };

    ObjectPaging::ObjectPaging(Resource::SceneManager* sceneManager)
            : GenericResourceManager<ChunkId>(nullptr)
         , mSceneManager(sceneManager)
    {
        mMergeFactor = Settings::Manager::getFloat("object paging merge factor", "Terrain");
        mMinSize = Settings::Manager::getFloat("object paging min size", "Terrain");
    }

    osg::ref_ptr<osg::Node> ObjectPaging::createChunk(float size, const osg::Vec2f& center, const osg::Vec3f& viewPoint, bool compile)
    {
        osg::Vec2i startCell = osg::Vec2i(std::floor(center.x() - size/2.f), std::floor(center.y() - size/2.f));

        osg::Vec3f worldCenter = osg::Vec3f(center.x(), center.y(), 0)*ESM::Land::REAL_SIZE;
        osg::Vec3f relativeViewPoint = viewPoint - worldCenter;

        std::map<ESM::RefNum, ESM::CellRef> refs;
        std::vector<ESM::ESMReader> esm;
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        for (int cellX = startCell.x(); cellX < startCell.x() + size; ++cellX)
        {
            for (int cellY = startCell.y(); cellY < startCell.y() + size; ++cellY)
            {
                const ESM::Cell* cell = store.get<ESM::Cell>().searchStatic(cellX, cellY);
                if (!cell) continue;
                for (size_t i=0; i<cell->mContextList.size(); ++i)
                {
                    try
                    {
                        unsigned int index = cell->mContextList.at(i).index;
                        if (esm.size()<=index)
                            esm.resize(index+1);
                        cell->restore(esm[index], i);
                        ESM::CellRef ref;
                        ref.mRefNum.mContentFile = ESM::RefNum::RefNum_NoContentFile;
                        bool deleted = false;
                        while(cell->getNextRef(esm[index], ref, deleted))
                        {
                            if (std::find(cell->mMovedRefs.begin(), cell->mMovedRefs.end(), ref.mRefNum) != cell->mMovedRefs.end()) continue;
                            int type = store.findStatic(Misc::StringUtils::lowerCase(ref.mRefID));
                            if (!typeFilter(type,size>=2)) continue;
                            if (deleted) { refs.erase(ref.mRefNum); continue; }
                            refs[ref.mRefNum] = ref;
                        }
                    }
                    catch (std::exception& e)
                    {
                        continue;
                    }
                }
                for (ESM::CellRefTracker::const_iterator it = cell->mLeasedRefs.begin(); it != cell->mLeasedRefs.end(); ++it)
                {
                    ESM::CellRef ref = it->first;
                    bool deleted = it->second;
                    if (deleted) { refs.erase(ref.mRefNum); continue; }
                    int type = store.findStatic(Misc::StringUtils::lowerCase(ref.mRefID));
                    if (!typeFilter(type,size>=2)) continue;
                    refs[ref.mRefNum] = ref;
                }
            }
        }

        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mDisabledMutex);
            for (auto disabled : mDisabled)
                refs.erase(disabled);
        }

        osg::Vec2f minBound = (center - osg::Vec2f(size/2.f, size/2.f));
        osg::Vec2f maxBound = (center + osg::Vec2f(size/2.f, size/2.f));
        struct InstanceList
        {
            std::vector<const ESM::CellRef*> mInstances;
            AnalyzeVisitor::Result mAnalyzeResult;
        };
        typedef std::map<osg::ref_ptr<const osg::Node>, InstanceList> NodeMap;
        NodeMap nodes;
        AnalyzeVisitor analyzeVisitor;

        for (const auto& pair : refs)
        {
            const ESM::CellRef& ref = pair.second;

            osg::Vec3f pos = ref.mPos.asVec3();
            if (size < 1.f)
            {
                osg::Vec3f cellPos = pos / ESM::Land::REAL_SIZE;
                cellPos.x() = std::max(cellPos.x(), std::floor(minBound.x()));
                cellPos.x() = std::min(cellPos.x(), std::ceil(maxBound.x()));
                cellPos.y() = std::max(cellPos.y(), std::floor(minBound.y()));
                cellPos.y() = std::min(cellPos.y(), std::ceil(maxBound.y()));
                if (cellPos.x() < minBound.x() || cellPos.x() > maxBound.x() || cellPos.y() < minBound.y() || cellPos.y() > maxBound.y())
                    continue;
            }

            float d = (viewPoint - pos).length();
            {
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mSizeCacheMutex);
                SizeCache::iterator found = mSizeCache.find(pair.first);
                if (found != mSizeCache.end())
                {
                    if (found->second < d*mMinSize)
                        continue;
                }
            }

            std::string id = Misc::StringUtils::lowerCase(ref.mRefID);
            if (id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker")
                continue; // marker objects that have a hardcoded function in the game logic, should be hidden from the player

            int type = store.findStatic(id);
            std::string model = getModel(type, id, store);
            if (model.empty()) continue;
            model = "meshes/" + model;
/*
            bool useAnim = type != ESM::REC_STAT;
            if (useAnim)
                model = Misc::ResourceHelpers::correctActorModelPath(model, mSceneManager->getVFS());
*/
            osg::ref_ptr<const osg::Node> cnode = mSceneManager->getTemplate(model, compile);

            float radius = cnode->getBound().radius() * ref.mScale;
            if (radius < d*mMinSize)
            {
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mSizeCacheMutex);
                {
                    mSizeCache[pair.first] = radius;
                }
                continue;
            }

            auto emplaced = nodes.emplace(cnode, InstanceList());
            if (emplaced.second)
            {
                const_cast<osg::Node*>(cnode.get())->accept(analyzeVisitor); // const-trickery required because there is no const version of NodeVisitor
                emplaced.first->second.mAnalyzeResult = analyzeVisitor.retrieveResult();
            }
            emplaced.first->second.mInstances.push_back(&ref);
        }

        osg::ref_ptr<osg::Group> group = new osg::Group;
        osg::ref_ptr<osg::Group> mergeGroup = new osg::Group;
        osg::ref_ptr<TemplateRef> templateRefs = new TemplateRef;
        for (const auto& pair : nodes)
        {
            const osg::Node* cnode = pair.first;

            // add a ref to the original template, to hint to the cache that it's still being used and should be kept in cache
            templateRefs->mObjects.push_back(cnode);

            const AnalyzeVisitor::Result& analyzeResult = pair.second.mAnalyzeResult;

            float mergeCost = analyzeResult.mNumVerts * size;
            float mergeBenefit = analyzeVisitor.getMergeBenefit(analyzeResult) * mMergeFactor;
            bool merge = mergeBenefit > mergeCost;

            for (auto cref : pair.second.mInstances)
            {
                const ESM::CellRef& ref = *cref;
                osg::Vec3f pos = ref.mPos.asVec3();

                osg::Matrixf matrix;
                matrix.preMultTranslate(pos - worldCenter);
                matrix.preMultRotate( osg::Quat(ref.mPos.rot[2], osg::Vec3f(0,0,-1)) *
                                        osg::Quat(ref.mPos.rot[1], osg::Vec3f(0,-1,0)) *
                                        osg::Quat(ref.mPos.rot[0], osg::Vec3f(-1,0,0)) );
                matrix.preMultScale(osg::Vec3f(ref.mScale, ref.mScale, ref.mScale));
                osg::ref_ptr<osg::MatrixTransform> trans = new osg::MatrixTransform(matrix);
                trans->setDataVariance(osg::Object::STATIC);

                CopyOp co = CopyOp(merge);
                co.mNodePath.push_back(trans);
                co.mSqrDistance = (viewPoint - pos).length2();
                co.mViewVector = (viewPoint - worldCenter);
                osg::ref_ptr<osg::Node> node = osg::clone(cnode, co);
                node->setUserDataContainer(nullptr);

                trans->addChild(node);

                if (merge)
                    mergeGroup->addChild(trans);
                else
                    group->addChild(trans);
            }
        }

        if (mergeGroup->getNumChildren())
        {
            SceneUtil::Optimizer optimizer;
            if ((relativeViewPoint - mergeGroup->getBound().center()).length2() > mergeGroup->getBound().radius2())
            {
                optimizer.setViewPoint(relativeViewPoint);
                optimizer.setMergeAlphaBlending(true);
            }
            optimizer.setIsOperationPermissibleForObjectCallback(new CanOptimizeCallback);
            unsigned int options = SceneUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS|SceneUtil::Optimizer::REMOVE_REDUNDANT_NODES|SceneUtil::Optimizer::MERGE_GEOMETRY;
            optimizer.optimize(mergeGroup, options);

            group->addChild(mergeGroup);

            auto ico = mSceneManager->getIncrementalCompileOperation();
            if (compile && ico)
            {
                auto compileSet = new osgUtil::IncrementalCompileOperation::CompileSet(mergeGroup);
                ico->add(compileSet);
                compileSet->_subgraphToCompile = group; // for ref counting in SceneManager::updateCache
            }
        }

        group->getBound();
        group->setNodeMask(Mask_Static);
        group->getOrCreateUserDataContainer()->addUserObject(templateRefs);

        return group;
    }

    unsigned int ObjectPaging::getNodeMask()
    {
        return Mask_Static;
    }

    void ObjectPaging::enableObject(const ESM::RefNum & refnum, bool enabled)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mDisabledMutex);
        if (enabled) mDisabled.erase(refnum);
        else mDisabled.insert(refnum);
    }

    void ObjectPaging::clear()
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mDisabledMutex);
        mDisabled.clear();
    }

    void ObjectPaging::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        stats->setAttribute(frameNumber, "Object Chunk", mCache->getCacheSize());
    }

}
