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
#include <memory>

#include <components/debug/debuglog.hpp>

// OpenGL constants not provided by OSG:
#include <SDL_opengl_glext.h>

namespace Debug
{

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

        Level logSeverity = Warning;
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            typeStr = "ERROR";
            logSeverity = Error;
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

    class PushDebugGroup
    {
    public:
        static std::unique_ptr<PushDebugGroup> sInstance;

        void (GL_APIENTRY * glPushDebugGroup) (GLenum source, GLuint id, GLsizei length, const GLchar * message);

        void (GL_APIENTRY * glPopDebugGroup) (void);

        bool valid()
        {
            return glPushDebugGroup && glPopDebugGroup;
        }
    };

    std::unique_ptr<PushDebugGroup> PushDebugGroup::sInstance{ std::make_unique<PushDebugGroup>() };

    EnableGLDebugOperation::EnableGLDebugOperation() : osg::GraphicsOperation("EnableGLDebugOperation", false)
    {
    }

    void EnableGLDebugOperation::operator()(osg::GraphicsContext* graphicsContext)
    {
#ifdef GL_DEBUG_OUTPUT
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

        unsigned int contextID = graphicsContext->getState()->getContextID();

        typedef void (GL_APIENTRY *DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
        typedef void (GL_APIENTRY *GLDebugMessageControlFunction)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
        typedef void (GL_APIENTRY *GLDebugMessageCallbackFunction)(DEBUGPROC, const void* userParam);

        GLDebugMessageControlFunction glDebugMessageControl = nullptr;
        GLDebugMessageCallbackFunction glDebugMessageCallback = nullptr;

        if (osg::isGLExtensionSupported(contextID, "GL_KHR_debug"))
        {
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallback");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControl");
            osg::setGLExtensionFuncPtr(PushDebugGroup::sInstance->glPushDebugGroup, "glPushDebugGroup");
            osg::setGLExtensionFuncPtr(PushDebugGroup::sInstance->glPopDebugGroup, "glPopDebugGroup");
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

            Log(Info) << "OpenGL debug callback attached.";
        }
        else
#endif
            Log(Error) << "Unable to attach OpenGL debug callback.";
    }

    bool shouldDebugOpenGL()
    {
        const char* env = std::getenv("OPENMW_DEBUG_OPENGL");
        if (!env)
            return false;
        std::string str(env);
        if (str.length() == 0)
            return true;

        return str.find("OFF") == std::string::npos && str.find('0') == std::string::npos && str.find("NO") == std::string::npos;
    }

    DebugGroup::DebugGroup(const std::string & message, GLuint id)
    #ifdef GL_DEBUG_OUTPUT
        : mSource(GL_DEBUG_SOURCE_APPLICATION)
    #else
        : mSource(0x824A)
    #endif
        , mId(id)
        , mMessage(message)
    {
    }

    DebugGroup::DebugGroup(const DebugGroup & debugGroup, const osg::CopyOp & copyop)
        : osg::StateAttribute(debugGroup, copyop)
        , mSource(debugGroup.mSource)
        , mId(debugGroup.mId)
        , mMessage(debugGroup.mMessage)
    {
    }

    void DebugGroup::apply(osg::State & state) const
    {
        if (!PushDebugGroup::sInstance->valid())
        {
            Log(Error) << "OpenGL debug groups not supported on this system, or OPENMW_DEBUG_OPENGL environment variable not set.";
            return;
        }

        auto& attributeVec = state.getAttributeVec(this);
        auto& lastAppliedStack = sLastAppliedStack[state.getContextID()];

        size_t firstNonMatch = 0;
        while (firstNonMatch < lastAppliedStack.size()
               && ((firstNonMatch < attributeVec.size() && lastAppliedStack[firstNonMatch] == attributeVec[firstNonMatch].first)
                   || lastAppliedStack[firstNonMatch] == this))
            firstNonMatch++;

        for (size_t i = lastAppliedStack.size(); i > firstNonMatch; --i)
            lastAppliedStack[i - 1]->pop(state);
        lastAppliedStack.resize(firstNonMatch);

        lastAppliedStack.reserve(attributeVec.size());
        for (size_t i = firstNonMatch; i < attributeVec.size(); ++i)
        {
            const DebugGroup* group = static_cast<const DebugGroup*>(attributeVec[i].first);
            group->push(state);
            lastAppliedStack.push_back(group);
        }
        if (!(lastAppliedStack.back() == this))
        {
            push(state);
            lastAppliedStack.push_back(this);
        }
    }

    int DebugGroup::compare(const StateAttribute & sa) const
    {
        COMPARE_StateAttribute_Types(DebugGroup, sa);

        COMPARE_StateAttribute_Parameter(mSource);
        COMPARE_StateAttribute_Parameter(mId);
        COMPARE_StateAttribute_Parameter(mMessage);

        return 0;
    }

    void DebugGroup::releaseGLObjects(osg::State * state) const
    {
        if (state)
            sLastAppliedStack.erase(state->getContextID());
        else
            sLastAppliedStack.clear();
    }

    bool DebugGroup::isValid() const
    {
        return mSource || mId || mMessage.length();
    }

    void DebugGroup::push(osg::State & state) const
    {
        if (isValid())
            PushDebugGroup::sInstance->glPushDebugGroup(mSource, mId, mMessage.size(), mMessage.c_str());
    }

    void DebugGroup::pop(osg::State & state) const
    {
        if (isValid())
            PushDebugGroup::sInstance->glPopDebugGroup();
    }

    std::map<unsigned int, std::vector<const DebugGroup *>> DebugGroup::sLastAppliedStack{};

}
