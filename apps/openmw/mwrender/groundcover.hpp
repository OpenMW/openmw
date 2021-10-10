#ifndef OPENMW_MWRENDER_GROUNDCOVER_H
#define OPENMW_MWRENDER_GROUNDCOVER_H

#include <components/terrain/quadtreeworld.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/esm/loadcell.hpp>
#include <osg/Program>

namespace MWRender
{
    typedef std::tuple<osg::Vec2f, float> GroundcoverChunkId; // Center, Size
    class Groundcover : public Resource::GenericResourceManager<GroundcoverChunkId>, public Terrain::QuadTreeWorld::ChunkManager
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

            GroundcoverEntry(const ESM::CellRef& ref) : mPos(ref.mPos), mScale(ref.mScale)
            {}
        };

    private:
        Resource::SceneManager* mSceneManager;
        float mDensity;
        osg::ref_ptr<osg::StateSet> mStateset;
        osg::ref_ptr<osg::Program> mProgramTemplate;

        typedef std::map<std::string, std::vector<GroundcoverEntry>> InstanceMap;
        osg::ref_ptr<osg::Node> createChunk(InstanceMap& instances, const osg::Vec2f& center);
        void collectInstances(InstanceMap& instances, float size, const osg::Vec2f& center);
    };
}

#endif
