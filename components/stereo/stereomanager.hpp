#ifndef STEREO_MANAGER_H
#define STEREO_MANAGER_H

#include <osg/Camera>
#include <osg/Matrix>
#include <osg/StateSet>
#include <osg/Vec3>
#include <osgUtil/CullVisitor>

#include <array>
#include <memory>

#include <components/shader/shadermanager.hpp>

#include "types.hpp"

namespace osg
{
    class FrameBufferObject;
    class Texture2D;
    class Texture2DArray;
}

namespace osgViewer
{
    class Viewer;
}

namespace SceneUtil
{
    class MWShadowTechnique;
}

namespace Stereo
{
    class MultiviewFramebuffer;
    class StereoFrustumManager;
    class MultiviewStereoStatesetUpdateCallback;

    bool getStereo();

    //! Sets up any definitions necessary for stereo rendering
    void shaderStereoDefines(Shader::ShaderManager::DefineMap& defines);

    //! Class that provides tools for managing stereo mode
    class Manager
    {
    public:
        struct UpdateViewCallback
        {
            virtual ~UpdateViewCallback() = default;

            //! Called during the update traversal of every frame to update stereo views.
            virtual void updateView(View& left, View& right) = 0;
        };

        //! An UpdateViewCallback that supplies a fixed, custom view. Useful for debugging purposes,
        //! such as emulating a given HMD's view.
        struct CustomViewCallback : public UpdateViewCallback
        {
        public:
            CustomViewCallback(View& left, View& right);

            void updateView(View& left, View& right) override;

        private:
            View mLeft;
            View mRight;
        };

        //! Gets the singleton instance
        static Manager& instance();

        //! Constructor
        //!
        //! @Param viewer the osg viewer whose stereo should be managed.
        //! @Param enableStereo whether or not stereo should be enabled.
        //! @Param enableMultiview whether or not to make use of the GL_OVR_Multiview extension, if supported.
        //! @Param near defines distance to near camera clipping plane from view point.
        //! @Param far defines distance to far camera clipping plane from view point.
        explicit Manager(osgViewer::Viewer* viewer, bool enableStereo, double near, double far);
        ~Manager();

        //! Called during update traversal
        void update();

        void updateSettings(double near, double far)
        {
            mNear = near;
            mFar = far;
        }

        //! Initializes all details of stereo if applicable. If the constructor was called with enableMultiview=true,
        //! and the GL_OVR_Multiview extension is supported, Stereo::getMultiview() will return true after this call.
        void initializeStereo(osg::GraphicsContext* gc, bool enableMultiview, bool sharedShadowMaps);

        //! Callback that updates stereo configuration during the update pass
        void setUpdateViewCallback(std::shared_ptr<UpdateViewCallback> cb);

        //! Set the cull callback on the appropriate camera object
        void setCullCallback(osg::ref_ptr<osg::NodeCallback> cb);

        osg::Matrixd computeEyeProjection(int view, bool reverseZ) const;
        osg::Matrixd computeEyeViewOffset(int view) const;

        const std::shared_ptr<MultiviewFramebuffer>& multiviewFramebuffer() { return mMultiviewFramebuffer; }

        //! Sets rendering resolution of each eye to eyeResolution.
        //! Once set, there will no longer be any connection between rendering resolution and screen/window resolution.
        void overrideEyeResolution(const osg::Vec2i& eyeResolution);

        //! Notify stereo manager that the screen/window resolution has changed.
        void screenResolutionChanged();

        //! Get current eye resolution
        osg::Vec2i eyeResolution();

        //! The projection intended for rendering. When reverse Z is enabled, this is not the same as the camera's
        //! projection matrix, and therefore must be provided to the manager explicitly.
        void setMasterProjectionMatrix(const osg::Matrixd& projectionMatrix)
        {
            mMasterProjectionMatrix = projectionMatrix;
        }

        //! Causes the subgraph represented by the node to draw to the full viewport.
        //! This has no effect if stereo is not enabled
        void disableStereoForNode(osg::Node* node);

        void setShadowTechnique(SceneUtil::MWShadowTechnique* shadowTechnique);

        /// Determine which view the cull visitor belongs to
        Eye getEye(const osgUtil::CullVisitor* cv) const;

    private:
        friend class MultiviewStereoStatesetUpdateCallback;
        void updateMultiviewStateset(osg::StateSet* stateset);
        void updateStereoFramebuffer();
        void setupBruteForceTechnique();
        void setupOVRMultiView2Technique();

        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::Camera> mMainCamera;
        osg::ref_ptr<osg::Callback> mUpdateCallback;
        std::string mError;
        osg::Matrixd mMasterProjectionMatrix;
        std::shared_ptr<MultiviewFramebuffer> mMultiviewFramebuffer;
        bool mEyeResolutionOverriden;
        osg::Vec2i mEyeResolutionOverride;
        double mNear;
        double mFar;

        std::array<View, 2> mView;
        std::array<osg::Matrixd, 2> mViewOffsetMatrix;
        std::array<osg::Matrixd, 2> mProjectionMatrix;
        std::array<osg::Matrixd, 2> mProjectionMatrixReverseZ;

        std::unique_ptr<StereoFrustumManager> mFrustumManager;
        std::shared_ptr<UpdateViewCallback> mUpdateViewCallback;

        using Identifier = osgUtil::CullVisitor::Identifier;
        osg::ref_ptr<Identifier> mIdentifierMain = new Identifier();
        osg::ref_ptr<Identifier> mIdentifierLeft = new Identifier();
        osg::ref_ptr<Identifier> mIdentifierRight = new Identifier();
    };

    struct CustomView
    {
        Stereo::View mLeft;
        Stereo::View mRight;
    };

    struct Settings
    {
        bool mMultiview;
        bool mAllowDisplayListsForMultiview;
        bool mSharedShadowMaps;
        std::optional<CustomView> mCustomView;
        std::optional<osg::Vec2i> mEyeResolution;
    };

    //! Performs stereo-specific initialization operations.
    class InitializeStereoOperation final : public osg::GraphicsOperation
    {
    public:
        explicit InitializeStereoOperation(const Settings& settings);

        void operator()(osg::GraphicsContext* graphicsContext) override;

    private:
        bool mMultiview;
        bool mSharedShadowMaps;
        std::optional<CustomView> mCustomView;
        std::optional<osg::Vec2i> mEyeResolution;
    };
}

#endif
