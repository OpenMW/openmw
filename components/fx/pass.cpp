#include "pass.hpp"

#include <unordered_set>
#include <string>
#include <sstream>

#include <osg/Program>
#include <osg/Shader>
#include <osg/State>
#include <osg/StateSet>
#include <osg/BindImageTexture>
#include <osg/FrameBufferObject>

#include <components/misc/stringops.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/clearcolor.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/stereo/multiview.hpp>

#include "technique.hpp"
#include "stateupdater.hpp"

namespace
{
    constexpr char s_DefaultVertex[] = R"GLSL(
#if OMW_USE_BINDINGS
    omw_In vec2 omw_Vertex;
#endif
omw_Out vec2 omw_TexCoord;

void main()
{
    omw_Position = vec4(omw_Vertex.xy, 0.0, 1.0);
    omw_TexCoord = omw_Position.xy * 0.5 + 0.5;
})GLSL";

}

namespace fx
{
    Pass::Pass(Pass::Type type, Pass::Order order, bool ubo)
        : mCompiled(false)
        , mType(type)
        , mOrder(order)
        , mLegacyGLSL(true)
        , mUBO(ubo)
    {
    }

    std::string Pass::getPassHeader(Technique& technique, std::string_view preamble, bool fragOut)
    {
        std::string header = R"GLSL(
#version @version @profile
@extensions

@uboStruct

#define OMW_REVERSE_Z @reverseZ
#define OMW_RADIAL_FOG @radialFog
#define OMW_HDR @hdr
#define OMW_NORMALS @normals
#define OMW_USE_BINDINGS @useBindings
#define OMW_MULTIVIEW @multiview
#define omw_In @in
#define omw_Out @out
#define omw_Position @position
#define omw_Texture1D @texture1D
#define omw_Texture2D @texture2D
#define omw_Texture3D @texture3D
#define omw_Vertex @vertex
#define omw_FragColor @fragColor

@fragBinding

uniform @builtinSampler omw_SamplerLastShader;
uniform @builtinSampler omw_SamplerLastPass;
uniform @builtinSampler omw_SamplerDepth;
uniform @builtinSampler omw_SamplerNormals;

uniform vec4 omw_PointLights[@pointLightCount];
uniform int omw_PointLightsCount;

int omw_GetPointLightCount()
{
    return omw_PointLightsCount;
}

vec3 omw_GetPointLightViewPos(int index)
{
    return omw_PointLights[(index * 3)].xyz;
}

vec3 omw_GetPointLightDiffuse(int index)
{
    return omw_PointLights[(index * 3) + 1].xyz;
}

vec3 omw_GetPointLightAttenuation(int index)
{
    return omw_PointLights[(index * 3) + 2].xyz;
}

float omw_GetPointLightRadius(int index)
{
    return omw_PointLights[(index * 3) + 2].w;
}

#if @ubo
    layout(std140) uniform _data { _omw_data omw; };
#else
    uniform _omw_data omw;
#endif

    float omw_GetDepth(vec2 uv)
    {
#if OMW_MULTIVIEW
        float depth = omw_Texture2D(omw_SamplerDepth, vec3(uv, gl_ViewID_OVR)).r;
#else
        float depth = omw_Texture2D(omw_SamplerDepth, uv).r;
#endif
#if OMW_REVERSE_Z
        return 1.0 - depth;
#else
        return depth;
#endif
    }

    vec4 omw_GetLastShader(vec2 uv)
    {
#if OMW_MULTIVIEW
        return omw_Texture2D(omw_SamplerLastShader, vec3(uv, gl_ViewID_OVR));
#else
        return omw_Texture2D(omw_SamplerLastShader, uv);
#endif
    }

    vec4 omw_GetLastPass(vec2 uv)
    {
#if OMW_MULTIVIEW
        return omw_Texture2D(omw_SamplerLastPass, vec3(uv, gl_ViewID_OVR));
#else
        return omw_Texture2D(omw_SamplerLastPass, uv);
#endif
    }

    vec3 omw_GetNormals(vec2 uv)
    {
#if OMW_MULTIVIEW
        return omw_Texture2D(omw_SamplerNormals, vec3(uv, gl_ViewID_OVR)).rgb * 2.0 - 1.0;
#else
        return omw_Texture2D(omw_SamplerNormals, uv).rgb * 2.0 - 1.0;
#endif
    }

#if OMW_HDR
    uniform sampler2D omw_EyeAdaptation;
#endif

    float omw_GetEyeAdaptation()
    {
#if OMW_HDR
        return omw_Texture2D(omw_EyeAdaptation, vec2(0.5, 0.5)).r;
#else
        return 1.0;
#endif
    }
)GLSL";

