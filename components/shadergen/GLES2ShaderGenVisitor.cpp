/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2009 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
 */

/**
 * \brief    Shader generator framework.
 * \author   Maciej Krol
 */

#include "GLES2ShaderGenVisitor.h"
#include <osg/Geode>
#include <osg/Geometry> // for ShaderGenVisitor::update
#include <osg/Fog>
#include <osg/Material>
#include <sstream>

#ifndef WIN32
#define SHADER_COMPAT \
"#ifndef GL_ES\n" \
"#if (__VERSION__ <= 110)\n" \
"#define lowp\n" \
"#define mediump\n" \
"#define highp\n" \
"#endif\n" \
"#endif\n"
#else
#define SHADER_COMPAT ""
#endif


using namespace osgUtil;

namespace osgUtil
{
    
    /// State extended by mode/attribute accessors
    class StateEx : public osg::State
    {
    public:
        StateEx() : State() {}
        
        osg::StateAttribute::GLModeValue getMode(osg::StateAttribute::GLMode mode,
                                                 osg::StateAttribute::GLModeValue def = osg::StateAttribute::INHERIT) const
        {
            return getMode(_modeMap, mode, def);
        }
        
        osg::StateAttribute *getAttribute(osg::StateAttribute::Type type, unsigned int member = 0) const
        {
            return getAttribute(_attributeMap, type, member);
        }
        
        osg::StateAttribute::GLModeValue getTextureMode(unsigned int unit,
                                                        osg::StateAttribute::GLMode mode,
                                                        osg::StateAttribute::GLModeValue def = osg::StateAttribute::INHERIT) const
        {
            return unit < _textureModeMapList.size() ? getMode(_textureModeMapList[unit], mode, def) : def;
        }
        
        osg::StateAttribute *getTextureAttribute(unsigned int unit, osg::StateAttribute::Type type) const
        {
            return unit < _textureAttributeMapList.size() ? getAttribute(_textureAttributeMapList[unit], type, 0) : 0;
        }
        
        osg::Uniform *getUniform(const std::string& name) const
        {
            UniformMap::const_iterator it = _uniformMap.find(name);
            return it != _uniformMap.end() ? 
            const_cast<osg::Uniform *>(it->second.uniformVec.back().first) : 0;
        }
        
    protected:
        
        osg::StateAttribute::GLModeValue getMode(const ModeMap &modeMap,
                                                 osg::StateAttribute::GLMode mode, 
                                                 osg::StateAttribute::GLModeValue def = osg::StateAttribute::INHERIT) const
        {
            ModeMap::const_iterator it = modeMap.find(mode);
            return (it != modeMap.end() && it->second.valueVec.size()) ? it->second.valueVec.back() : def;
        }
        
        osg::StateAttribute *getAttribute(const AttributeMap &attributeMap,
                                          osg::StateAttribute::Type type, unsigned int member = 0) const
        {
            AttributeMap::const_iterator it = attributeMap.find(std::make_pair(type, member));
            return (it != attributeMap.end() && it->second.attributeVec.size()) ? 
            const_cast<osg::StateAttribute*>(it->second.attributeVec.back().first) : 0;
        }
    };
    
}

void GLES2ShaderGenCache::setStateSet(int stateMask, osg::StateSet *stateSet)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    _stateSetMap[stateMask] = stateSet;
}

osg::StateSet* GLES2ShaderGenCache::getStateSet(int stateMask) const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    StateSetMap::const_iterator it = _stateSetMap.find(stateMask);
    return (it != _stateSetMap.end()) ? it->second.get() : 0;
}

osg::StateSet* GLES2ShaderGenCache::getOrCreateStateSet(int stateMask)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    StateSetMap::iterator it = _stateSetMap.find(stateMask);
    if (it == _stateSetMap.end())
    {
        osg::StateSet *stateSet = createStateSet(stateMask);
        _stateSetMap.insert(it, StateSetMap::value_type(stateMask, stateSet));
        return stateSet;
    }
    return it->second.get();
}

