#include "depth.hpp"

#include <algorithm>
#include <array>

#include <components/debug/debuglog.hpp>
#include <components/sceneutil/glextensions.hpp>
#include <components/settings/values.hpp>

namespace SceneUtil
{
    void setCameraClearDepth(osg::Camera* camera)
    {
        camera->setClearDepth(AutoDepth::isReversed() ? 0.0 : 1.0);
    }

    osg::Matrix getReversedZProjectionMatrixAsPerspectiveInf(double fov, double aspect, double near)
    {
        double A = 1.0 / std::tan(osg::DegreesToRadians(fov) / 2.0);
        return osg::Matrix(A / aspect, 0, 0, 0, 0, A, 0, 0, 0, 0, 0, -1, 0, 0, near, 0);
    }

    osg::Matrix getReversedZProjectionMatrixAsPerspective(double fov, double aspect, double near, double far)
    {
        double A = 1.0 / std::tan(osg::DegreesToRadians(fov) / 2.0);
        return osg::Matrix(
            A / aspect, 0, 0, 0, 0, A, 0, 0, 0, 0, near / (far - near), -1, 0, 0, (far * near) / (far - near), 0);
    }

    osg::Matrix getReversedZProjectionMatrixAsOrtho(
        double left, double right, double bottom, double top, double near, double far)
    {
        return osg::Matrix(2 / (right - left), 0, 0, 0, 0, 2 / (top - bottom), 0, 0, 0, 0, 1 / (far - near), 0,
            (right + left) / (left - right), (top + bottom) / (bottom - top), far / (far - near), 1);
    }

    bool isDepthFormat(GLenum format)
    {
        constexpr std::array<GLenum, 8> formats = {
            GL_DEPTH_COMPONENT32F,
            GL_DEPTH_COMPONENT32F_NV,
            GL_DEPTH_COMPONENT16,
            GL_DEPTH_COMPONENT24,
            GL_DEPTH_COMPONENT32,
            GL_DEPTH32F_STENCIL8,
            GL_DEPTH32F_STENCIL8_NV,
            GL_DEPTH24_STENCIL8,
        };

        return std::find(formats.cbegin(), formats.cend(), format) != formats.cend();
    }

    bool isDepthStencilFormat(GLenum format)
    {
        constexpr std::array<GLenum, 8> formats = {
            GL_DEPTH32F_STENCIL8,
            GL_DEPTH32F_STENCIL8_NV,
            GL_DEPTH24_STENCIL8,
        };

        return std::find(formats.cbegin(), formats.cend(), format) != formats.cend();
    }

    void getDepthFormatSourceFormatAndType(GLenum internalFormat, GLenum& sourceFormat, GLenum& sourceType)
    {
        switch (internalFormat)
        {
            case GL_DEPTH_COMPONENT16:
            case GL_DEPTH_COMPONENT24:
            case GL_DEPTH_COMPONENT32:
                sourceType = GL_UNSIGNED_INT;
                sourceFormat = GL_DEPTH_COMPONENT;
                break;
            case GL_DEPTH_COMPONENT32F:
            case GL_DEPTH_COMPONENT32F_NV:
                sourceType = GL_FLOAT;
                sourceFormat = GL_DEPTH_COMPONENT;
                break;
            case GL_DEPTH24_STENCIL8:
                sourceType = GL_UNSIGNED_INT_24_8_EXT;
                sourceFormat = GL_DEPTH_STENCIL_EXT;
                break;
            case GL_DEPTH32F_STENCIL8:
            case GL_DEPTH32F_STENCIL8_NV:
                sourceType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
                sourceFormat = GL_DEPTH_STENCIL_EXT;
                break;
            default:
                sourceType = GL_UNSIGNED_INT;
                sourceFormat = GL_DEPTH_COMPONENT;
                break;
        }
    }

    GLenum getDepthFormatOfDepthStencilFormat(GLenum internalFormat)
    {
        switch (internalFormat)
        {
            case GL_DEPTH24_STENCIL8:
                return GL_DEPTH_COMPONENT24;
                break;
            case GL_DEPTH32F_STENCIL8:
                return GL_DEPTH_COMPONENT32F;
                break;
            case GL_DEPTH32F_STENCIL8_NV:
                return GL_DEPTH_COMPONENT32F_NV;
                break;
            default:
                return internalFormat;
                break;
        }
    }

    void SelectDepthFormatOperation::operator()(osg::GraphicsContext* graphicsContext)
    {
        bool enableReverseZ = false;

        if (Settings::camera().mReverseZ)
        {
            if (SceneUtil::getGLExtensions().isClipControlSupported)
            {
                enableReverseZ = true;
                Log(Debug::Info) << "Using reverse-z depth buffer";
            }
            else
                Log(Debug::Warning) << "GL_ARB_clip_control not supported: disabling reverse-z depth buffer";
        }
        else
            Log(Debug::Info) << "Using standard depth buffer";

        SceneUtil::AutoDepth::setReversed(enableReverseZ);

        constexpr char errPreamble[] = "Postprocessing and floating point depth buffers disabled: ";
        std::vector<GLenum> requestedFormats;
        unsigned int contextID = graphicsContext->getState()->getContextID();
        if (SceneUtil::AutoDepth::isReversed())
        {
            if (osg::isGLExtensionSupported(contextID, "GL_ARB_depth_buffer_float"))
            {
                requestedFormats.push_back(GL_DEPTH32F_STENCIL8);
            }
            else if (osg::isGLExtensionSupported(contextID, "GL_NV_depth_buffer_float"))
            {
                requestedFormats.push_back(GL_DEPTH32F_STENCIL8_NV);
            }
            else
            {
                Log(Debug::Warning) << errPreamble
                                    << "'GL_ARB_depth_buffer_float' and 'GL_NV_depth_buffer_float' unsupported.";
            }
        }

        requestedFormats.push_back(GL_DEPTH24_STENCIL8);
        if (mSupportedFormats.empty())
        {
            SceneUtil::AutoDepth::setDepthFormat(requestedFormats.front());
        }
        else
        {
            for (auto requestedFormat : requestedFormats)
            {
                if (std::find(mSupportedFormats.cbegin(), mSupportedFormats.cend(), requestedFormat)
                    != mSupportedFormats.cend())
                {
                    SceneUtil::AutoDepth::setDepthFormat(requestedFormat);
                    break;
                }
            }
        }
    }

    void AutoDepth::setDepthFormat(GLenum format)
    {
        sDepthInternalFormat = format;
        getDepthFormatSourceFormatAndType(sDepthInternalFormat, sDepthSourceFormat, sDepthSourceType);
    }
}
