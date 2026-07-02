#ifndef OPENMW_COMPONENTS_NIFOSG_AUTOTRANSFORM_H
#define OPENMW_COMPONENTS_NIFOSG_AUTOTRANSFORM_H

#include <components/nifosg/matrixtransform.hpp>

namespace NifOsg
{
    class AutoTransform : public MatrixTransform
    {
    public:
        enum class Mode
        {
            AlwaysFaceCamera,
            RotateAboutUp,
            RigidFaceCamera
        };

        AutoTransform(Mode mode = Mode::RigidFaceCamera);
        AutoTransform(const Nif::NiTransform& transform, Mode mode = Mode::RigidFaceCamera);
        AutoTransform(const AutoTransform& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Node(NifOsg, AutoTransform)

        void setMode(Mode mode) { mMode = mode; }
        Mode getMode() const { return mMode; }

        virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const override;
        virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const override;

        osg::Matrixd computeMatrix(const osg::NodeVisitor* nv) const;
        osg::Matrixd computeMatrixForFrame(const osg::Vec3d& eye, const osg::Vec3d& look, const osg::Vec3d& up) const;

        void setRotation(const osg::Quat& rotation);
        void setRotation(const Nif::Matrix3& rotation);

    private:
        Mode mMode;
        osg::Quat mBaseRotation;
        mutable osg::Quat mRotation;
    };
}

#endif