osg::StateSet* GLES2ShaderGenCache::createStateSet(int stateMask) const
{
    osg::StateSet *stateSet = new osg::StateSet;
    osg::Program *program = new osg::Program;
    stateSet->setAttribute(program);
    
    std::ostringstream vert;
    std::ostringstream frag;
    
    //first add the shader compat defines so that we don't have issues with using precision stuff
    vert << SHADER_COMPAT;
    frag << SHADER_COMPAT;
    
    // write varyings
    if ((stateMask & LIGHTING) && !(stateMask & NORMAL_MAP))
    {
        vert << "varying highp vec3 normalDir;\n";
    }
    
    if (stateMask & (LIGHTING | NORMAL_MAP))
    {
        vert << "struct osg_LightSourceParameters {"
        << "    mediump vec4  ambient;"
        << "    mediump vec4  diffuse;"
        << "    mediump vec4  specular;"
        << "    mediump vec4  position;"
        << "    mediump vec4  halfVector;"
        << "    mediump vec3  spotDirection;" 
        << "    mediump float  spotExponent;"
        << "    mediump float  spotCutoff;"
        << "    mediump float  spotCosCutoff;" 
        << "    mediump float  constantAttenuation;"
        << "    mediump float  linearAttenuation;"
        << "    mediump float  quadraticAttenuation;" 
        << "};\n"
        << "uniform osg_LightSourceParameters osg_LightSource[" << 1 << "];\n"
        
        << "struct  osg_LightProducts {"
        << "    mediump vec4  ambient;"
        << "    mediump vec4  diffuse;"
        << "    mediump vec4  specular;"
        << "};\n"
        << "uniform osg_LightProducts osg_FrontLightProduct[" << 1 << "];\n"
        
        << "varying highp vec3 lightDir;\n";
    }
    
    if (stateMask & (LIGHTING | NORMAL_MAP | FOG))
    {
        vert << "varying highp vec3 viewDir;\n";
    }
    
    //add texcoord varying if using gles2 as built in gl_TexCoord does not exist,
    //also no gl_FrontColor so we will define vColor
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
    if (stateMask & (DIFFUSE_MAP | NORMAL_MAP))
    {
        vert << "varying mediump vec4 texCoord0;\n";
    }
    vert << "varying mediump vec4 vColor;\n";
#endif
    
    // copy varying to fragment shader
    frag << vert.str();
    
    //add our material replacment uniforms for non fixed function versions
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
    if (stateMask & (LIGHTING | NORMAL_MAP))
    {
        frag <<
        "struct osgMaterial{\n"\
        "  mediump vec4 ambient;\n"\
        "  mediump vec4 diffuse;\n"\
        "  mediump vec4 specular;\n"\
        "  mediump float shine;\n"\
        "};\n"\
        
        "uniform osgMaterial osg_Material;\n";
    }
#endif
    

    // write uniforms and attributes
    int unit = 0;
    
    //add the replacements for gl matricies and verticies
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
    //vert << "attribute vec4 gl_Vertex;\n";
    vert << "attribute vec4 osg_Color;\n";
    vert << "uniform mat4 osg_ModelViewProjectionMatrix;\n";
#endif
    
    if (stateMask & DIFFUSE_MAP)
    {
        osg::Uniform *diffuseMap = new osg::Uniform("diffuseMap", unit++);
        stateSet->addUniform(diffuseMap);
        frag << "uniform sampler2D diffuseMap;\n";
    }
    
    if (stateMask & NORMAL_MAP)
    {
        osg::Uniform *normalMap = new osg::Uniform("normalMap", unit++);
        stateSet->addUniform(normalMap);
        frag << "uniform sampler2D normalMap;\n";
        program->addBindAttribLocation("tangent", 6);
        vert << "attribute vec3 tangent;\n";
    }
    
    
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE

    //non fixed function texturing
    if (stateMask & (DIFFUSE_MAP | NORMAL_MAP))
    {
        vert << "attribute vec4 osg_MultiTexCoord0;\n";
    }
    
    //non fixed function normal info
    if (stateMask & (LIGHTING | NORMAL_MAP))
    {
        //vert << "uniform mat4 osg_ModelViewMatrix;\n";
        //vert << "attribute vec3 osg_Normal;\n";
        //vert << "uniform mat3 osg_NormalMatrix;\n";        
    }
#endif
    
    vert << "\n"\
    "void main()\n"\
    "{\n"\
    
    //ftransform does not exist in gles2 (need to see if it's still in GL3)
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
    "  gl_Position = ftransform();\n";
#else
    "  gl_Position = osg_ModelViewProjectionMatrix * gl_Vertex;\n";
#endif
    
    if (stateMask & (DIFFUSE_MAP | NORMAL_MAP))
    {
        //gles2 does not have built in gl_TexCoord varying
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        vert << "  gl_TexCoord[0] = gl_MultiTexCoord0;\n";
#else
        vert << "  texCoord0 = osg_MultiTexCoord0;\n";
#endif
    }
    
    //
    
    if (stateMask & NORMAL_MAP)
    {
        //gl_NormalMatrix etc should be replaced automatically
        vert << 
        "  highp vec3 n = gl_NormalMatrix * gl_Normal;\n"\
        "  highp vec3 t = gl_NormalMatrix * tangent;\n"\
        "  highp vec3 b = cross(n, t);\n"\
        "  highp vec3 dir = -vec3(gl_ModelViewMatrix * gl_Vertex);\n"\
        "  viewDir.x = dot(dir, t);\n"\
        "  viewDir.y = dot(dir, b);\n"\
        "  viewDir.z = dot(dir, n);\n";
//use fixed light pos for now where gl_LightSource is not avaliable
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE 
        vert << "  vec4 lpos = gl_LightSource[0].position;\n";
#else
        vert << "  highp vec4 lpos = osg_LightSource[0].position;\n";
#endif
        vert << 
        "  if (lpos.w == 0.0)\n"\
        "    dir = lpos.xyz;\n"\
        "  else\n"\
        "    dir += lpos.xyz;\n"\
        "  lightDir.x = dot(dir, t);\n"\
        "  lightDir.y = dot(dir, b);\n"\
        "  lightDir.z = dot(dir, n);\n";
    }
    else if (stateMask & LIGHTING)
    {
        vert << 
        "  normalDir = gl_NormalMatrix * gl_Normal;\n"\
        "  highp vec3 dir = -vec3(gl_ModelViewMatrix * gl_Vertex);\n"\
        "  viewDir = dir;\n";
//use fixed light pos for now where gl_LightSource is not avaliable
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        vert << "  vec4 lpos = gl_LightSource[0].position;\n";
#else
        vert << "  vec4 lpos = osg_LightSource[0].position;\n";
#endif
        vert <<
        "  if (lpos.w == 0.0)\n"\
        "    lightDir = lpos.xyz;\n"\
        "  else\n"\
        "    lightDir = lpos.xyz + dir;\n";
    }
    else if (stateMask & FOG)
    {
        vert << "  viewDir = -vec3(gl_ModelViewMatrix * gl_Vertex);\n";
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        vert << "  gl_FrontColor = gl_Color;\n";
#else
        vert << "  vColor = osg_Color;\n";
#endif
    }
    else
    {
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        vert << "  gl_FrontColor = gl_Color;\n";
#else
        vert << "  vColor = osg_Color;\n";
#endif
    }
    
    vert << "}\n";
    
    frag << "\n"\
    "void main()\n"\
    "{\n";
    
    if (stateMask & DIFFUSE_MAP)
    {
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        frag << "  vec4 base = texture2D(diffuseMap, gl_TexCoord[0].st);\n";
#else
        frag << "  mediump vec4 base = texture2D(diffuseMap, texCoord0.st);\n";
#endif
    }
    else
    {
        frag << "  mediump vec4 base = vec4(1.0);\n";
    }
    
    if (stateMask & NORMAL_MAP)
    {
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        frag << "  vec3 normalDir = texture2D(normalMap, gl_TexCoord[0].st).xyz*2.0-1.0;\n";
#else
        frag << "  mediump vec3 normalDir = texture2D(normalMap, texCoord0.st).xyz*2.0-1.0;\n";        
        //frag << " normalDir.g = -normalDir.g;\n";
#endif
    }
    
    if (stateMask & (LIGHTING | NORMAL_MAP))
    {
//for now we will just have two versions of the below, once we have access to lights
//then we can completely replace use of gl_FrontLightModelProduct
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        frag << 
        "  highp vec3 nd = normalize(normalDir);\n"\
        "  highp vec3 ld = normalize(lightDir);\n"\
        "  highp vec3 vd = normalize(viewDir);\n"\
        "  mediump vec4 color = gl_FrontLightModelProduct.sceneColor;\n"\
        "  color += gl_FrontLightProduct[0].ambient;\n"\
        "  mediump float diff = max(dot(ld, nd), 0.0);\n"\
        "  color += gl_FrontLightProduct[0].diffuse * diff;\n"\
        "  color *= base;\n"\
        "  if (diff > 0.0)\n"\
        "  {\n"\
        "    highp vec3 halfDir = normalize(ld+vd);\n"\
        "    color.rgb += base.a * gl_FrontLightProduct[0].specular.rgb * \n"\
        "      pow(max(dot(halfDir, nd), 0.0), gl_FrontMaterial.shininess);\n"\
        "  }\n";
#else
        frag << 
        "  highp vec3 nd = normalize(normalDir);\n"\
        "  highp vec3 ld = normalize(lightDir);\n"\
        "  highp vec3 vd = normalize(viewDir);\n"\
        "  mediump vec4 color = vec4(0.01,0.01,0.01,1.0);\n"\
        "  color += osg_FrontLightProduct[0].ambient;\n"\
        "  mediump float diff = max(dot(ld, nd), 0.0);\n"\
        "  color += osg_FrontLightProduct[0].diffuse * diff;\n"\
        "  color *= base;\n"\
        "  if (diff > 0.0)\n"\
        "  {\n"\
        "    highp vec3 halfDir = normalize(ld+vd);\n"\
        "    color.rgb += base.a * osg_FrontLightProduct[0].specular.rgb * \n"\
        "      pow(max(dot(halfDir, nd), 0.0), osg_Material.shine);\n"\
        "  }\n";       
#endif
    }
    else
    {
        frag << "  mediump vec4 color = base;\n";
    }
    
    if (!(stateMask & LIGHTING))
    {
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        frag << "  color *= gl_Color;\n";
#else
        frag << "  color *= vColor;\n";
#endif
    }
    
    if (stateMask & FOG)
    {
        frag << 
        "  float d2 = dot(viewDir, viewDir);//gl_FragCoord.z/gl_FragCoord.w;\n"\
        "  float f = exp2(-1.442695*gl_Fog.density*gl_Fog.density*d2);\n"\
        "  color.rgb = mix(gl_Fog.color.rgb, color.rgb, clamp(f, 0.0, 1.0));\n";
    }
    
    frag << "  gl_FragColor = color;\n";
    frag << "}\n";
    
    std::string vertstr = vert.str();
    std::string fragstr = frag.str();
    
    OSG_DEBUG << "ShaderGenCache Vertex shader:\n" << vertstr << std::endl;
    OSG_DEBUG << "ShaderGenCache Fragment shader:\n" << fragstr << std::endl;
    
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertstr));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragstr));
    
    return stateSet;
}

