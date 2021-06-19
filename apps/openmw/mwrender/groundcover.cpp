#include "groundcover.hpp"

#include <osg/AlphaFunc>
#include <osg/Geometry>
#include <osg/VertexAttribDivisor>

#include <components/esm/esmreader.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/world.hpp"

#include "vismask.hpp"

namespace MWRender
{
    std::string getGroundcoverModel(int type, const std::string& id, const MWWorld::ESMStore& store)
    {
        switch (type)
        {
          case ESM::REC_STAT:
            return store.get<ESM::Static>().searchStatic(id)->mModel;
          default:
            return std::string();
        }
    }

    void GroundcoverUpdater::setWindSpeed(float windSpeed)
    {
        mWindSpeed = windSpeed;
    }

    void GroundcoverUpdater::setPlayerPos(osg::Vec3f playerPos)
    {
        mPlayerPos = playerPos;
    }

    void GroundcoverUpdater::setDefaults(osg::StateSet *stateset)
    {
        osg::ref_ptr<osg::Uniform> windUniform = new osg::Uniform("windSpeed", 0.0f);
        stateset->addUniform(windUniform.get());

        osg::ref_ptr<osg::Uniform> playerPosUniform = new osg::Uniform("playerPos", osg::Vec3f(0.f, 0.f, 0.f));
        stateset->addUniform(playerPosUniform.get());
    }

    void GroundcoverUpdater::apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
    {
        osg::ref_ptr<osg::Uniform> windUniform = stateset->getUniform("windSpeed");
        if (windUniform != nullptr)
            windUniform->set(mWindSpeed);

        osg::ref_ptr<osg::Uniform> playerPosUniform = stateset->getUniform("playerPos");
        if (playerPosUniform != nullptr)
            playerPosUniform->set(mPlayerPos);
    }

    class InstancingVisitor : public osg::NodeVisitor
    {
    public:
        InstancingVisitor(std::vector<Groundcover::GroundcoverEntry>& instances, osg::Vec3f& chunkPosition)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mInstances(instances)
        , mChunkPosition(chunkPosition)
        {
        }

        void apply(osg::Node& node) override
        {
            osg::ref_ptr<osg::StateSet> ss = node.getStateSet();
            if (ss != nullptr)
            {
                ss->removeAttribute(osg::StateAttribute::MATERIAL);
                removeAlpha(ss);
            }

            traverse(node);
        }

        void apply(osg::Geometry& geom) override
        {
            for (unsigned int i = 0; i < geom.getNumPrimitiveSets(); ++i)
            {
                geom.getPrimitiveSet(i)->setNumInstances(mInstances.size());
            }

            osg::ref_ptr<osg::Vec4Array> transforms = new osg::Vec4Array(mInstances.size());
            osg::BoundingBox box;
            float radius = geom.getBoundingBox().radius();
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

            osg::ref_ptr<osg::Vec3Array> rotations = new osg::Vec3Array(mInstances.size());
            for (unsigned int i = 0; i < rotations->getNumElements(); i++)
            {
                (*rotations)[i] = mInstances[i].mPos.asRotationVec3();
            }

            // Display lists do not support instancing in OSG 3.4
            geom.setUseDisplayList(false);

            geom.setVertexAttribArray(6, transforms.get(), osg::Array::BIND_PER_VERTEX);
            geom.setVertexAttribArray(7, rotations.get(), osg::Array::BIND_PER_VERTEX);

            osg::ref_ptr<osg::StateSet> ss = geom.getOrCreateStateSet();
            ss->setAttribute(new osg::VertexAttribDivisor(6, 1));
            ss->setAttribute(new osg::VertexAttribDivisor(7, 1));

            ss->removeAttribute(osg::StateAttribute::MATERIAL);
            removeAlpha(ss);

            traverse(geom);
        }
    private:
        std::vector<Groundcover::GroundcoverEntry> mInstances;
        osg::Vec3f mChunkPosition;

        void removeAlpha(osg::StateSet* stateset)
        {
            // MGE uses default alpha settings for groundcover, so we can not rely on alpha properties
            stateset->removeAttribute(osg::StateAttribute::ALPHAFUNC);
            stateset->removeMode(GL_ALPHA_TEST);
            stateset->removeAttribute(osg::StateAttribute::BLENDFUNC);
            stateset->removeMode(GL_BLEND);
            stateset->setRenderBinToInherit();
        }
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
            if (mDensity >= 1.f) return true;

            mCurrentGroundcover += mDensity;
            if (mCurrentGroundcover < 1.f) return false;

            mCurrentGroundcover -= 1.f;

            return true;
        }
        void reset() { mCurrentGroundcover = 0.f; }

