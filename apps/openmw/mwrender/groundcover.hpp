#ifndef OPENMW_MWRENDER_GROUNDCOVER_H
#define OPENMW_MWRENDER_GROUNDCOVER_H

#include <components/terrain/quadtreeworld.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/esm/loadcell.hpp>

namespace MWRender
{
    class GroundcoverUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        GroundcoverUpdater()
            : mWindSpeed(0.f)
            , mPlayerPos(osg::Vec3f())
        {
        }

        void setWindSpeed(float windSpeed);
        void setPlayerPos(osg::Vec3f playerPos);

    protected:
        void setDefaults(osg::StateSet *stateset) override;
        void apply(osg::StateSet *stateset, osg::NodeVisitor *nv) override;

    private:
        float mWindSpeed;
        osg::Vec3f mPlayerPos;
    };

    typedef std::tuple<osg::Vec2f, float, bool> ChunkId; // Center, Size, ActiveGrid
    class Groundcover : public Resource::GenericResourceManager<ChunkId>, public Terrain::QuadTreeWorld::ChunkManager
    {
    public:
        Groundcover(Resource::SceneManager* sceneManager, float density);
        ~Groundcover() = default;

        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile) override;

        unsigned int getNodeMask() override;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

        struct GroundcoverEntry
        {
            ESM::Position mPos;
            float mScale;
            std::string mModel;

            GroundcoverEntry(const ESM::CellRef& ref, const std::string& model):
                mPos(ref.mPos), mScale(ref.mScale), mModel(model)
            {}
        };

    private:
        Resource::SceneManager* mSceneManager;
        float mDensity;

        typedef std::map<std::string, std::vector<GroundcoverEntry>> InstanceMap;
        osg::ref_ptr<osg::Node> createChunk(InstanceMap& instances, const osg::Vec2f& center);
        void collectInstances(InstanceMap& instances, float size, const osg::Vec2f& center);
    };
}

#endif
