#ifndef MISC_MATHUTIL_H
#define MISC_MATHUTIL_H

#include <osg/Math>
#include <osg/Vec2f>

namespace Misc
{

    /// Normalizes given angle to the range [-PI, PI]. E.g. PI*3/2 -> -PI/2.
    inline double normalizeAngle(double angle)
    {
        double fullTurns = angle / (2 * osg::PI) + 0.5;
        return (fullTurns - floor(fullTurns) - 0.5) * (2 * osg::PI);
    }
    
    /// Rotates given 2d vector counterclockwise. Angle is in radians.
    inline osg::Vec2f rotateVec2f(osg::Vec2f vec, float angle)
    {
        float s = std::sin(angle);
        float c = std::cos(angle);
        return osg::Vec2f(vec.x() * c + vec.y() * -s, vec.x() * s + vec.y() * c);
    }

}

#endif
