#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <array>

#include <osg/Light>

#include <osg/Group>
#include <osg/NodeVisitor>
#include <osg/observer_ptr>

#include <components/shader/shadermanager.hpp>

#include <components/settings/settings.hpp>
#include <components/sceneutil/nodecallback.hpp>

namespace osgUtil
{
    class CullVisitor;
}

namespace SceneUtil
{
    class LightBuffer;
    struct StateSetGenerator;

    enum class LightingMethod
    {
        FFP,
        PerObjectUniform,
        SingleUBO,
    };

    /// LightSource managed by a LightManager.
    /// @par Typically used for point lights. Spot lights are not supported yet. Directional lights affect the whole scene
    ///     so do not need to be managed by a LightManager - so for directional lights use a plain osg::LightSource instead.
    /// @note LightSources must be decorated by a LightManager node in order to have an effect. Typical use would
    ///     be one LightManager as the root of the scene graph.
    /// @note One needs to attach LightListCallback's to the scene to have objects receive lighting from LightSources.
    ///     See the documentation of LightListCallback for more information.
    /// @note The position of the contained osg::Light is automatically updated based on the LightSource's world position.
    class LightSource : public osg::Node
    {
        // double buffered osg::Light's, since one of them may be in use by the draw thread at any given time
        osg::ref_ptr<osg::Light> mLight[2];

        // LightSource will affect objects within this radius
        float mRadius;

        int mId;

        float mActorFade;

    public:

        META_Node(SceneUtil, LightSource)

        LightSource();

        LightSource(const LightSource& copy, const osg::CopyOp& copyop);

        float getRadius() const
        {
            return mRadius;
        }

        /// The LightSource will affect objects within this radius.
        void setRadius(float radius)
        {
            mRadius = radius;
        }

        void setActorFade(float alpha)
        {
            mActorFade = alpha;
        }

        float getActorFade() const
        {
            return mActorFade;
        }

        /// Get the osg::Light safe for modification in the given frame.
        /// @par May be used externally to animate the light's color/attenuation properties,
        /// and is used internally to synchronize the light's position with the position of the LightSource.
        osg::Light* getLight(size_t frame)
        {
            return mLight[frame % 2];
        }

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
        int getId() const
        {
            return mId;
        }
    };

    /// @brief Decorator node implementing the rendering of any number of LightSources that can be anywhere in the subgraph.
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

        LightManager(bool ffp = true);

        LightManager(const LightManager& copy, const osg::CopyOp& copyop);

        ~LightManager();

        /// @param mask This mask is compared with the current Camera's cull mask to determine if lighting is desired.
        /// By default, it's ~0u i.e. always on.
        /// If you have some views that do not require lighting, then set the Camera's cull mask to not include
        /// the lightingMask for a much faster cull and rendering.
        void setLightingMask(size_t mask);
        size_t getLightingMask() const;

        /// Set the first light index that should be used by this manager, typically the number of directional lights in the scene.
        void setStartLight(int start);
        int getStartLight() const;

        /// Internal use only, called automatically by the LightManager's UpdateCallback
        void update(size_t frameNum);

        /// Internal use only, called automatically by the LightSource's UpdateCallback
        void addLight(LightSource* lightSource, const osg::Matrixf& worldMat, size_t frameNum);

        const std::vector<LightSourceViewBound>& getLightsInViewSpace(osgUtil::CullVisitor* cv, const osg::RefMatrix* viewMatrix, size_t frameNum);

        osg::ref_ptr<osg::StateSet> getLightListStateSet(const LightList& lightList, size_t frameNum, const osg::RefMatrix* viewMatrix);

        void setSunlight(osg::ref_ptr<osg::Light> sun);
        osg::ref_ptr<osg::Light> getSunlight();

        bool usingFFP() const;

        LightingMethod getLightingMethod() const;

        int getMaxLights() const;

        int getMaxLightsInScene() const;

        auto& getDummies() { return mDummies; }

        auto& getLightIndexMap(size_t frameNum) { return mLightIndexMaps[frameNum%2]; }

        auto& getLightBuffer(size_t frameNum) { return mLightBuffers[frameNum%2]; }

        osg::Matrixf getSunlightBuffer(size_t frameNum) const { return mSunlightBuffers[frameNum%2]; }
        void setSunlightBuffer(const osg::Matrixf& buffer, size_t frameNum) { mSunlightBuffers[frameNum%2] = buffer; }

        SupportedMethods getSupportedLightingMethods() { return mSupported; }

        std::map<std::string, std::string> getLightDefines() const;

        void processChangedSettings(const Settings::CategorySettingVector& changed);

        /// Not thread safe, it is the responsibility of the caller to stop/start threading on the viewer
        void updateMaxLights();

    private:
        void initFFP(int targetLights);
        void initPerObjectUniform(int targetLights);
        void initSingleUBO(int targetLights);

        void updateSettings();

        void setLightingMethod(LightingMethod method);
        void setMaxLights(int value);

        void updateGPUPointLight(int index, LightSource* lightSource, size_t frameNum, const osg::RefMatrix* viewMatrix);

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

        osg::ref_ptr<LightBuffer> mLightBuffers[2];

        osg::Matrixf mSunlightBuffers[2];

        // < Light ID , Buffer Index >
        using LightIndexMap = std::unordered_map<int, int>;
        LightIndexMap mLightIndexMaps[2];

        std::unique_ptr<StateSetGenerator> mStateSetGenerator;

        LightingMethod mLightingMethod;

        float mPointLightRadiusMultiplier;
        float mPointLightFadeEnd;
        float mPointLightFadeStart;

        int mMaxLights;

        SupportedMethods mSupported;

        static constexpr auto mMaxLightsLowerLimit = 2;
        static constexpr auto mMaxLightsUpperLimit = 64;
        static constexpr auto mFFPMaxLights = 8;

        static const std::unordered_map<std::string, LightingMethod> mLightingMethodSettingMap;
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
        {}
        LightListCallback(const LightListCallback& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop), SceneUtil::NodeCallback<LightListCallback, osg::Node*, osgUtil::CullVisitor*>(copy, copyop)
            , mLightManager(copy.mLightManager)
            , mLastFrameNumber(0)
            , mIgnoredLightSources(copy.mIgnoredLightSources)
        {}

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

    void configureStateSetSunOverride(LightManager* lightManager, const osg::Light* light, osg::StateSet* stateset, int mode = osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

}

#endif
