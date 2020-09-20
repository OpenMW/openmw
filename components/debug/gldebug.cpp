// This file is based heavily on code from https://github.com/ThermalPixel/osgdemos/blob/master/osgdebug/EnableGLDebugOperation.cpp
// The original licence is included below:
/*
Copyright (c) 2014, Andreas Klein
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#include "gldebug.hpp"

#include <cstdlib>

#include <components/debug/debuglog.hpp>

// OpenGL constants not provided by OSG:
#include <SDL_opengl_glext.h>

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
#ifdef GL_DEBUG_OUTPUT
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
#endif
}

void enableGLDebugExtension(unsigned int contextID)
{
#ifdef GL_DEBUG_OUTPUT
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
#endif
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

bool Debug::shouldDebugOpenGL()
{
    const char* env = std::getenv("OPENMW_DEBUG_OPENGL");
    if (!env)
        return false;
    std::string str(env);
    if (str.length() == 0)
        return true;

    return str.find("OFF") == std::string::npos && str.find("0") == std::string::npos && str.find("NO") == std::string::npos;
}
