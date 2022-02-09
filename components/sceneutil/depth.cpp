#include "depth.hpp"

#include <algorithm>

#include <SDL_opengl_glext.h>

#include <components/settings/settings.hpp>

namespace SceneUtil
{
    void setCameraClearDepth(osg::Camera* camera)
    {
        camera->setClearDepth(AutoDepth::isReversed() ? 0.0 : 1.0);
    }

    osg::Matrix getReversedZProjectionMatrixAsPerspectiveInf(double fov, double aspect, double near)
    {
        double A = 1.0/std::tan(osg::DegreesToRadians(fov)/2.0);
        return osg::Matrix(
            A/aspect,   0,      0,      0,
            0,          A,      0,      0,
            0,          0,      0,      -1,
            0,          0,      near,   0
        );
    }

    osg::Matrix getReversedZProjectionMatrixAsPerspective(double fov, double aspect, double near, double far)
    {
        double A = 1.0/std::tan(osg::DegreesToRadians(fov)/2.0);
        return osg::Matrix(
            A/aspect,   0,      0,                          0,
            0,          A,      0,                          0,
            0,          0,      near/(far-near),            -1,
            0,          0,      (far*near)/(far - near),    0
        );
    }

    osg::Matrix getReversedZProjectionMatrixAsOrtho(double left, double right, double bottom, double top, double near, double far)
    {
        return osg::Matrix(
            2/(right-left),             0,                          0,                  0,
            0,                          2/(top-bottom),             0,                  0,
            0,                          0,                          1/(far-near),       0,
            (right+left)/(left-right),  (top+bottom)/(bottom-top),  far/(far-near),     1
        );
    }

    bool isFloatingPointDepthFormat(GLenum format)
    {
        constexpr std::array<GLenum, 4> formats = {
            GL_DEPTH_COMPONENT32F,
            GL_DEPTH_COMPONENT32F_NV,
            GL_DEPTH32F_STENCIL8,
            GL_DEPTH32F_STENCIL8_NV,
        };

        return std::find(formats.cbegin(), formats.cend(), format) != formats.cend();
    }
}