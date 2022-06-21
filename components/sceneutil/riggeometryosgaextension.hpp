#ifndef OPENMW_COMPONENTS_OSGAEXTENSION_RIGGEOMETRY_H
#define OPENMW_COMPONENTS_OSGAEXTENSION_RIGGEOMETRY_H

#include <array>

#include <osg/Drawable>
#include <osgAnimation/RigGeometry>

#include <osg/NodeVisitor>

namespace SceneUtil
{
    /// @brief Custom RigGeometry-class for osgAnimation-formats (collada)
    class OsgaRigGeometry : public osgAnimation::RigGeometry
    {
    public:

        OsgaRigGeometry();

        OsgaRigGeometry(const osgAnimation::RigGeometry& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        OsgaRigGeometry(const OsgaRigGeometry& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Object(SceneUtil, OsgaRigGeometry);

        void computeMatrixFromRootSkeleton(osg::MatrixList mtxList);
    };

    /// @brief OpenMW-compatible double buffered static datavariance version of osgAnimation::RigGeometry
    /// This class is based on osgAnimation::RigGeometry and SceneUtil::RigGeometry
    class RigGeometryHolder : public osg::Drawable
    {
    public:
        RigGeometryHolder();

        RigGeometryHolder(const RigGeometryHolder& copy, const osg::CopyOp& copyop);

        RigGeometryHolder(const osgAnimation::RigGeometry& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, RigGeometryHolder);

        void setSourceRigGeometry(osg::ref_ptr<OsgaRigGeometry> sourceRigGeometry);
        osg::ref_ptr<OsgaRigGeometry> getSourceRigGeometry() const;

        /// @brief Modified rig update, code based on osgAnimation::UpdateRigGeometry : public osg::Drawable::UpdateCallback
        void updateRigGeometry(OsgaRigGeometry* geom, osg::NodeVisitor *nv);

        OsgaRigGeometry* getGeometry(int geometry);

        void accept(osg::NodeVisitor &nv) override;
        void accept(osg::PrimitiveFunctor&) const override;
        bool supports(const osg::PrimitiveFunctor&) const override{ return true; }
        
        void setBackToOrigin(osg::MatrixTransform* backToOrigin) {mBackToOrigin = backToOrigin;}
        void setBodyPart(bool isBodyPart) {mIsBodyPart = isBodyPart;}

    private:
        std::array<osg::ref_ptr<OsgaRigGeometry>, 2> mGeometry;
        osg::ref_ptr<OsgaRigGeometry> mSourceRigGeometry;
        osg::MatrixTransform* mBackToOrigin; //This is used to move riggeometries from their slot locations to skeleton origin in order to get correct deformations for bodyparts

        unsigned int mLastFrameNumber;
        bool mIsBodyPart;
        
        void updateBackToOriginTransform(OsgaRigGeometry* geometry);

        OsgaRigGeometry* getRigGeometryPerFrame(unsigned int frame) const;
    };

}

#endif
