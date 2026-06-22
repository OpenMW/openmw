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
        osg::Quat rotation = mBaseRotation;
        const osg::Quat invBaseRotation = mBaseRotation.inverse();

        if (mMode == Mode::RigidFaceCamera)
        {
            osg::Vec3d forward = -look;
            osg::Vec3d upNorm = up;
            forward.normalize();
            upNorm.normalize();
            osg::Vec3d right = upNorm ^ forward;
            upNorm = forward ^ right;

            const osg::Vec3d relRight = invBaseRotation * right;
            const osg::Vec3d relUp = invBaseRotation * upNorm;
            const osg::Vec3d relForward = invBaseRotation * forward;

            osg::Matrixd mat(relRight.x(), relRight.y(), relRight.z(), 0.0, relUp.x(), relUp.y(), relUp.z(), 0.0,
                relForward.x(), relForward.y(), relForward.z(), 0.0, 0.0, 0.0, 0.0, 1.0);

            rotation = mat.getRotate() * mBaseRotation;
        }
        else if (mMode == Mode::RotateAboutUp)
        {
            const osg::Vec3d relDelta = invBaseRotation * (eye - _matrix.getTrans());

            const double norm = std::sqrt(relDelta.x() * relDelta.x() + relDelta.z() * relDelta.z());
            if (norm > 1e-12)
            {
                const double xNorm = relDelta.x() / norm;
                const double zNorm = relDelta.z() / norm;

                osg::Matrixd mat(
                    zNorm, 0.0, -xNorm, 0.0, 0.0, 1.0, 0.0, 0.0, xNorm, 0.0, zNorm, 0.0, 0.0, 0.0, 0.0, 1.0);

                rotation = mat.getRotate() * mBaseRotation;
            }
        }

        osg::Matrixd matrix;
        matrix.makeScale(mScale, mScale, mScale);
        matrix.postMultRotate(rotation);
        matrix.postMultTranslate(_matrix.getTrans());

        return matrix;
    }
}
