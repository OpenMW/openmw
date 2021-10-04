#ifndef OPENMW_MWRENDER_ROTATECONTROLLER_H
#define OPENMW_MWRENDER_ROTATECONTROLLER_H

#include <components/sceneutil/nodecallback.hpp>
#include <osg/Quat>

namespace osg
{
    class MatrixTransform;
}

namespace MWRender
{

/// Applies a rotation in \a relativeTo's space.
/// @note Assumes that the node being rotated has its "original" orientation set every frame by a different controller.
/// The rotation is then applied on top of that orientation.
class RotateController : public SceneUtil::NodeCallback<RotateController, osg::MatrixTransform*>
{
public:
    RotateController(osg::Node* relativeTo);

    void setEnabled(bool enabled);

    void setRotate(const osg::Quat& rotate);

    void operator()(osg::MatrixTransform* node, osg::NodeVisitor* nv);

protected:
    osg::Quat getWorldOrientation(osg::Node* node);

    bool mEnabled;
    osg::Quat mRotate;
    osg::Node* mRelativeTo;
};


}

#endif
