#include "autotransform.hpp"

#include <cmath>

#include <osg/CullStack>

namespace NifOsg
{
    AutoTransform::AutoTransform(Mode mode)
        : MatrixTransform()
        , mMode(mode)
    {
    }

    AutoTransform::AutoTransform(const Nif::NiTransform& transform, Mode mode)
        : MatrixTransform(transform)
        , mMode(mode)
    {
        osg::Matrixd rotMat;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                rotMat(i, j) = mRotationScale.mValues[j][i];
        mBaseRotation = rotMat.getRotate();
        mRotation = mBaseRotation;
    }

    AutoTransform::AutoTransform(const AutoTransform& copy, const osg::CopyOp& copyop)
        : MatrixTransform(copy, copyop)
        , mMode(copy.mMode)
        , mBaseRotation(copy.mBaseRotation)
        , mRotation(copy.mRotation)
    {
    }

    void AutoTransform::setRotation(const osg::Quat& rotation)
    {
        MatrixTransform::setRotation(rotation);
        mBaseRotation = rotation;
        mRotation = rotation;
    }

    void AutoTransform::setRotation(const Nif::Matrix3& rotation)
    {
        MatrixTransform::setRotation(rotation);

        osg::Matrixd rotMat;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                rotMat(i, j) = rotation.mValues[j][i];

        mBaseRotation = rotMat.getRotate();
        mRotation = mBaseRotation;
    }

    bool AutoTransform::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (_referenceFrame == RELATIVE_RF)
            matrix.preMult(computeMatrix(nv));
        else
            matrix = computeMatrix(nv);
        return true;
    }

    bool AutoTransform::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (_referenceFrame == RELATIVE_RF)
            matrix.postMult(osg::Matrix::inverse(computeMatrix(nv)));
        else
            matrix = osg::Matrix::inverse(computeMatrix(nv));
        return true;
    }

    osg::Matrixd AutoTransform::computeMatrix(const osg::NodeVisitor* nv) const
    {
        const osg::CullStack* cs = nv ? nv->asCullStack() : nullptr;
        if (cs)
        {
            osg::Matrixd mat = computeMatrixForFrame(cs->getEyeLocal(), cs->getLookVectorLocal(), cs->getUpLocal());
            mRotation = mat.getRotate();
            return mat;
        }

        osg::Matrixd matrix;
        matrix.makeScale(mScale, mScale, mScale);
        matrix.postMultRotate(mRotation);
        matrix.postMultTranslate(_matrix.getTrans());
        return matrix;
    }

    osg::Matrixd AutoTransform::computeMatrixForFrame(
        const osg::Vec3d& eye, const osg::Vec3d& look, const osg::Vec3d& up) const
    {
        osg::Matrixd mat;

        const osg::Quat invBaseRotation = mBaseRotation.inverse();

        if (mMode == Mode::AlwaysFaceCamera || mMode == Mode::RigidFaceCamera)
        {
            osg::Vec3d relForward = invBaseRotation * look;
            osg::Vec3d relUp = invBaseRotation * up;
            relForward.normalize();
            osg::Vec3d relRight = relUp ^ relForward;
            relRight.normalize();
            relUp = relForward ^ relRight;
            relUp.normalize();

            if (mMode == Mode::AlwaysFaceCamera)
            {
                const double norm = std::sqrt(relUp.y() * relUp.y() + relRight.y() * relRight.y());
                if (norm > 1e-6)
                {
                    const double cosTheta = relUp.y() / norm;
                    const double sinTheta = -relRight.y() / norm;
                    const double m00 = -relRight.x() * cosTheta - relUp.x() * sinTheta;
                    const double m01 = -relRight.y() * cosTheta - relUp.y() * sinTheta;
                    const double m02 = -relRight.z() * cosTheta - relUp.z() * sinTheta;
                    const double m10 = relUp.x() * cosTheta - relRight.x() * sinTheta;
                    const double m11 = relUp.y() * cosTheta - relRight.y() * sinTheta;
                    const double m12 = relUp.z() * cosTheta - relRight.z() * sinTheta;
                    const double m20 = -relForward.x();
                    const double m21 = -relForward.y();
                    const double m22 = -relForward.z();

                    mat.set(m00, m01, m02, 0.0, m10, m11, m12, 0.0, m20, m21, m22, 0.0, 0.0, 0.0, 0.0, 1.0);
                }
            }
            else // Mode::RigidFaceCamera
            {
                mat.set(relRight.x(), relRight.y(), relRight.z(), 0.0, relUp.x(), relUp.y(), relUp.z(), 0.0,
                    relForward.x(), relForward.y(), relForward.z(), 0.0, 0.0, 0.0, 0.0, 1.0);
            }
        }
        else if (mMode == Mode::RotateAboutUp)
        {
            const osg::Vec3d relDelta = invBaseRotation * (eye - _matrix.getTrans());
            const double norm = std::sqrt(relDelta.x() * relDelta.x() + relDelta.z() * relDelta.z());
            if (norm > 1e-12)
            {
                const double xNorm = relDelta.x() / norm;
                const double zNorm = relDelta.z() / norm;

                mat.set(zNorm, 0.0, -xNorm, 0.0, 0.0, 1.0, 0.0, 0.0, xNorm, 0.0, zNorm, 0.0, 0.0, 0.0, 0.0, 1.0);
            }
        }

        osg::Matrixd matrix;
        matrix.makeScale(mScale, mScale, mScale);
        matrix.postMultRotate(mat.getRotate() * mBaseRotation);
        matrix.postMultTranslate(_matrix.getTrans());

        return matrix;
    }
}
