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
        osg::ref_ptr<osg::Light> mLight[2];

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

        /// Get the osg::Light safe for modification in the given frame.
        osg::Light* getLight(unsigned int frame)
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
            mLight[1] = osg::clone(light);
        }

        /// Get the unique ID for this light source.
        int getId() const
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
        void addLight(LightSource* lightSource, const osg::Matrixf& worldMat, unsigned int frameNum);

        struct LightSourceTransform
        {
            LightSource* mLightSource;
            osg::Matrixf mWorldMatrix;
        };

        const std::vector<LightSourceTransform>& getLights() const;

        struct LightSourceViewBound
        {
            LightSource* mLightSource;
            osg::BoundingSphere mViewBound;
        };

        const std::vector<LightSourceViewBound>& getLightsInViewSpace(osg::Camera* camera, const osg::RefMatrix* viewMatrix);

        typedef std::vector<const LightSourceViewBound*> LightList;

        osg::ref_ptr<osg::StateSet> getLightListStateSet(const LightList& lightList, unsigned int frameNum);

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
        LightStateSetMap mStateSetCache[2];

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
