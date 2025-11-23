#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H

#include <array>
#include <memory>
#include <set>
#include <unordered_map>

#include <osg/Group>
#include <osg/Light>
#include <osg/NodeVisitor>
#include <osg/observer_ptr>

#include <components/sceneutil/nodecallback.hpp>

#include "lightingmethod.hpp"

namespace SceneUtil
{
    class LightBuffer;
    struct StateSetGenerator;

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
        std::array<int, 2> mIndex;
        std::array<osg::ref_ptr<osg::Uniform>, 2> mUniformBuffers;
        std::array<osg::ref_ptr<osg::Uniform>, 2> mUniformCount;
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
        std::array<osg::ref_ptr<osg::Light>, 2> mLight;

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

    class UBOManager : public osg::StateAttribute
    {
    public:
        UBOManager(int lightCount = 1);
        UBOManager(const UBOManager& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        void releaseGLObjects(osg::State* state) const override;

        int compare(const StateAttribute& sa) const override;

        META_StateAttribute(SceneUtil, UBOManager, osg::StateAttribute::LIGHT)

        void apply(osg::State& state) const override;

        auto& getLightBuffer(size_t frameNum) { return mLightBuffers[frameNum % 2]; }

    private:
        std::string generateDummyShader(int maxLightsInScene);
        void initSharedLayout(osg::GLExtensions* ext, int handle, unsigned int frame) const;

        osg::ref_ptr<osg::Program> mDummyProgram;
        mutable bool mInitLayout;
        mutable std::array<osg::ref_ptr<LightBuffer>, 2> mLightBuffers;
        mutable std::array<bool, 2> mDirty;
        osg::ref_ptr<LightBuffer> mTemplate;
    };

    struct LightSettings
    {
        LightingMethod mLightingMethod = LightingMethod::FFP;
        int mMaxLights = 0;
        float mMaximumLightDistance = 0;
        float mLightFadeStart = 0;
        float mLightBoundsMultiplier = 0;
    };

    /// @brief Decorator node implementing the rendering of any number of LightSources that can be anywhere in the
    /// subgraph.
    class LightManager : public osg::Group
    {
    public:
        static LightingMethod getLightingMethodFromString(const std::string& value);
        /// Returns string as used in settings file, or the empty string if the method is undefined
        static std::string getLightingMethodString(LightingMethod method);

        struct LightSourceTransform
        {
            LightSource* mLightSource;
            osg::Matrixf mWorldMatrix;
        };

        struct LightSourceViewBound
        {
            LightSource* mLightSource;
            osg::BoundingSphere mViewBound;
        };

        using LightList = std::vector<const LightSourceViewBound*>;
        using SupportedMethods = std::array<bool, 3>;

        META_Node(SceneUtil, LightManager)

        explicit LightManager(const LightSettings& settings = LightSettings{});

        LightManager(const LightManager& copy, const osg::CopyOp& copyop);

        /// @param mask This mask is compared with the current Camera's cull mask to determine if lighting is desired.
        /// By default, it's ~0u i.e. always on.
        /// If you have some views that do not require lighting, then set the Camera's cull mask to not include
        /// the lightingMask for a much faster cull and rendering.
        void setLightingMask(size_t mask);
        size_t getLightingMask() const;

        /// Set the first light index that should be used by this manager, typically the number of directional lights in
        /// the scene.
        void setStartLight(int start);
        int getStartLight() const;

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

        bool usingFFP() const;

        LightingMethod getLightingMethod() const;

        int getMaxLights() const;

        int getMaxLightsInScene() const;

        auto& getDummies() { return mDummies; }

        auto& getLightIndexMap(size_t frameNum) { return mLightIndexMaps[frameNum % 2]; }

        auto& getUBOManager() { return mUBOManager; }

        osg::Matrixf getSunlightBuffer(size_t frameNum) const { return mSunlightBuffers[frameNum % 2]; }
        void setSunlightBuffer(const osg::Matrixf& buffer, size_t frameNum) { mSunlightBuffers[frameNum % 2] = buffer; }

        SupportedMethods getSupportedLightingMethods() { return mSupported; }

        std::map<std::string, std::string> getLightDefines() const;

        void processChangedSettings(float lightBoundsMultiplier, float maximumLightDistance, float lightFadeStart);

        /// Not thread safe, it is the responsibility of the caller to stop/start threading on the viewer
        void updateMaxLights(int maxLights);

        osg::ref_ptr<osg::Uniform> generateLightBufferUniform(const osg::Matrixf& sun);

        // Whether to collect main scene camera points lights into a buffer to be later sent to postprocessing shaders
        void setCollectPPLights(bool enabled);

        std::shared_ptr<PPLightBuffer> getPPLightsBuffer() { return mPPLightBuffer; }

    private:
        void initFFP(int targetLights);
        void initPerObjectUniform(int targetLights);
        void initSingleUBO(int targetLights);

        void updateSettings(float lightBoundsMultiplier, float maximumLightDistance, float lightFadeStart);

        void setLightingMethod(LightingMethod method);
        void setMaxLights(int value);

        void updateGPUPointLight(
            int index, LightSource* lightSource, size_t frameNum, const osg::RefMatrix* viewMatrix);

        std::vector<LightSourceTransform> mLights;

        using LightSourceViewBoundCollection = std::vector<LightSourceViewBound>;
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection> mLightsInViewSpace;

        using LightIdList = std::vector<int>;
        struct HashLightIdList
        {
            size_t operator()(const LightIdList&) const;
        };
        using LightStateSetMap = std::unordered_map<LightIdList, osg::ref_ptr<osg::StateSet>, HashLightIdList>;
        LightStateSetMap mStateSetCache[2];

        std::vector<osg::ref_ptr<osg::StateAttribute>> mDummies;

        int mStartLight;

        size_t mLightingMask;

        osg::ref_ptr<osg::Light> mSun;

        osg::Matrixf mSunlightBuffers[2];

        // < Light ID , Buffer Index >
        using LightIndexMap = std::unordered_map<int, int>;
        LightIndexMap mLightIndexMaps[2];

        std::unique_ptr<StateSetGenerator> mStateSetGenerator;

        osg::ref_ptr<UBOManager> mUBOManager;

        LightingMethod mLightingMethod;

        float mPointLightRadiusMultiplier;
        float mPointLightFadeEnd;
        float mPointLightFadeStart;

        int mMaxLights;

        SupportedMethods mSupported;

        std::shared_ptr<PPLightBuffer> mPPLightBuffer;
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

    void configureStateSetSunOverride(LightManager* lightManager, const osg::Light* light, osg::StateSet* stateset,
        int mode = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

}

#endif
