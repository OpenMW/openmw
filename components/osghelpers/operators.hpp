#ifndef OPENMW_COMPONENTS_OSGHELPERS_OPERATORS_H
#define OPENMW_COMPONENTS_OSGHELPERS_OPERATORS_H

#include <iomanip>
#include <limits>
#include <ostream>

#include <osg/Vec2i>
#include <osg/Vec2f>
#include <osg/Vec3f>

namespace osg
{
    inline std::ostream& operator <<(std::ostream& stream, const Vec2i& value)
    {
        return stream << "osg::Vec2i(" << value.x() << ", " << value.y() << ")";
    }

    inline std::ostream& operator <<(std::ostream& stream, const Vec2f& value)
    {
        return stream << "osg::Vec2f(" << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.x()
                      << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.y()
                      << ')';
    }

    inline std::ostream& operator <<(std::ostream& stream, const Vec3f& value)
    {
        return stream << "osg::Vec3f(" << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.x()
                      << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.y()
                      << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.z()
                      << ')';
    }
}

#endif