GLES2ShaderGenVisitor::GLES2ShaderGenVisitor() : 
NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_stateCache(new GLES2ShaderGenCache),
_state(new StateEx)
{
}

GLES2ShaderGenVisitor::GLES2ShaderGenVisitor(GLES2ShaderGenCache *stateCache) : 
NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_stateCache(stateCache),
_state(new StateEx)
{
}

void GLES2ShaderGenVisitor::setRootStateSet(osg::StateSet *stateSet)
{
    if (_rootStateSet.valid())
        _state->removeStateSet(0);
    _rootStateSet = stateSet;
    if (_rootStateSet.valid())
        _state->pushStateSet(_rootStateSet.get());
}

void GLES2ShaderGenVisitor::reset()
{
    _state->popAllStateSets();
    if (_rootStateSet.valid())
        _state->pushStateSet(_rootStateSet.get());
}

void GLES2ShaderGenVisitor::apply(osg::Node &node)
{
    osg::StateSet *stateSet = node.getStateSet();
    
    if (stateSet)
        _state->pushStateSet(stateSet);
    
    traverse(node);
    
    if (stateSet)
        _state->popStateSet();
}

void GLES2ShaderGenVisitor::apply(osg::Geode &geode)
{
    osg::StateSet *stateSet = geode.getStateSet();
    if (stateSet)
        _state->pushStateSet(stateSet);
    
    for (unsigned int i=0; i<geode.getNumDrawables(); ++i)
    {
        osg::Drawable *drawable = geode.getDrawable(i);
        osg::StateSet *ss = drawable->getStateSet();
        if (ss)
            _state->pushStateSet(ss);
        
        update(drawable);
        
        if (ss)
            _state->popStateSet();
    }
    
    if (stateSet)
        _state->popStateSet();
}

