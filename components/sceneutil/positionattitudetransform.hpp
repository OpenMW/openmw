#ifndef OPENMW_COMPONENTS_POSITIONATTITUDE_TRANSFORM_H
#define OPENMW_COMPONENTS_POSITIONATTITUDE_TRANSFORM_H

#include <osg/Transform>

namespace SceneUtil
{

/// @brief A customized version of osg::PositionAttitudeTransform optimized for speed.
/// Uses single precision values. Also removed _pivotPoint which we don't need.
class PositionAttitudeTransform : public osg::Transform
{
    public :
        PositionAttitudeTransform();

        PositionAttitudeTransform(const PositionAttitudeTransform& pat,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            Transform(pat,copyop),
            _position(pat._position),
            _attitude(pat._attitude),
            _scale(pat._scale){}


        META_Node(SceneUtil, PositionAttitudeTransform)

        inline void setPosition(const osg::Vec3f& pos) { _position = pos; dirtyBound(); }
        inline const osg::Vec3f& getPosition() const { return _position; }

        inline void setAttitude(const osg::Quat& quat) { _attitude = quat; dirtyBound(); }
        inline const osg::Quat& getAttitude() const { return _attitude; }

        inline void setScale(const osg::Vec3f& scale) { _scale = scale; dirtyBound(); }
        inline const osg::Vec3f& getScale() const { return _scale; }

        bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const override;
        bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const override;


    protected :
        virtual ~PositionAttitudeTransform() {}

        osg::Vec3f _position;
        osg::Quat _attitude;
        osg::Vec3f _scale;
};

}

#endif
