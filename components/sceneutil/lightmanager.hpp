#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H

#include <osg/Light>

#include <osg/Group>
#include <osg/NodeVisitor>

namespace SceneUtil
{

    /// LightSource managed by a LightManager.
    class LightSource : public osg::Node
    {
        osg::ref_ptr<osg::Light> mLight;

        // The activation radius
        float mRadius;

        int mId;

    public:

        META_Node(SceneUtil, SceneUtil::LightSource)

        LightSource();

        LightSource(const LightSource& copy, const osg::CopyOp& copyop);

        float getRadius() const
        {
            return mRadius;
        }

        void setRadius(float radius)
        {
            mRadius = radius;
        }

        osg::Light* getLight()
        {
            return mLight;
        }

        void setLight(osg::Light* light)
        {
            mLight = light;
        }

        int getId()
        {
            return mId;
        }
    };

    /// All light sources must be a child of the LightManager node. The LightManager can be anywhere in the scene graph,
    /// but would be typically somewhere near the top.
    class LightManager : public osg::Group
    {
    public:

        META_Node(SceneUtil, SceneUtil::LightManager)

        LightManager();

        LightManager(const LightManager& copy, const osg::CopyOp& copyop);

        /// @param mask This mask is compared with the current Camera's cull mask to determine if lighting is desired.
        /// By default, it's ~0u i.e. always on.
        /// If you have some views that do not require lighting, then set the Camera's cull mask to not include
        /// the lightingMask for a much faster cull and rendering.
        void setLightingMask (unsigned int mask);

        unsigned int getLightingMask() const;

        // Called automatically by the UpdateCallback
        void update();

        // Called automatically by the LightSource's UpdateCallback
        void addLight(LightSource* lightSource, osg::Matrix worldMat);

        struct LightSourceTransform
        {
            LightSource* mLightSource;
            osg::Matrix mWorldMatrix;
        };

        const std::vector<LightSourceTransform>& getLights() const;

        struct LightSourceViewBound
        {
            LightSource* mLightSource;
            osg::BoundingSphere mViewBound;
        };

        const std::vector<LightSourceViewBound>& getLightsInViewSpace(osg::Camera* camera, const osg::RefMatrix* viewMatrix);

        typedef std::vector<const LightSourceViewBound*> LightList;

        osg::ref_ptr<osg::StateSet> getLightListStateSet(const LightList& lightList);

        /// Set the first light index that should be used by this manager, typically the number of directional lights in the scene.
        void setStartLight(int start);

        int getStartLight() const;

    private:
        // Lights collected from the scene graph. Only valid during the cull traversal.
        std::vector<LightSourceTransform> mLights;

        typedef std::vector<LightSourceViewBound> LightSourceViewBoundCollection;
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection> mLightsInViewSpace;

        // < Light list hash , StateSet >
        typedef std::map<size_t, osg::ref_ptr<osg::StateSet> > LightStateSetMap;
        LightStateSetMap mStateSetCache;

        int mStartLight;

        unsigned int mLightingMask;
    };

    /// @note Not thread safe for CullThreadPerCamera threading mode.
    class LightListCallback : public osg::NodeCallback
    {
    public:
        LightListCallback()
            : mLightManager(NULL)
            , mLastFrameNumber(0)
        {}
        LightListCallback(const LightListCallback& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop), osg::NodeCallback(copy, copyop)
            , mLightManager(copy.mLightManager)
            , mLastFrameNumber(0)
        {}

        META_Object(NifOsg, LightListCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        LightManager* mLightManager;
        unsigned int mLastFrameNumber;
        LightManager::LightList mLightList;
    };

    /// @brief Configures a light's attenuation according to vanilla Morrowind attenuation settings.
    void configureLight(osg::Light* light, float radius, bool isExterior, bool outQuadInLin, bool useQuadratic, float quadraticValue,
                         float quadraticRadiusMult, bool useLinear, float linearRadiusMult, float linearValue);

}

#endif
