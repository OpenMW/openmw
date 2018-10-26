#include "gldebug.hpp"

#include <components/debug/debuglog.hpp>

// OpenGL constants not provided by OSG:
#include <SDL_opengl_glext.h>

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    std::string srcStr;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        srcStr = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        srcStr = "WINDOW_SYSTEM";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        srcStr = "SHADER_COMPILER";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        srcStr = "THIRD_PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        srcStr = "APPLICATION";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        srcStr = "OTHER";
        break;
    default:
        srcStr = "UNDEFINED";
        break;
    }

    std::string typeStr;

    Debug::Level logSeverity = Debug::Warning;
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        logSeverity = Debug::Error;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "OTHER";
        break;
    default:
        typeStr = "UNDEFINED";
        break;
    }

    Log(logSeverity) << "OpenGL " << typeStr << " [" << srcStr << "]: " << message;
}

void enableGLDebugExtension(unsigned int contextID)
{
    typedef void (GL_APIENTRY *DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
    typedef void (GL_APIENTRY *GLDebugMessageControlFunction)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
    typedef void (GL_APIENTRY *GLDebugMessageCallbackFunction)(DEBUGPROC, const void* userParam);
    
    GLDebugMessageControlFunction glDebugMessageControl = nullptr;
    GLDebugMessageCallbackFunction glDebugMessageCallback = nullptr;

    if (osg::isGLExtensionSupported(contextID, "GL_KHR_debug"))
    {
        osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallback");
        osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControl");
    }
    else if (osg::isGLExtensionSupported(contextID, "GL_ARB_debug_output"))
    {
        osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackARB");
        osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlARB");
    }
    else if (osg::isGLExtensionSupported(contextID, "GL_AMD_debug_output"))
    {
        osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackAMD");
        osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlAMD");
    }

    if (glDebugMessageCallback && glDebugMessageControl)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, true);
        glDebugMessageCallback(debugCallback, nullptr);

        Log(Debug::Info) << "OpenGL debug callback attached.";
    }
    else
        Log(Debug::Error) << "Unable to attach OpenGL debug callback.";
}

Debug::EnableGLDebugOperation::EnableGLDebugOperation() : osg::GraphicsOperation("EnableGLDebugOperation", false)
{
}

void Debug::EnableGLDebugOperation::operator()(osg::GraphicsContext* graphicsContext)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

    unsigned int contextID = graphicsContext->getState()->getContextID();
    enableGLDebugExtension(contextID);
}
