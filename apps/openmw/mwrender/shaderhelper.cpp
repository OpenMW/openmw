#include "shaderhelper.hpp"
#include "renderingmanager.hpp"
#include "shadows.hpp"

#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreGpuProgramParams.h>

#include <components/settings/settings.hpp>

using namespace Ogre;
using namespace MWRender;

ShaderHelper::ShaderHelper(RenderingManager* rend)
{
    mRendering = rend;
    applyShaders();
}

void ShaderHelper::applyShaders()
{
    if (!Settings::Manager::getBool("shaders", "Objects")) return;

    bool mrt = RenderingManager::useMRT();
    bool shadows = Settings::Manager::getBool("enabled", "Shadows");
    bool split = Settings::Manager::getBool("split", "Shadows");

    // shader for normal rendering
    createShader(mrt, shadows, split, "main");

    // fallback shader without mrt and without shadows
    // (useful for reflection and for minimap)
    createShader(false, false, false, "main_fallback");
}

void ShaderHelper::createShader(const bool mrt, const bool shadows, const bool split, const std::string& name)
{
    HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();

    const int numsplits = 3;

    // the number of lights to support.
    // when rendering an object, OGRE automatically picks the lights that are
    // closest to the object being rendered. unfortunately this mechanism does
    // not work perfectly for objects batched together (they will all use the same
    // lights). to work around this, we are simply pushing the maximum number
    // of lights here in order to minimize disappearing lights.
    int num_lights = Settings::Manager::getInt("num lights", "Objects");

    {
        // vertex
        HighLevelGpuProgramPtr vertex;
        if (!mgr.getByName(name+"_vp").isNull())
            mgr.remove(name+"_vp");

        vertex = mgr.createProgram(name+"_vp", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "cg", GPT_VERTEX_PROGRAM);
        vertex->setParameter("profiles", "vs_4_0 vs_2_x vp40 arbvp1");
        vertex->setParameter("entry_point", "main_vp");
        StringUtil::StrStreamType outStream;
        outStream <<
        "void main_vp(	\n"
        "	float4 position : POSITION,	\n"
        "   float4 normal : NORMAL, \n"
        "   float4 colour : COLOR, \n"
        "   in float2 uv : TEXCOORD0, \n"
        "   out float2 oUV : TEXCOORD0, \n"
        "	out float4 oPosition : POSITION,	\n"
        "   out float4 oPositionObjSpace : TEXCOORD1, \n"
        "   out float4 oNormal : TEXCOORD2, \n"
        "   out float oDepth : TEXCOORD3, \n"
        "   out float4 oVertexColour : TEXCOORD4, \n";
        if (shadows && !split) outStream <<
            "   out float4 oLightSpacePos0 : TEXCOORD5, \n"
            "   uniform float4x4 worldMatrix, \n"
            "   uniform float4x4 texViewProjMatrix0, \n";
        else
        {
            for (int i=0; i<numsplits; ++i)
            {
                outStream <<
                "   out float4 oLightSpacePos"<<i<<" : TEXCOORD"<<i+5<<", \n"
                "   uniform float4x4 texViewProjMatrix"<<i<<", \n";
            }
            outStream <<
            "   uniform float4x4 worldMatrix, \n";
        }
        outStream <<
        "	uniform float4x4 worldViewProj	\n"
        ")	\n"
        "{	\n"
        "   oVertexColour = colour; \n"
        "   oUV = uv; \n"
        "   oNormal = normal; \n"
        "	oPosition = mul( worldViewProj, position );  \n"
        "   oDepth = oPosition.z; \n"
        "   oPositionObjSpace = position; \n";
        if (shadows && !split) outStream <<
            "   oLightSpacePos0 = mul(texViewProjMatrix0, mul(worldMatrix, position)); \n";
        else
        {
            outStream <<
            "   float4 wPos = mul(worldMatrix, position); \n";
            for (int i=0; i<numsplits; ++i)
            {
                outStream <<
                "   oLightSpacePos"<<i<<" = mul(texViewProjMatrix"<<i<<", wPos); \n";
            }
        }
        outStream <<
        "}";
        vertex->setSource(outStream.str());
        vertex->load();
        vertex->getDefaultParameters()->setNamedAutoConstant("worldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        if (shadows)
        {
            vertex->getDefaultParameters()->setNamedAutoConstant("worldMatrix", GpuProgramParameters::ACT_WORLD_MATRIX);
            if (!split)
                vertex->getDefaultParameters()->setNamedAutoConstant("texViewProjMatrix0", GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX, 0);
            else
            {
                for (int i=0; i<numsplits; ++i)
                {
                    vertex->getDefaultParameters()->setNamedAutoConstant("texViewProjMatrix"+StringConverter::toString(i), GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX, i);
                }
            }
        }
    }

    {
        // fragment
        HighLevelGpuProgramPtr fragment;
        if (!mgr.getByName(name+"_fp").isNull())
            mgr.remove(name+"_fp");

        fragment = mgr.createProgram(name+"_fp", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "cg", GPT_FRAGMENT_PROGRAM);
        fragment->setParameter("profiles", "ps_4_0 ps_2_x fp40 arbfp1");
        fragment->setParameter("entry_point", "main_fp");
        StringUtil::StrStreamType outStream;

        if (shadows) outStream <<
            "float depthShadow(sampler2D shadowMap, float4 shadowMapPos, float2 offset) \n"
            "{ \n"
            "   shadowMapPos /= shadowMapPos.w; \n"
            "   float3 o = float3(offset.xy, -offset.x) * 0.3f; \n"
            "   float c =   (shadowMapPos.z <= tex2D(shadowMap, shadowMapPos.xy - o.xy).r) ? 1 : 0; // top left \n"
            "   c +=        (shadowMapPos.z <= tex2D(shadowMap, shadowMapPos.xy + o.xy).r) ? 1 : 0; // bottom right \n"
            "   c +=        (shadowMapPos.z <= tex2D(shadowMap, shadowMapPos.xy + o.zy).r) ? 1 : 0; // bottom left \n"
            "   c +=        (shadowMapPos.z <= tex2D(shadowMap, shadowMapPos.xy - o.zy).r) ? 1 : 0; // top right \n"
            "   return c / 4; \n"
            "} \n";

        outStream <<
        "void main_fp(	\n"
        "   in float2 uv : TEXCOORD0, \n"
        "	out float4 oColor    : COLOR, \n"
        "   uniform sampler2D texture : register(s0), \n"
        "   float4 positionObjSpace : TEXCOORD1, \n"
        "   float4 normal : TEXCOORD2, \n"
        "   float iDepth : TEXCOORD3, \n"
        "   float4 vertexColour : TEXCOORD4, \n"
        "   uniform float4 fogColour, \n"
        "   uniform float4 fogParams, \n";

        if (shadows) outStream <<
            "   uniform float4 shadowFar_fadeStart, \n";

        if (shadows && !split) outStream <<
            "   uniform sampler2D shadowMap : register(s1), \n"
            "   float4 lightSpacePos0 : TEXCOORD5, \n"
            "   uniform float4 invShadowmapSize0, \n";
        else
        {
            outStream <<
            "   uniform float4 pssmSplitPoints, \n";
            for (int i=0; i<numsplits; ++i)
            {
                outStream <<
                "   uniform sampler2D shadowMap"<<i<<" : register(s"<<i+1<<"), \n"
                "   float4 lightSpacePos"<<i<<" : TEXCOORD"<<i+5<<", \n"
                "   uniform float4 invShadowmapSize"<<i<<", \n";
            }
        }

        if (mrt) outStream <<
            "   out float4 oColor1 : COLOR1, \n"
            "   uniform float far, \n";

        for (int i=0; i<num_lights; ++i)
        {
            outStream <<
            "   uniform float4 lightDiffuse"<<i<<", \n"
            "   uniform float4 lightPositionObjSpace"<<i<<", \n"
            "   uniform float4 lightAttenuation"<<i<<", \n";
        }
        outStream <<
        "   uniform float4 lightAmbient, \n"
        "   uniform float4 ambient, \n"
        "   uniform float4 diffuse, \n"
        "   uniform float4 emissive \n"
        ")	\n"
        "{	\n"
        "   float4 tex =  tex2D(texture, uv); \n"
        "   float d; \n"
        "   float attn; \n"
        "   float3 lightDir; \n"
        "   float3 lightColour = float3(0, 0, 0); \n";

        for (int i=0; i<num_lights; ++i)
        {
            outStream <<
            "   lightDir = lightPositionObjSpace"<<i<<".xyz - (positionObjSpace.xyz * lightPositionObjSpace"<<i<<".w); \n"

            // pre-multiply light color with attenuation factor
            "   d = length( lightDir ); \n"
            "   attn = ( 1.0 / (( lightAttenuation"<<i<<".y ) + ( lightAttenuation"<<i<<".z * d ) + ( lightAttenuation"<<i<<".w * d * d ))); \n"
            "   lightDiffuse"<<i<<" *= attn; \n";

            if (i == 0 && shadows)
            {
                outStream <<
                "   float shadow; \n";
                if (!split) outStream <<
                    "   shadow = depthShadow(shadowMap, lightSpacePos0, invShadowmapSize0.xy); \n";
                else
                {
                    for (int j=0; j<numsplits; ++j)
                    {
                        std::string channel;
                        if (j==0) channel = "x";
                        else if (j==1) channel = "y";
                        else if (j==2) channel = "z";

                        if (j==0)
                        outStream << "	if (iDepth <= pssmSplitPoints." << channel << ") \n";
                        else if (j < numsplits - 1)
                            outStream << "	else if (iDepth <= pssmSplitPoints." << channel << ") \n";
                        else
                            outStream << "	else \n";

                        outStream <<
                        "	{ \n"
                        "		shadow = depthShadow(shadowMap" << j << ", lightSpacePos" << j << ", invShadowmapSize" << j << ".xy); \n"
                        "   } \n";
                    }
                }
                outStream <<
                "   float fadeRange = shadowFar_fadeStart.x - shadowFar_fadeStart.y; \n"
                "   float fade = 1-((iDepth - shadowFar_fadeStart.y) / fadeRange); \n"
                "   shadow = (iDepth > shadowFar_fadeStart.x) ? 1 : ((iDepth > shadowFar_fadeStart.y) ? 1-((1-shadow)*fade) : shadow); \n"
                "	lightColour.xyz += shadow * lit(dot(normalize(lightDir), normalize(normal)), 0, 0).y * lightDiffuse"<<i<<".xyz;\n";
            }
            else outStream <<
                "	lightColour.xyz += lit(dot(normalize(lightDir), normalize(normal)), 0, 0).y * lightDiffuse"<<i<<".xyz;\n";
        }

        outStream <<
        "   float3 lightingFinal = lightColour.xyz * diffuse.xyz * vertexColour.xyz + ambient.xyz * lightAmbient.xyz + emissive.xyz; \n"
        "   float fogValue = saturate((iDepth - fogParams.y) * fogParams.w); \n"
        "   oColor.xyz = lerp(lightingFinal * tex.xyz, fogColour, fogValue); \n"
        "   oColor.a = tex.a * diffuse.a * vertexColour.a; \n";

        if (mrt) outStream <<
            "   oColor1 = float4(iDepth / far, 0, 0, (oColor.a == 1)); \n"; // only write to MRT if alpha is 1

        outStream <<
        "}";
        fragment->setSource(outStream.str());
        fragment->load();

        for (int i=0; i<num_lights; ++i)
        {
            fragment->getDefaultParameters()->setNamedAutoConstant("lightPositionObjSpace"+StringConverter::toString(i), GpuProgramParameters::ACT_LIGHT_POSITION_OBJECT_SPACE, i);
            fragment->getDefaultParameters()->setNamedAutoConstant("lightDiffuse"+StringConverter::toString(i), GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR, i);
            fragment->getDefaultParameters()->setNamedAutoConstant("lightAttenuation"+StringConverter::toString(i), GpuProgramParameters::ACT_LIGHT_ATTENUATION, i);
        }
        fragment->getDefaultParameters()->setNamedAutoConstant("emissive", GpuProgramParameters::ACT_SURFACE_EMISSIVE_COLOUR);
        fragment->getDefaultParameters()->setNamedAutoConstant("diffuse", GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
        fragment->getDefaultParameters()->setNamedAutoConstant("ambient", GpuProgramParameters::ACT_SURFACE_AMBIENT_COLOUR);
        fragment->getDefaultParameters()->setNamedAutoConstant("lightAmbient", GpuProgramParameters::ACT_AMBIENT_LIGHT_COLOUR);
        fragment->getDefaultParameters()->setNamedAutoConstant("fogColour", GpuProgramParameters::ACT_FOG_COLOUR);
        fragment->getDefaultParameters()->setNamedAutoConstant("fogParams", GpuProgramParameters::ACT_FOG_PARAMS);

        if (shadows)
        {
            fragment->getDefaultParameters()->setNamedConstant("shadowFar_fadeStart", Vector4(mRendering->getShadows()->getShadowFar(), mRendering->getShadows()->getFadeStart()*mRendering->getShadows()->getShadowFar(), 0, 0));
            for (int i=0; i < (split ? numsplits : 1); ++i)
            {
                fragment->getDefaultParameters()->setNamedAutoConstant("invShadowmapSize" + StringConverter::toString(i), GpuProgramParameters::ACT_INVERSE_TEXTURE_SIZE, i+1);
            }
            if (split)
            {
				Vector4 splitPoints;
				const PSSMShadowCameraSetup::SplitPointList& splitPointList = mRendering->getShadows()->getPSSMSetup()->getSplitPoints();
				// Populate from split point 1, not 0, since split 0 isn't useful (usually 0)
				for (int i = 1; i < numsplits; ++i)
				{
					splitPoints[i-1] = splitPointList[i];
				}
                fragment->getDefaultParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
            }
        }

        if (mrt)
            fragment->getDefaultParameters()->setNamedAutoConstant("far", GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
    }
}