void GLES2ShaderGenVisitor::update(osg::Drawable *drawable)
{
    // update only geometry due to compatibility issues with user defined drawables
    osg::Geometry *geometry = drawable->asGeometry();
#if 1
    if (!geometry)
        return;
#endif
    
    StateEx *state = static_cast<StateEx *>(_state.get());
    // skip nodes without state sets
    if (state->getStateSetStackSize() == (_rootStateSet.valid() ? 1u : 0u))
        return;
    
    // skip state sets with already attached programs
    if (state->getAttribute(osg::StateAttribute::PROGRAM))
        return;
    
    int stateMask = 0;
    //if (state->getMode(GL_BLEND) & osg::StateAttribute::ON)
    //    stateMask |= ShaderGen::BLEND;
    if (state->getMode(GL_LIGHTING) & osg::StateAttribute::ON)
        stateMask |= GLES2ShaderGenCache::LIGHTING;
    if (state->getMode(GL_FOG) & osg::StateAttribute::ON)
        stateMask |= GLES2ShaderGenCache::FOG;
    if (state->getTextureAttribute(0, osg::StateAttribute::TEXTURE))
        stateMask |= GLES2ShaderGenCache::DIFFUSE_MAP;
    
    if (state->getTextureAttribute(1, osg::StateAttribute::TEXTURE) && geometry!=0 &&
        geometry->getVertexAttribArray(6)) //tangent
        stateMask |= GLES2ShaderGenCache::NORMAL_MAP;
    
    // Get program and uniforms for accumulated state.
    osg::StateSet *progss = _stateCache->getOrCreateStateSet(stateMask);
    // Set program and uniforms to the last state set.
    osg::StateSet *ss = const_cast<osg::StateSet *>(state->getStateSetStack().back());
    ss->setAttribute(progss->getAttribute(osg::StateAttribute::PROGRAM));
    ss->setUniformList(progss->getUniformList());
    
    //Edit, for now we will pinch the Material colors and bind as uniforms for non fixed function to replace gl_Front Material
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
    osg::Material* mat = dynamic_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
    if(mat){
        ss->addUniform(new osg::Uniform("osg_Material.ambient", mat->getAmbient(osg::Material::FRONT)));
        ss->addUniform(new osg::Uniform("osg_Material.diffuse", mat->getDiffuse(osg::Material::FRONT)));
        ss->addUniform(new osg::Uniform("osg_Material.specular", mat->getSpecular(osg::Material::FRONT)));
        ss->addUniform(new osg::Uniform("osg_Material.shine", mat->getShininess(osg::Material::FRONT)));
        ss->removeAttribute(osg::StateAttribute::MATERIAL);
    }else{
        //if no material then setup some reasonable defaults
        ss->addUniform(new osg::Uniform("osg_Material.ambient", osg::Vec4(0.2f,0.2f,0.2f,1.0f)));
        ss->addUniform(new osg::Uniform("osg_Material.diffuse", osg::Vec4(0.8f,0.8f,0.8f,1.0f)));
        ss->addUniform(new osg::Uniform("osg_Material.specular", osg::Vec4(1.0f,1.0f,1.0f,1.0f)));
        ss->addUniform(new osg::Uniform("osg_Material.shine", 16.0f));
    }
#endif
    
    
    // remove any modes that won't be appropriate when using shaders
    if ((stateMask & GLES2ShaderGenCache::LIGHTING)!=0)
    {
        ss->removeMode(GL_LIGHTING);
        ss->removeMode(GL_LIGHT0);
    }
    if ((stateMask & GLES2ShaderGenCache::FOG)!=0)
    {
        ss->removeMode(GL_FOG);
    }
    if ((stateMask & GLES2ShaderGenCache::DIFFUSE_MAP)!=0) ss->removeTextureMode(0, GL_TEXTURE_2D);
    if ((stateMask & GLES2ShaderGenCache::NORMAL_MAP)!=0) ss->removeTextureMode(1, GL_TEXTURE_2D);
}