        std::stringstream extBlock;
        for (const auto& extension : technique.getGLSLExtensions())
            extBlock << "#ifdef " << extension << '\n' << "\t#extension " << extension << ": enable" << '\n' << "#endif" << '\n';

        const std::vector<std::pair<std::string,std::string>> defines = {
            {"@pointLightCount", std::to_string(SceneUtil::PPLightBuffer::sMaxPPLightsArraySize)},
            {"@version", std::to_string(technique.getGLSLVersion())},
            {"@multiview", Stereo::getMultiview() ? "1" : "0"},
            {"@builtinSampler", Stereo::getMultiview() ? "sampler2DArray" : "sampler2D"},
            {"@profile", technique.getGLSLProfile()},
            {"@extensions", extBlock.str()},
            {"@uboStruct", StateUpdater::getStructDefinition()},
            {"@ubo", mUBO ? "1" : "0"},
            {"@normals", technique.getNormals() ? "1" : "0"},
            {"@reverseZ", SceneUtil::AutoDepth::isReversed() ? "1" : "0"},
            {"@radialFog", Settings::Manager::getBool("radial fog", "Shaders") ? "1" : "0"},
            {"@hdr", technique.getHDR() ? "1" : "0"},
            {"@in", mLegacyGLSL ? "varying" : "in"},
            {"@out", mLegacyGLSL ? "varying" : "out"},
            {"@position", "gl_Position"},
            {"@texture1D", mLegacyGLSL ? "texture1D" : "texture"},
            {"@texture2D", mLegacyGLSL ? "texture2D" : "texture"},
            {"@texture3D", mLegacyGLSL ? "texture3D" : "texture"},
            {"@vertex", mLegacyGLSL ? "gl_Vertex" : "_omw_Vertex"},
            {"@fragColor", mLegacyGLSL ? "gl_FragColor" : "_omw_FragColor"},
            {"@useBindings", mLegacyGLSL ? "0" : "1"},
            {"@fragBinding", mLegacyGLSL ? "" : "out vec4 omw_FragColor;"}
        };

        for (const auto& [define, value]: defines)
            for (size_t pos = header.find(define); pos != std::string::npos; pos = header.find(define))
                header.replace(pos, define.size(), value);

        for (const auto& target : mRenderTargets)
            header.append("uniform sampler2D " + std::string(target) + ";");

        for (auto& uniform : technique.getUniformMap())
            if (auto glsl = uniform->getGLSL())
                header.append(glsl.value());

        header.append(preamble);

        return header;
    }

    void Pass::prepareStateSet(osg::StateSet* stateSet, const std::string& name) const
    {
        osg::ref_ptr<osg::Program> program = new osg::Program;
        if (mType == Type::Pixel)
        {
            program->addShader(new osg::Shader(*mVertex));
            program->addShader(new osg::Shader(*mFragment));
        }
        else if (mType == Type::Compute)
        {
            program->addShader(new osg::Shader(*mCompute));
        }

        if (mUBO)
            program->addBindUniformBlock("_data", static_cast<int>(Resource::SceneManager::UBOBinding::PostProcessor));

        program->setName(name);

        if (!mLegacyGLSL)
        {
            program->addBindFragDataLocation("_omw_FragColor", 0);
            program->addBindAttribLocation("_omw_Vertex", 0);
        }

        stateSet->setAttribute(program);

        if (mBlendSource && mBlendDest)
            stateSet->setAttribute(new osg::BlendFunc(mBlendSource.value(), mBlendDest.value()));

        if (mBlendEq)
            stateSet->setAttribute(new osg::BlendEquation(mBlendEq.value()));

        if (mClearColor)
            stateSet->setAttribute(new SceneUtil::ClearColor(mClearColor.value(), GL_COLOR_BUFFER_BIT));
    }

    void Pass::dirty()
    {
        mVertex = nullptr;
        mFragment = nullptr;
        mCompute = nullptr;
        mCompiled = false;
    }

    void Pass::compile(Technique& technique, std::string_view preamble)
    {
        if (mCompiled)
            return;

        mLegacyGLSL = technique.getGLSLVersion() != 330;

        if (mType == Type::Pixel)
        {
            if (!mVertex)
                mVertex = new osg::Shader(osg::Shader::VERTEX, s_DefaultVertex);

            mVertex->setShaderSource(getPassHeader(technique, preamble).append(mVertex->getShaderSource()));
            mFragment->setShaderSource(getPassHeader(technique, preamble, true).append(mFragment->getShaderSource()));

            mVertex->setName(mName);
            mFragment->setName(mName);
        }
        else if (mType == Type::Compute)
        {
            mCompute->setShaderSource(getPassHeader(technique, preamble).append(mCompute->getShaderSource()));
            mCompute->setName(mName);
        }

        mCompiled = true;
    }

}
