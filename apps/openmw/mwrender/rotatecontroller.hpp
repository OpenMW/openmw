#ifndef OPENMW_MWRENDER_ROTATECONTROLLER_H
#define OPENMW_MWRENDER_ROTATECONTROLLER_H

#include <osg/NodeCallback>
#include <osg/Quat>

namespace MWRender
{

/// Applies a rotation in \a relativeTo's space.
/// @note Assumes that the node being rotated has its "original" orientation set every frame by a different controller.
/// The rotation is then applied on top of that orientation.
/// @note Must be set on a MatrixTransform.
class RotateController : public osg::NodeCallback
{
public:
    RotateController(osg::Node* relativeTo);

    void setEnabled(bool enabled);

    void setRotate(const osg::Quat& rotate);

    void operator()(osg::Node* node, osg::NodeVisitor* nv) override;

protected:
    osg::Quat getWorldOrientation(osg::Node* node);

    bool mEnabled;
    osg::Quat mRotate;
    osg::Node* mRelativeTo;
};


}

#endif