    private:
        float mCurrentGroundcover = 0.f;
        float mDensity = 0.f;
    };

    inline bool isInChunkBorders(ESM::CellRef& ref, osg::Vec2f& minBound, osg::Vec2f& maxBound)
    {
        osg::Vec2f size = maxBound - minBound;
        if (size.x() >=1 && size.y() >=1) return true;

        osg::Vec3f pos = ref.mPos.asVec3();
        osg::Vec3f cellPos = pos / ESM::Land::REAL_SIZE;
        if ((minBound.x() > std::floor(minBound.x()) && cellPos.x() < minBound.x()) || (minBound.y() > std::floor(minBound.y()) && cellPos.y() < minBound.y())
            || (maxBound.x() < std::ceil(maxBound.x()) && cellPos.x() >= maxBound.x()) || (minBound.y() < std::ceil(maxBound.y()) && cellPos.y() >= maxBound.y()))
            return false;

        return true;
    }

    osg::ref_ptr<osg::Node> Groundcover::getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile)
    {
        ChunkId id = std::make_tuple(center, size, activeGrid);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
        if (obj)
            return obj->asNode();
        else
        {
            InstanceMap instances;
            collectInstances(instances, size, center);
            osg::ref_ptr<osg::Node> node = createChunk(instances, center);
            mCache->addEntryToObjectCache(id, node.get());
            return node;
        }
    }

    Groundcover::Groundcover(Resource::SceneManager* sceneManager, float density)
         : GenericResourceManager<ChunkId>(nullptr)
         , mSceneManager(sceneManager)
         , mDensity(density)
    {
    }

    void Groundcover::collectInstances(InstanceMap& instances, float size, const osg::Vec2f& center)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        osg::Vec2f minBound = (center - osg::Vec2f(size/2.f, size/2.f));
        osg::Vec2f maxBound = (center + osg::Vec2f(size/2.f, size/2.f));
        DensityCalculator calculator(mDensity);
        std::vector<ESM::ESMReader> esm;
        osg::Vec2i startCell = osg::Vec2i(std::floor(center.x() - size/2.f), std::floor(center.y() - size/2.f));
        for (int cellX = startCell.x(); cellX < startCell.x() + size; ++cellX)
        {
            for (int cellY = startCell.y(); cellY < startCell.y() + size; ++cellY)
            {
                const ESM::Cell* cell = store.get<ESM::Cell>().searchStatic(cellX, cellY);
                if (!cell) continue;

                calculator.reset();
                for (size_t i=0; i<cell->mContextList.size(); ++i)
                {
                    unsigned int index = cell->mContextList[i].index;
                    if (esm.size() <= index)
                        esm.resize(index+1);
                    cell->restore(esm[index], i);
                    ESM::CellRef ref;
                    ref.mRefNum.mContentFile = ESM::RefNum::RefNum_NoContentFile;
                    bool deleted = false;
                    while(cell->getNextRef(esm[index], ref, deleted))
                    {
                        if (deleted) continue;
                        if (!ref.mRefNum.fromGroundcoverFile()) continue;

                        if (!calculator.isInstanceEnabled()) continue;
                        if (!isInChunkBorders(ref, minBound, maxBound)) continue;

                        Misc::StringUtils::lowerCaseInPlace(ref.mRefID);
                        int type = store.findStatic(ref.mRefID);
                        std::string model = getGroundcoverModel(type, ref.mRefID, store);
                        if (model.empty()) continue;
                        model = "meshes/" + model;

                        instances[model].emplace_back(std::move(ref), std::move(model));
                    }
                }
            }
        }
    }

    osg::ref_ptr<osg::Node> Groundcover::createChunk(InstanceMap& instances, const osg::Vec2f& center)
    {
        osg::ref_ptr<osg::Group> group = new osg::Group;
        osg::Vec3f worldCenter = osg::Vec3f(center.x(), center.y(), 0)*ESM::Land::REAL_SIZE;
        for (auto& pair : instances)
        {
            const osg::Node* temp = mSceneManager->getTemplate(pair.first);
            osg::ref_ptr<osg::Node> node = static_cast<osg::Node*>(temp->clone(osg::CopyOp::DEEP_COPY_ALL&(~osg::CopyOp::DEEP_COPY_TEXTURES)));

            // Keep link to original mesh to keep it in cache
            group->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(temp));

            mSceneManager->reinstateRemovedState(node);

            InstancingVisitor visitor(pair.second, worldCenter);
            node->accept(visitor);
            group->addChild(node);
        }

        // Force a unified alpha handling instead of data from meshes
        osg::ref_ptr<osg::AlphaFunc> alpha = new osg::AlphaFunc(osg::AlphaFunc::GEQUAL, 128.f / 255.f);
        group->getOrCreateStateSet()->setAttributeAndModes(alpha.get(), osg::StateAttribute::ON);
        group->getBound();
        group->setNodeMask(Mask_Groundcover);
        if (mSceneManager->getLightingMethod() != SceneUtil::LightingMethod::FFP)
            group->setCullCallback(new SceneUtil::LightListCallback);
        mSceneManager->recreateShaders(group, "groundcover", false, true);

        return group;
    }

    unsigned int Groundcover::getNodeMask()
    {
        return Mask_Groundcover;
    }

    void Groundcover::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        stats->setAttribute(frameNumber, "Groundcover Chunk", mCache->getCacheSize());
    }
}
