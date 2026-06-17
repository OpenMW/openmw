#include "types.hpp"

#include <osg/io_utils>

namespace Stereo
{

    Pose Pose::operator+(const Pose& rhs)
    {
        Pose pose = *this;
        pose.position += this->orientation * rhs.position;
        pose.orientation = rhs.orientation * this->orientation;
        return pose;
    }

    const Pose& Pose::operator+=(const Pose& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    Pose Pose::operator*(float scalar)
    {
        Pose pose = *this;
        pose.position *= scalar;
        return pose;
    }

    const Pose& Pose::operator*=(float scalar)
    {
        *this = *this * scalar;
        return *this;
    }

    Pose Pose::operator/(float scalar)
    {
        Pose pose = *this;
        pose.position /= scalar;
        return pose;
    }
    const Pose& Pose::operator/=(float scalar)
    {
        *this = *this / scalar;
        return *this;
    }

    bool Pose::operator==(const Pose& rhs) const
    {
        return position == rhs.position && orientation == rhs.orientation;
    }

    osg::Matrix View::viewMatrix(bool useGLConventions)
    {
        auto position = pose.position;
        auto orientation = pose.orientation;

        if (useGLConventions)
        {
            // When applied as an offset to an existing view matrix,
            // that view matrix will already convert points to a camera space
            // with opengl conventions. So we need to convert offsets to opengl
            // conventions.
            {
                float y = position.y();
                float z = position.z();
                position.y() = z;
                position.z() = -y;
            }
            {
                double y = orientation.y();
                double z = orientation.z();
                orientation.y() = z;
                orientation.z() = -y;
            }

            osg::Matrix viewMatrix;
            viewMatrix.setTrans(-position);
            viewMatrix.postMultRotate(orientation.conj());
            return viewMatrix;
        }
        else
        {
            osg::Vec3d forward = orientation * osg::Vec3d(0, 1, 0);
            osg::Vec3d up = orientation * osg::Vec3d(0, 0, 1);
            osg::Matrix viewMatrix;
            viewMatrix.makeLookAt(position, position + forward, up);

            return viewMatrix;
        }
    }

    osg::Matrix View::perspectiveMatrix(float near, float far, bool reverseZ)
    {
        const float tanLeft = tanf(fov.angleLeft);
        const float tanRight = tanf(fov.angleRight);
        const float tanDown = tanf(fov.angleDown);
        const float tanUp = tanf(fov.angleUp);

        const float tanWidth = tanRight - tanLeft;
        const float tanHeight = tanUp - tanDown;

        float matrix[16] = {};

        matrix[0] = 2 / tanWidth;
        matrix[4] = 0;
        matrix[8] = (tanRight + tanLeft) / tanWidth;
        matrix[12] = 0;

        matrix[1] = 0;
        matrix[5] = 2 / tanHeight;
        matrix[9] = (tanUp + tanDown) / tanHeight;
        matrix[13] = 0;

        if (reverseZ)
        {
            matrix[2] = 0;
            matrix[6] = 0;
            matrix[10] = (2.f * near) / (far - near);
            matrix[14] = ((2.f * near) * far) / (far - near);
        }
        else
        {
            matrix[2] = 0;
            matrix[6] = 0;
            matrix[10] = -(far + near) / (far - near);
            matrix[14] = -(far * (2.f * near)) / (far - near);
        }

        matrix[3] = 0;
        matrix[7] = 0;
        matrix[11] = -1;
        matrix[15] = 0;

        return osg::Matrix(matrix);
    }

    bool FieldOfView::operator==(const FieldOfView& rhs) const
    {
        return angleDown == rhs.angleDown && angleUp == rhs.angleUp && angleLeft == rhs.angleLeft
            && angleRight == rhs.angleRight;
    }

    bool View::operator==(const View& rhs) const
    {
        return pose == rhs.pose && fov == rhs.fov;
    }

    std::ostream& operator<<(std::ostream& os, const Pose& pose)
    {
        os << "position=" << pose.position << ", orientation=" << pose.orientation;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const FieldOfView& fov)
    {
        os << "left=" << fov.angleLeft << ", right=" << fov.angleRight << ", down=" << fov.angleDown
           << ", up=" << fov.angleUp;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const View& view)
    {
        os << "pose=< " << view.pose << " >, fov=< " << view.fov << " >";
        return os;
    }
}
