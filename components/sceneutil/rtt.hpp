#ifndef OPENMW_RTT_H
#define OPENMW_RTT_H

#include <osg/Node>

#include <map>
#include <memory>

namespace osg
{
    class Texture2D;
    class Camera;
}

namespace osgUtil
{
    class CullVisitor;
}

namespace SceneUtil
{
    /// @brief Implements per-view RTT operations.
    /// @par With a naive RTT implementation, subsequent views of multiple views will overwrite the results of the previous views, leading to
    ///     the results of the last view being broadcast to all views. An error in all cases where the RTT result depends on the view.
    /// @par If using an RTTNode this is solved by mapping RTT operations to CullVisitors, which will be unique per view. This requires
    ///     instancing one camera per view, and traversing only the camera mapped to that CV during cull traversals.
    /// @par Camera settings should be effectuated by overriding the setDefaults() and apply() methods, following a pattern similar to SceneUtil::StateSetUpdater
    /// @par When using the RTT texture in your statesets, it is recommended to use SceneUtil::StateSetUpdater as a cull callback to handle this as the appropriate
    ///     textures can be retrieved during SceneUtil::StateSetUpdater::Apply()
    /// @par For any of COLOR_BUFFER or DEPTH_BUFFER not added during setDefaults(), RTTNode will attach a default buffer. The default color buffer has an internal format of GL_RGB.
    ///     The default depth buffer has internal format GL_DEPTH_COMPONENT24, source format GL_DEPTH_COMPONENT, and source type GL_UNSIGNED_INT. Default wrap is CLAMP_TO_EDGE and filter LINEAR.
    class RTTNode : public osg::Node
    {
    public:
        RTTNode(uint32_t textureWidth, uint32_t textureHeight, int renderOrderNum, bool doPerViewMapping);
        ~RTTNode();

        osg::Texture* getColorTexture(osgUtil::CullVisitor* cv);

        osg::Texture* getDepthTexture(osgUtil::CullVisitor* cv);


        /// Apply state - to override in derived classes
        /// @note Due to the view mapping approach you *have* to apply all camera settings, even if they have not changed since the last frame.
        virtual void setDefaults(osg::Camera* camera) {};

        /// Set default settings - optionally override in derived classes
        virtual void apply(osg::Camera* camera) {};

        void cull(osgUtil::CullVisitor* cv);

    private:
        struct ViewDependentData
        {
            osg::ref_ptr<osg::Camera> mCamera;
        };

        ViewDependentData* getViewDependentData(osgUtil::CullVisitor* cv);

        typedef std::map< osgUtil::CullVisitor*, std::unique_ptr<ViewDependentData> >  ViewDependentDataMap;
        ViewDependentDataMap mViewDependentDataMap;
        uint32_t mTextureWidth;
        uint32_t mTextureHeight;
        int mRenderOrderNum;
        bool mDoPerViewMapping;
    };
}
#endif
