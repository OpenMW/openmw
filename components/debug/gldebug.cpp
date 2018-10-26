#include "gldebug.hpp"

#include <components/debug/debuglog.hpp>

// OpenGL constants not provided by OSG:
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_TYPE_ERROR 0x824C

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    Log(Debug::Error) << message;
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
}

EnableGLDebugOperation::EnableGLDebugOperation() : osg::GraphicsOperation("EnableGLDebugOperation", false)
{
}

void EnableGLDebugOperation::operator()(osg::GraphicsContext* graphicsContext)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

    unsigned int contextID = graphicsContext->getState()->getContextID();
    enableGLDebugExtension(contextID);
}
