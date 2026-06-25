#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H

#include <array>
#include <memory>
#include <set>
#include <unordered_map>

#include <osg/BufferIndexBinding>
#include <osg/BufferTemplate>
#include <osg/DispatchCompute>
#include <osg/Group>
#include <osg/Light>
#include <osg/NodeVisitor>
#include <osg/observer_ptr>

#include <components/misc/constants.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sceneutil/clusteredlighting.hpp>
#include <components/sceneutil/nodecallback.hpp>

namespace SceneUtil
{
    template <class T>
    using DoubleBuffer = std::array<T, 2>;

    class PPLightBuffer
    {
    public:
        inline static constexpr auto sMaxPPLights = 40;
        inline static constexpr auto sMaxPPLightsArraySize = sMaxPPLights * 3;

        PPLightBuffer()
        {
            for (size_t i = 0; i < 2; ++i)
            {
                mIndex[i] = 0;
                mUniformBuffers[i]
                    = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "omw_PointLights", sMaxPPLightsArraySize);
                mUniformCount[i] = new osg::Uniform("omw_PointLightsCount", static_cast<int>(0));
            }
        }

        void applyUniforms(size_t frame, osg::StateSet* stateset)
        {
            size_t frameId = frame % 2;

            if (!stateset->getUniform("omw_PointLights"))
                stateset->addUniform(mUniformBuffers[frameId]);
            if (!stateset->getUniform("omw_PointLightsCount"))
                stateset->addUniform(mUniformCount[frameId]);

            mUniformBuffers[frameId]->dirty();
            mUniformCount[frameId]->dirty();
        }

        void clear(size_t frame) { mIndex[frame % 2] = 0; }

        void setLight(size_t frame, const osg::Light* light, float radius)
        {
            size_t frameId = frame % 2;
            int i = mIndex[frameId];

            if (i >= (sMaxPPLights - 1))
                return;

            i *= 3;

            mUniformBuffers[frameId]->setElement(i + 0, light->getPosition());
            mUniformBuffers[frameId]->setElement(i + 1, light->getDiffuse());
            mUniformBuffers[frameId]->setElement(i + 2,
                osg::Vec4f(light->getConstantAttenuation(), light->getLinearAttenuation(),
                    light->getQuadraticAttenuation(), radius));

            mIndex[frameId]++;
        }

        void updateCount(size_t frame)
        {
            size_t frameId = frame % 2;
            mUniformCount[frameId]->set(mIndex[frameId]);
        }

