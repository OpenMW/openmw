#ifndef OPENMW_MWRENDER_TERRAINDEFORMATION_H
#define OPENMW_MWRENDER_TERRAINDEFORMATION_H

#include <array>

#include <osg/Camera>
#include <osg/Geometry>

#include <components/sceneutil/rtt.hpp>
#include <components/sceneutil/statesetupdater.hpp>

namespace Resource
{
    class ResourceSystem;
}

namespace osg
{
    class Camera;
    class Geometry;
    class Program;
    class Texture;
    class StateSet;
    class NodeVisitor;
    class Texture2D;
    class FrameBufferObject;
}

namespace MWRender
{
    enum TerrainMaterialType
    {
        TERRAIN_NORMAL = 0,
        TERRAIN_SNOW = 1,
        TERRAIN_SAND = 2,
        TERRAIN_ASH = 3
    };

    class TerrainDeformationSurface : public osg::Geometry
    {
    public:
        TerrainDeformationSurface(Resource::ResourceSystem* resourceSystem);

        osg::Texture* getColorTexture() const;

        void emit(const osg::Vec3f pos, float sizeInCellUnits, TerrainMaterialType materialType);

        void drawImplementation(osg::RenderInfo& renderInfo) const override;

        void setPaused(bool paused) { mPaused = paused; }

        void traverse(osg::NodeVisitor& nv) override;

        void releaseGLObjects(osg::State* state) const override;

        static constexpr size_t sRTTSize = 1024;
        // Texel to cell unit ratio (determines coverage area)
        static constexpr float sWorldScaleFactor = 2.5;

    private:
        struct State
        {
            bool mPaused = true;
            osg::ref_ptr<osg::StateSet> mStateset;
        };

        struct DeformationPoint
        {
            osg::Vec3f position;
            TerrainMaterialType materialType;
        };

        void setupFragmentPipeline();

        void setupComputePipeline();

        inline void updateState(const osg::FrameStamp& frameStamp, State& state);

        Resource::ResourceSystem* mResourceSystem;

        size_t mPositionCount = 0;
        std::array<DeformationPoint, 100> mDeformationPoints;

        std::array<State, 2> mState;

        osg::Vec2f mCurrentPlayerPos;
        osg::Vec2f mLastPlayerPos;

        std::array<osg::ref_ptr<osg::Texture2D>, 2> mTextures;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mFBOs;

        osg::ref_ptr<osg::Program> mProgramStamp;
        osg::ref_ptr<osg::Program> mProgramDecay;

        bool mPaused = false;
        bool mUseCompute = false;

        double mLastSimulationTime = 0;
        double mRemainingDecayTime = 0;
    };

    class TerrainDeformation : public osg::Camera
    {
    public:
        TerrainDeformation(Resource::ResourceSystem* resourceSystem);

        osg::Texture* getColorTexture() const;

        void emit(const osg::Vec3f pos, float sizeInCellUnits, TerrainMaterialType materialType);

        void setPaused(bool paused);

        osg::ref_ptr<TerrainDeformationSurface> mDeformationSurface;
    };
}

#endif
