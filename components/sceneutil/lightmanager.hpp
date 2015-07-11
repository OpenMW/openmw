#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H

#include <osg/Light>

#include <osg/Group>
#include <osg/NodeVisitor>

namespace SceneUtil
{

    // This mask should be included in the Cull and Update visitor's traversal mask if lighting is desired.
    const int Mask_Lit = (1<<16);

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

        // Called automatically by the UpdateCallback
        void update();

        // Called automatically by the LightSource's UpdateCallback
        void addLight(LightSource* lightSource, osg::Matrix worldMat);

        void prepareForCamera(osg::Camera* cam);

        struct LightSourceTransform
        {
            LightSource* mLightSource;
            osg::Matrix mWorldMatrix;
            osg::BoundingSphere mViewBound;
        };

        const std::vector<LightSourceTransform>& getLights() const;

        typedef std::vector<const LightSourceTransform*> LightList;

        osg::ref_ptr<osg::StateSet> getLightListStateSet(const LightList& lightList);

        /// Set the first light index that should be used by this manager, typically the number of directional lights in the scene.
        void setStartLight(int start);

        int getStartLight() const;

    private:
        // Lights collected from the scene graph. Only valid during the cull traversal.
        std::vector<LightSourceTransform> mLights;

        bool mLightsInViewSpace;

        // < Light list hash , StateSet >
        typedef std::map<size_t, osg::ref_ptr<osg::StateSet> > LightStateSetMap;
        LightStateSetMap mStateSetCache;

        int mStartLight;
    };

    class LightListCallback : public osg::NodeCallback
    {
    public:
        LightListCallback()
            : mLightManager(nullptr)
        {}
        LightListCallback(const LightListCallback& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(copy, copyop), osg::NodeCallback(copy, copyop), mLightManager(copy.mLightManager)
        {}

        META_Object(NifOsg, LightListCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        LightManager* mLightManager;
    };

    /// @brief Configures a light's attenuation according to vanilla Morrowind attenuation settings.
    void configureLight(osg::Light* light, float radius, bool isExterior, bool outQuadInLin, bool useQuadratic, float quadraticValue,
                         float quadraticRadiusMult, bool useLinear, float linearRadiusMult, float linearValue);

}

#endif
