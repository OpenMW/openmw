#include "matrixtransform.hpp"

namespace NifOsg
{
    MatrixTransform::MatrixTransform()
        : osg::MatrixTransform()
    {
    }

    MatrixTransform::MatrixTransform(const Nif::Transformation &trafo)
        : osg::MatrixTransform(trafo.toMatrix())
        , mScale(trafo.scale)
        , mRotationScale(trafo.rotation)
    {
    }

    MatrixTransform::MatrixTransform(const MatrixTransform &copy, const osg::CopyOp &copyop)
        : osg::MatrixTransform(copy, copyop)
        , mScale(copy.mScale)
        , mRotationScale(copy.mRotationScale)
    {
    }
}