    private:
        DoubleBuffer<int> mIndex;
        DoubleBuffer<osg::ref_ptr<osg::Uniform>> mUniformBuffers;
        DoubleBuffer<osg::ref_ptr<osg::Uniform>> mUniformCount;
    };

    /// LightSource managed by a LightManager.
    /// @par Typically used for point lights. Spot lights are not supported yet. Directional lights affect the whole
    /// scene
    ///     so do not need to be managed by a LightManager - so for directional lights use a plain osg::LightSource
    ///     instead.
    /// @note LightSources must be decorated by a LightManager node in order to have an effect. Typical use would
    ///     be one LightManager as the root of the scene graph.
    /// @note One needs to attach LightListCallback's to the scene to have objects receive lighting from LightSources.
    ///     See the documentation of LightListCallback for more information.
    /// @note The position of the contained osg::Light is automatically updated based on the LightSource's world
    /// position.
    class LightSource : public osg::Node
    {
        // double buffered osg::Light's, since one of them may be in use by the draw thread at any given time
        DoubleBuffer<osg::ref_ptr<osg::Light>> mLight;

        // LightSource will affect objects within this radius
        float mRadius;

        int mId;

        float mActorFade;

        size_t mLastAppliedFrame;

        bool mEmpty = false;

    public:
        META_Node(SceneUtil, LightSource)

        LightSource();

        LightSource(const LightSource& copy, const osg::CopyOp& copyop);

        float getRadius() const { return mRadius; }

        /// The LightSource will affect objects within this radius.
        void setRadius(float radius) { mRadius = radius; }

        void setActorFade(float alpha) { mActorFade = alpha; }

        float getActorFade() const { return mActorFade; }

        void setEmpty(bool empty) { mEmpty = empty; }

        bool getEmpty() const { return mEmpty; }

        /// Get the osg::Light safe for modification in the given frame.
        /// @par May be used externally to animate the light's color/attenuation properties,
        /// and is used internally to synchronize the light's position with the position of the LightSource.
        osg::Light* getLight(size_t frame) { return mLight[frame % 2]; }

        /// @warning It is recommended not to replace an existing osg::Light, because there might still be
        /// references to it in the light StateSet cache that are associated with this LightSource's ID.
        /// These references will stay valid due to ref_ptr but will point to the old object.
        /// @warning Do not modify the \a light after you've called this function.
        void setLight(osg::Light* light)
        {
            mLight[0] = light;
            mLight[1] = new osg::Light(*light);
        }

        /// Get the unique ID for this light source.
        int getId() const { return mId; }

        void setLastAppliedFrame(size_t lastAppliedFrame) { mLastAppliedFrame = lastAppliedFrame; }

        size_t getLastAppliedFrame() const { return mLastAppliedFrame; }
    };

    struct LightSettings
    {
        bool mClusteredLighting = false;
        int mMaxLights = 8;
        float mMaximumLightDistance = Constants::CellSizeInUnits;
        float mLightFadeStart = 0;
        float mLightRadiusMultiplier = 1;
        osg::Vec3i mClusteredGridSize = { 16, 8, 24 };
        int mClusteredWorkGroupSize = 512;
    };

    class LightManagerCullCallback;

    /// @brief Decorator node implementing the rendering of any number of LightSources that can be anywhere in the
    /// subgraph.
    class LightManager : public osg::Group
    {
    public:
        struct LightSourceTransform
        {
            LightSource* mLightSource;
            osg::Matrixf mWorldMatrix;
        };

        struct LightSourceViewBound
        {
            LightSource* mLightSource;
            osg::BoundingSphere mViewBound;
            bool mCulled = false;
        };

        using LightList = std::vector<const LightSourceViewBound*>;
        using SupportedMethods = std::array<bool, 3>;

        META_Node(SceneUtil, LightManager)

        explicit LightManager(
            const LightSettings& settings = LightSettings{}, Resource::ResourceSystem* resourceSystem = nullptr);

        LightManager(const LightManager& copy, const osg::CopyOp& copyop);

        /// @param mask This mask is compared with the current Camera's cull mask to determine if lighting is desired.
        /// By default, it's ~0u i.e. always on.
        /// If you have some views that do not require lighting, then set the Camera's cull mask to not include
        /// the lightingMask for a much faster cull and rendering.
        void setLightingMask(size_t mask);
        size_t getLightingMask() const;

        /// Internal use only, called automatically by the LightManager's UpdateCallback
        void update(size_t frameNum);

        /// Internal use only, called automatically by the LightSource's UpdateCallback
        void addLight(LightSource* lightSource, const osg::Matrixf& worldMat, size_t frameNum);

        const std::vector<LightSourceViewBound>& getLightsInViewSpace(
            osgUtil::CullVisitor* cv, const osg::RefMatrix* viewMatrix, size_t frameNum);

        osg::ref_ptr<osg::StateSet> getLightListStateSet(
            const LightList& lightList, size_t frameNum, const osg::RefMatrix* viewMatrix);

        void setSunlight(osg::ref_ptr<osg::Light> sun);
        osg::ref_ptr<osg::Light> getSunlight();

        bool getClusteredLighting() const;

        int getMaxLights() const;

        bool isClusteredSupported() { return mSupportsClustered; }

        std::map<std::string, std::string> getLightDefines() const;

        void processChangedSettings(float lightBoundsMultiplier, float maximumLightDistance, float lightFadeStart);

        /// Not thread safe, it is the responsibility of the caller to stop/start threading on the viewer
        void updateMaxLights(int maxLights);

        osg::ref_ptr<osg::Uniform> generateLightBufferUniform();

        // Whether to collect main scene camera points lights into a buffer to be later sent to postprocessing shaders
        void setCollectPPLights(bool enabled);

        std::shared_ptr<PPLightBuffer> getPPLightsBuffer() { return mPPLightBuffer; }

        float getPointLightRadiusMultiplier() const { return mPointLightRadiusMultiplier; }

        float getPointLightFadeEnd() const { return mPointLightFadeEnd; }

        void enableClustered(bool enabled);

        Resource::ResourceSystem* getResourceSystem() { return mResourceSystem; }

    private:
        void initPerObjectUniform(int targetLights);
        void initClustered();

        void updateSettings(float lightBoundsMultiplier, float maximumLightDistance, float lightFadeStart);

        void setMaxLights(int value);

        Resource::ResourceSystem* mResourceSystem;

        std::vector<LightSourceTransform> mLights;

        using LightSourceViewBoundCollection = std::vector<LightSourceViewBound>;
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection> mLightsInViewSpace;

        size_t mLightingMask;

        osg::ref_ptr<osg::Light> mSun;

        bool mClusteredLighting;

        float mPointLightRadiusMultiplier;
        float mPointLightFadeEnd;
        float mPointLightFadeStart;

        int mMaxLights;

        bool mSupportsClustered;

        std::shared_ptr<PPLightBuffer> mPPLightBuffer;

        osg::ref_ptr<LightManagerCullCallback> mCullCallback;
    };

    class LightManagerCullCallback
        : public SceneUtil::NodeCallback<LightManagerCullCallback, LightManager*, osgUtil::CullVisitor*>
    {
    public:
        LightManagerCullCallback(const LightSettings& settings = LightSettings{});

        void operator()(LightManager* node, osgUtil::CullVisitor* cv);

        void reset() { mCache.clear(); }

        struct ViewData
        {
            DoubleBuffer<osg::Matrixd> mProjection;
            DoubleBuffer<osg::ref_ptr<osg::BufferTemplate<std::vector<PointLight>>>> mGPULights;
            DoubleBuffer<osg::ref_ptr<osg::StateSet>> mStateSet;
            DoubleBuffer<osg::ref_ptr<osg::ShaderStorageBufferBinding>> mPointLightSSBB;
            DoubleBuffer<osg::ref_ptr<osg::ShaderStorageBufferBinding>> mLightGridSSBB;
            DoubleBuffer<osg::ref_ptr<osg::ShaderStorageBufferBinding>> mLightIndexListSSBB;
            DoubleBuffer<osg::ref_ptr<osg::ShaderStorageBufferBinding>> mLightIndexCounterSSBB;
            DoubleBuffer<osg::ref_ptr<osg::DispatchCompute>> mClusterComputeNode;
            DoubleBuffer<osg::ref_ptr<osg::DispatchCompute>> mCullComputeNode;
            osg::ref_ptr<osg::ShaderStorageBufferBinding> mClusterSSBB;

            float clusterFar = 1.f;
            size_t mLastFrameNumber = 0;
        };

        std::unordered_map<osg::Camera*, ViewData> mCache;

        const int mGridSizeX;
        const int mGridSizeY;
        const int mGridSizeZ;
        const int mNumClusters;
        const int mWorkGroupSize;

        const int mMaxLightsPerCluster = 512;
    };

    /// To receive lighting, objects must be decorated by a LightListCallback. Light list callbacks must be added via
    /// node->addCullCallback(new LightListCallback). Once a light list callback is added to a node, that node and all
    /// its child nodes can receive lighting.
    /// @par The placement of these LightListCallbacks affects the granularity of light lists. Having too fine grained
    /// light lists can result in degraded performance. Too coarse grained light lists can result in lights no longer
    /// rendering when the size of a light list exceeds the OpenGL limit on the number of concurrent lights (8). A good
    /// starting point is to attach a LightListCallback to each game object's base node.
    /// @note Not thread safe for CullThreadPerCamera threading mode.
    /// @note Due to lack of OSG support, the callback does not work on Drawables.
    class LightListCallback : public SceneUtil::NodeCallback<LightListCallback, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        LightListCallback()
            : mLightManager(nullptr)
            , mLastFrameNumber(0)
        {
        }
        LightListCallback(const LightListCallback& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , SceneUtil::NodeCallback<LightListCallback, osg::Node*, osgUtil::CullVisitor*>(copy, copyop)
            , mLightManager(copy.mLightManager)
            , mLastFrameNumber(0)
            , mIgnoredLightSources(copy.mIgnoredLightSources)
        {
        }

        META_Object(SceneUtil, LightListCallback)

        void operator()(osg::Node* node, osgUtil::CullVisitor* nv);

        bool pushLightState(osg::Node* node, osgUtil::CullVisitor* nv);

        std::set<SceneUtil::LightSource*>& getIgnoredLightSources() { return mIgnoredLightSources; }

    private:
        LightManager* mLightManager;
        size_t mLastFrameNumber;
        LightManager::LightList mLightList;
        std::set<SceneUtil::LightSource*> mIgnoredLightSources;
    };

    void configureStateSetSunOverride(const osg::Light* light, osg::StateSet* stateset,
        int mode = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    void configureSunAmbientOverride(const osg::Vec4f& ambient, osg::StateSet* stateset);
}

#endif
