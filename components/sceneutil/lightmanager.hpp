#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTMANAGER_H

#include <set>

#include <osg/Light>

#include <osg/Group>
#include <osg/NodeVisitor>
#include <osg/observer_ptr>

namespace osgUtil
{
    class CullVisitor;
}

namespace SceneUtil
{

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

        /// Get the osg::Light safe for modification in the given frame.
        /// @par May be used externally to animate the light's color/attenuation properties,
        /// and is used internally to synchronize the light's position with the position of the LightSource.
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

        META_Node(SceneUtil, LightManager)

        LightManager();

        LightManager(const LightManager& copy, const osg::CopyOp& copyop);

        /// @param mask This mask is compared with the current Camera's cull mask to determine if lighting is desired.
        /// By default, it's ~0u i.e. always on.
        /// If you have some views that do not require lighting, then set the Camera's cull mask to not include
        /// the lightingMask for a much faster cull and rendering.
        void setLightingMask (unsigned int mask);

        unsigned int getLightingMask() const;

        /// Set the first light index that should be used by this manager, typically the number of directional lights in the scene.
        void setStartLight(int start);

        int getStartLight() const;

        /// Internal use only, called automatically by the LightManager's UpdateCallback
        void update();

        /// Internal use only, called automatically by the LightSource's UpdateCallback
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

    private:
        // Lights collected from the scene graph. Only valid during the cull traversal.
        std::vector<LightSourceTransform> mLights;

        typedef std::vector<LightSourceViewBound> LightSourceViewBoundCollection;
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection> mLightsInViewSpace;

        // < Light list hash , StateSet >
        typedef std::map<size_t, osg::ref_ptr<osg::StateSet> > LightStateSetMap;
        LightStateSetMap mStateSetCache[2];

        std::vector<osg::ref_ptr<osg::StateAttribute>> mDummies;

        int mStartLight;

        unsigned int mLightingMask;
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
    class LightListCallback : public osg::NodeCallback
    {
    public:
        LightListCallback()
            : mLightManager(nullptr)
            , mLastFrameNumber(0)
        {}
        LightListCallback(const LightListCallback& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop), osg::NodeCallback(copy, copyop)
            , mLightManager(copy.mLightManager)
            , mLastFrameNumber(0)
            , mIgnoredLightSources(copy.mIgnoredLightSources)
        {}

        META_Object(SceneUtil, LightListCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv);

        bool pushLightState(osg::Node* node, osgUtil::CullVisitor* nv);

        std::set<SceneUtil::LightSource*>& getIgnoredLightSources() { return mIgnoredLightSources; }

    private:
        LightManager* mLightManager;
        unsigned int mLastFrameNumber;
        LightManager::LightList mLightList;
        std::set<SceneUtil::LightSource*> mIgnoredLightSources;
    };

}

#endif
