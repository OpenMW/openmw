#include "rotatecontroller.hpp"

#include <osg/MatrixTransform>
#include <osgAnimation/Bone>

namespace MWRender
{

    RotateController::RotateController(osg::Node* relativeTo)
        : mEnabled(true)
        , mRelativeTo(relativeTo)
    {
    }

    void RotateController::setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    void RotateController::setRotate(const osg::Quat& rotate)
    {
        mRotate = rotate;
    }

    void RotateController::setOffset(const osg::Vec3f& offset)
    {
        mOffset = offset;
    }

    void RotateController::operator()(osg::MatrixTransform* node, osg::NodeVisitor* nv)
    {
        if (!mEnabled)
        {
            traverse(node, nv);
            return;
        }
        osg::Matrix matrix = node->getMatrix();

        osg::Quat worldOrient;
        osg::Vec3d worldScale(1.0, 1.0, 1.0);

        osg::NodePathList nodepaths = node->getParentalNodePaths(mRelativeTo);

        if (!nodepaths.empty())
        {
            osg::Matrixf worldMat = osg::computeLocalToWorld(nodepaths[0]);
            worldOrient = worldMat.getRotate();
            worldScale = worldMat.getScale();
        }

        osg::Quat worldOrientInverse = worldOrient.inverse();

        osg::Quat orient = worldOrient * mRotate * worldOrientInverse * matrix.getRotate();
        matrix.setRotate(orient);

        matrix *= osg::Matrix::scale(worldScale);

        node->setMatrix(matrix);

        // If we are linked to a bone we must call setMatrixInSkeletonSpace
        osgAnimation::Bone* b = dynamic_cast<osgAnimation::Bone*>(node);
        if (b)
        {
            osgAnimation::Bone* parent = b->getBoneParent();
            if (parent)
                matrix *= parent->getMatrixInSkeletonSpace();

            b->setMatrixInSkeletonSpace(matrix);
        }

        traverse(node, nv);
    }
}
