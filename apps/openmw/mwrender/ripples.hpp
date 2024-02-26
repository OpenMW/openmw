#ifndef OPENMW_MWRENDER_RIPPLES_H
#define OPENMW_MWRENDER_RIPPLES_H

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
    class Texture;
    class Texture2D;
    class FrameBufferObject;
}

namespace MWRender
{
    class RipplesSurface : public osg::Geometry
    {
    public:
        RipplesSurface(Resource::ResourceSystem* resourceSystem);

        osg::Texture* getColorTexture() const;

        void emit(const osg::Vec3f pos, float sizeInCellUnits);

        void drawImplementation(osg::RenderInfo& renderInfo) const override;

        void setPaused(bool paused) { mPaused = paused; }

        void traverse(osg::NodeVisitor& nv) override;

        void releaseGLObjects(osg::State* state) const override;

        static constexpr size_t sRTTSize = 1024;
        // e.g. texel to cell unit ratio
        static constexpr float sWorldScaleFactor = 2.5;

    private:
        struct State
        {
            bool mPaused = true;
            osg::ref_ptr<osg::StateSet> mStateset;
        };

        void setupFragmentPipeline();

        void setupComputePipeline();

        inline void updateState(const osg::FrameStamp& frameStamp, State& state);

        Resource::ResourceSystem* mResourceSystem;

        size_t mPositionCount = 0;
        std::array<osg::Vec3f, 100> mPositions;

        std::array<State, 2> mState;

        osg::Vec2f mCurrentPlayerPos;
        osg::Vec2f mLastPlayerPos;

        std::array<osg::ref_ptr<osg::Texture2D>, 2> mTextures;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mFBOs;

        osg::ref_ptr<osg::Program> mProgramBlobber;
        osg::ref_ptr<osg::Program> mProgramSimulation;

        bool mPaused = false;
        bool mUseCompute = false;

        double mLastSimulationTime = 0;
        double mRemainingWaveTime = 0;
    };

    class Ripples : public osg::Camera
    {
    public:
        Ripples(Resource::ResourceSystem* resourceSystem);

        osg::Texture* getColorTexture() const;

        void emit(const osg::Vec3f pos, float sizeInCellUnits);

        void setPaused(bool paused);

        osg::ref_ptr<RipplesSurface> mRipples;
    };
}

#endif
