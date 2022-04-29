#ifndef STEREO_MANAGER_H
#define STEREO_MANAGER_H

#include <osg/Matrix>
#include <osg/Vec3>
#include <osg/Camera>
#include <osg/StateSet>
#include <osgUtil/CullVisitor>

#include <memory>
#include <array>

#include <components/stereo/types.hpp>
#include <components/shader/shadermanager.hpp>

namespace osg
{
    class FrameBufferObject;
    class Texture2D;
    class Texture2DMultisample;
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

        //! Gets the singleton instance
        static Manager& instance();

        Manager(osgViewer::Viewer* viewer);
        ~Manager();

        //! Called during update traversal
        void update();

        void initializeStereo(osg::GraphicsContext* gc);

        //! Callback that updates stereo configuration during the update pass
        void setUpdateViewCallback(std::shared_ptr<UpdateViewCallback> cb);

        //! Set the cull callback on the appropriate camera object
        void setCullCallback(osg::ref_ptr<osg::NodeCallback> cb);

        osg::Matrixd computeEyeProjection(int view, bool reverseZ) const;
        osg::Matrixd computeEyeView(int view) const;
        osg::Matrixd computeEyeViewOffset(int view) const;

        //! Sets up any definitions necessary for stereo rendering
        void shaderStereoDefines(Shader::ShaderManager::DefineMap& defines) const;

        const std::shared_ptr<MultiviewFramebuffer>& multiviewFramebuffer() { return mMultiviewFramebuffer; };

        //! Sets rendering resolution of each eye to eyeResolution. 
        //! Once set, there will no longer be any connection between rendering resolution and screen/window resolution.
        void overrideEyeResolution(const osg::Vec2i& eyeResolution);

        //! Notify stereo manager that the screen/window resolution has changed.
        void screenResolutionChanged();

        //! Get current eye resolution
        osg::Vec2i eyeResolution();

        //! The projection intended for rendering. When reverse Z is enabled, this is not the same as the camera's projection matrix, 
        //! and therefore must be provided to the manager explicitly.
        void setMasterProjectionMatrix(const osg::Matrix& projectionMatrix) { mMasterProjectionMatrix = projectionMatrix; }

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
        osg::ref_ptr<osg::Camera>       mMainCamera;
        osg::ref_ptr<osg::Callback>     mUpdateCallback;
        std::string                     mError;
        osg::Matrix                     mMasterProjectionMatrix;
        std::shared_ptr<MultiviewFramebuffer> mMultiviewFramebuffer;
        bool                            mEyeResolutionOverriden;
        osg::Vec2i                      mEyeResolutionOverride;

        std::array<View, 2>         mView;
        std::array<osg::Matrix, 2>  mViewMatrix;
        std::array<osg::Matrix, 2>  mViewOffsetMatrix;
        std::array<osg::Matrix, 2>  mProjectionMatrix;
        std::array<osg::Matrix, 2>  mProjectionMatrixReverseZ;

        std::unique_ptr<StereoFrustumManager> mFrustumManager;
        std::shared_ptr<UpdateViewCallback> mUpdateViewCallback;

        using Identifier = osgUtil::CullVisitor::Identifier;
        osg::ref_ptr<Identifier> mIdentifierMain = new Identifier();
        osg::ref_ptr<Identifier> mIdentifierLeft = new Identifier();
        osg::ref_ptr<Identifier> mIdentifierRight = new Identifier();
    };
}

#endif
