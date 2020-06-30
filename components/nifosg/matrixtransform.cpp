#include "matrixtransform.hpp"

namespace NifOsg
{
    MatrixTransform::MatrixTransform(int recordIndex, const Nif::Transformation &trafo)
        : osg::MatrixTransform(trafo.toMatrix())
        , mIndex(recordIndex)
        , mScale(trafo.scale)
        , mRotationScale(trafo.rotation)
    {
    }

    MatrixTransform::MatrixTransform(const MatrixTransform &copy, const osg::CopyOp &copyop)
        : osg::MatrixTransform(copy, copyop)
        , mIndex(copy.mIndex)
        , mScale(copy.mScale)
        , mRotationScale(copy.mRotationScale)
    {
    }
}
