/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2010 Robert Osfield
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

/* ##################################################################################################### */
/* ParallelSplitShadowMap written by Adrian Egli (3dhelp (at) gmail.com)                                 */
/* ##################################################################################################### */
/*                                                                                                       */
/* the pssm main idea is based on:                                                                       */
/*                                                                                                       */
/* Parallel-Split Shadow Maps for Large-scale Virtual Environments                                       */
/*    Fan Zhang     Hanqiu Sun    Leilei Xu    Lee Kit Lun                                               */
/*    The Chinese University of Hong Kong                                                                */
/*                                                                                                       */
/* Refer to our latest project webpage for "Parallel-Split Shadow Maps on Programmable GPUs" in GPU Gems */
/*                                                                                                       */
/* ##################################################################################################### */

#include "parallelsplitshadowmap.hpp"

#include <osgShadow/ShadowedScene>
#include <osg/Notify>
#include <osg/ComputeBoundsVisitor>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/io_utils>
#include <iostream>
#include <sstream>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osg/Texture1D>
#include <osg/Depth>
#include <osg/ShadeModel>
#include <osgUtil/RenderStage>
//#define CLASSIC 1
using namespace MWRender;

// split scheme
#define TEXTURE_RESOLUTION  1024


#define ZNEAR_MIN_FROM_LIGHT_SOURCE 5.0
#define MOVE_VIRTUAL_CAMERA_BEHIND_REAL_CAMERA_FACTOR 0.0

// #define SHOW_SHADOW_TEXTURE_DEBUG    // DEPTH instead of color for debug information texture display in a rectangle
//#define SHADOW_TEXTURE_DEBUG         // COLOR instead of DEPTH

#ifndef SHADOW_TEXTURE_DEBUG
#define SHADOW_TEXTURE_GLSL
#endif

//////////////////////////////////////////////////////////////////////////
// FragmentShaderGenerator
std::string ParallelSplitShadowMap::FragmentShaderGenerator::generateGLSL_FragmentShader_BaseTex(
    bool debug,
    unsigned int splitCount,
    double textureRes,
    bool filtered,
    unsigned int nbrSplits,
    unsigned int textureOffset
) {
    std::stringstream sstr;

    /// base texture
    sstr << "uniform sampler2D baseTexture; "      << std::endl;
    sstr << "uniform float enableBaseTexture; "     << std::endl;
    sstr << "uniform vec2 ambientBias;"    << std::endl;

    for (unsigned int i=0; i<nbrSplits; i++)    {
        sstr << "uniform sampler2DShadow shadowTexture"    <<    i    <<"; "    << std::endl;
        sstr << "uniform float zShadow"                    <<    i    <<"; "    << std::endl;
    }




    sstr << "void main(void)"    << std::endl;
    sstr << "{"    << std::endl;


    /// select the shadow map : split
    sstr << "float testZ = gl_FragCoord.z*2.0-1.0;" <<std::endl;
    sstr << "float map0 = step(testZ, zShadow0);"<< std::endl;//DEBUG
    for (unsigned int i=1; i<nbrSplits; i++)    {
        sstr << "float map" << i << "  = step(zShadow"<<i-1<<",testZ)*step(testZ, zShadow"<<i<<");"<< std::endl;//DEBUG
    }

    if (filtered) {
        sstr << "          float fTexelSize="<< (1.41 / textureRes ) <<";" << std::endl;
        sstr << "          float fZOffSet  = -0.001954;" << std::endl; // 2^-9 good value for ATI / NVidia

    }
    for (unsigned int i=0; i<nbrSplits; i++)    {
        if (!filtered) {
            sstr << "    float shadow"    <<    i    <<" = shadow2D( shadowTexture"    <<    i    <<",gl_TexCoord["    <<    (i+textureOffset)    <<"].xyz).r;"   << std::endl;
            sstr << " shadow"    <<    i    <<" = step(0.25,shadow"    <<    i    <<");" << std::endl; // reduce shadow artefacts
        } else {


            // filter the shadow (look up) 3x3
            //
            // 1 0 1
            // 0 2 0
            // 1 0 1
            //
            // / 6

            sstr << "    float shadowOrg"    <<    i    <<" = shadow2D( shadowTexture"  <<    i    <<",gl_TexCoord["    <<    (i+textureOffset)    <<"].xyz+vec3(0.0,0.0,fZOffSet) ).r;"   << std::endl;
            sstr << "    float shadow0"    <<    i    <<" = shadow2D( shadowTexture"    <<    i    <<",gl_TexCoord["    <<    (i+textureOffset)    <<"].xyz+vec3(-fTexelSize,-fTexelSize,fZOffSet) ).r;"   << std::endl;
            sstr << "    float shadow1"    <<    i    <<" = shadow2D( shadowTexture"    <<    i    <<",gl_TexCoord["    <<    (i+textureOffset)    <<"].xyz+vec3( fTexelSize,-fTexelSize,fZOffSet) ).r;"   << std::endl;
            sstr << "    float shadow2"    <<    i    <<" = shadow2D( shadowTexture"    <<    i    <<",gl_TexCoord["    <<    (i+textureOffset)    <<"].xyz+vec3( fTexelSize, fTexelSize,fZOffSet) ).r;"   << std::endl;
            sstr << "    float shadow3"    <<    i    <<" = shadow2D( shadowTexture"    <<    i    <<",gl_TexCoord["    <<    (i+textureOffset)    <<"].xyz+vec3(-fTexelSize, fTexelSize,fZOffSet) ).r;"   << std::endl;

            sstr << "    float shadow"    <<    i    <<" = ( 2.0*shadowOrg"    <<    i
                 <<" + shadow0"    <<    i
                 <<" + shadow1"    <<    i
                 <<" + shadow2"    <<    i
                 <<" + shadow3"    <<    i
                 << ")/6.0;"<< std::endl;

            //sstr << " shadow"    <<    i    <<" = shadow"    <<    i    <<" * step(0.025,shadow"    <<    i    <<");" << std::endl; // reduce shadow artefacts

            //sstr << "    float shadow02"    <<    i    <<" = (shadow0"    <<    i    <<"+shadow2"    <<    i    <<")*0.5;"<< std::endl;
            //sstr << "    float shadow13"    <<    i    <<" = (shadow1"    <<    i    <<"+shadow3"    <<    i    <<")*0.5;"<< std::endl;
            //sstr << "    float shadowSoft"    <<    i    <<" = (shadow02"    <<    i    <<"+shadow13"    <<    i    <<")*0.5;"<< std::endl;
            //sstr << "    float shadow"    <<    i    <<" = (shadowSoft"    <<    i    <<"+shadowOrg"    <<    i    <<")*0.5;"<< std::endl;
            //sstr << " shadow"    <<    i    <<" = step(0.25,shadow"    <<    i    <<");" << std::endl; // reduce shadow artefacts
        }
    }


    sstr << "    float term0 = (1.0-shadow0)*map0; "    << std::endl;
    for (unsigned int i=1; i<nbrSplits; i++)    {
        sstr << "    float term" << i << " = map"<< i << "*(1.0-shadow"<<i<<");"<< std::endl;
    }



    /// build shadow factor value v
    sstr << "    float v = clamp(";
    for (unsigned int i=0; i<nbrSplits; i++)    {
        sstr << "term"    <<    i;
        if ( i+1 < nbrSplits ) {
            sstr << "+";
        }
    }
    sstr << ",0.0,1.0);"    << std::endl;




    if ( debug ) {


        sstr << "    float c0=0.0;" << std::endl;
        sstr << "    float c1=0.0;" << std::endl;
        sstr << "    float c2=0.0;" << std::endl;

        sstr << "    float sumTerm=0.0;" << std::endl;

        for (unsigned int i=0; i<nbrSplits; i++)    {
            if ( i < 3 ) sstr << "    c" << i << "=term" << i << ";" << std::endl;
            sstr << "    sumTerm=sumTerm+term" << i << ";" << std::endl;
        }

        sstr << "    vec4 color    = gl_Color*( 1.0 - sumTerm ) + (sumTerm)* gl_Color*vec4(c0,(1.0-c0)*c1,(1.0-c0)*(1.0-c1)*c2,1.0); "    << std::endl;


        switch(nbrSplits) {
        case 1:
            sstr << "    color    =  color*0.75 + vec4(map0,0,0,1.0)*0.25; "    << std::endl;
            break;
        case 2:
            sstr << "    color    =  color*0.75 + vec4(map0,map1,0,1.0)*0.25; "    << std::endl;
            break;
        case 3:
            sstr << "    color    =  color*0.75 + vec4(map0,map1,map2,1.0)*0.25; "    << std::endl;
            break;
        case 4:
            sstr << "    color    =  color*0.75 + vec4(map0+map3,map1+map3,map2,1.0)*0.25; "    << std::endl;
            break;
        case 5:
            sstr << "    color    =  color*0.75 + vec4(map0+map3,map1+map3+map4,map2+map4,1.0)*0.25; "    << std::endl;
            break;
        case 6:
            sstr << "    color    =  color*0.75 + vec4(map0+map3+map5,map1+map3+map4,map2+map4+map5,1.0)*0.25; "    << std::endl;
            break;
        default:
            break;
        }


    } else {
        sstr << "    vec4 color    = gl_Color; "<< std::endl;
    }
    sstr << "    vec4 texcolor = texture2D(baseTexture,gl_TexCoord[0].st); "    << std::endl;
    sstr << "    float enableBaseTextureFilter = enableBaseTexture*(1.0 - step(texcolor.x+texcolor.y+texcolor.z+texcolor.a,0.0)); "    << std::endl;                                                //18
    sstr << "    vec4 colorTex = color*texcolor;" << std::endl;
    sstr << "    gl_FragColor.rgb = (((color*(ambientBias.x+1.0)*(1.0-enableBaseTextureFilter)) + colorTex*(1.0+ambientBias.x)*enableBaseTextureFilter)*(1.0-ambientBias.y*v)).rgb; "<< std::endl;
    sstr << "    gl_FragColor.a = (color*(1.0-enableBaseTextureFilter) + colorTex*enableBaseTextureFilter).a; "<< std::endl;



    sstr << "}"<< std::endl;

    //std::cout << sstr.str() << std::endl;
    if ( splitCount == nbrSplits-1 ) {
        OSG_INFO << std::endl << "ParallelSplitShadowMap: GLSL shader code:" << std::endl << "-------------------------------------------------------------------"  << std::endl << sstr.str() << std::endl;
    }

    return sstr.str();
}

//////////////////////////////////////////////////////////////////////////
// clamp variables of any type
template<class Type> inline Type Clamp(Type A, Type Min, Type Max) {
    if(A<Min) return Min;
    if(A>Max) return Max;
    return A;
}

#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))

//////////////////////////////////////////////////////////////////////////
ParallelSplitShadowMap::ParallelSplitShadowMap(osg::Geode** gr, int icountplanes) :
    _textureUnitOffset(2),
    _debug_color_in_GLSL(false),
    _user_polgyonOffset_set(false),
    _resolution(TEXTURE_RESOLUTION),
    _setMaxFarDistance(1000.0),
    _isSetMaxFarDistance(false),
    _split_min_near_dist(ZNEAR_MIN_FROM_LIGHT_SOURCE),
    _move_vcam_behind_rcam_factor(MOVE_VIRTUAL_CAMERA_BEHIND_REAL_CAMERA_FACTOR),
    _userLight(NULL),
    _GLSL_shadow_filtered(true),
    _ambientBiasUniform(NULL),
    _ambientBias(0.1f,0.3f)
{
    _displayTexturesGroupingNode = gr;
    _number_of_splits = icountplanes;

    _polgyonOffset.set(0.0f,0.0f);
    setFragmentShaderGenerator(new FragmentShaderGenerator());
    setSplitCalculationMode(SPLIT_EXP);
}

ParallelSplitShadowMap::ParallelSplitShadowMap(const ParallelSplitShadowMap& copy, const osg::CopyOp& copyop):
    ShadowTechnique(copy,copyop),
    _displayTexturesGroupingNode(0),
    _textureUnitOffset(copy._textureUnitOffset),
    _number_of_splits(copy._number_of_splits),
    _debug_color_in_GLSL(copy._debug_color_in_GLSL),
    _polgyonOffset(copy._polgyonOffset),
    _user_polgyonOffset_set(copy._user_polgyonOffset_set),
    _resolution(copy._resolution),
    _setMaxFarDistance(copy._setMaxFarDistance),
    _isSetMaxFarDistance(copy._isSetMaxFarDistance),
    _split_min_near_dist(copy._split_min_near_dist),
    _move_vcam_behind_rcam_factor(copy._move_vcam_behind_rcam_factor),
    _userLight(copy._userLight),
    _FragmentShaderGenerator(copy._FragmentShaderGenerator),
    _GLSL_shadow_filtered(copy._GLSL_shadow_filtered),
    _SplitCalcMode(copy._SplitCalcMode),
    _ambientBiasUniform(NULL),
    _ambientBias(copy._ambientBias)
{
}

void ParallelSplitShadowMap::resizeGLObjectBuffers(unsigned int maxSize)
{
    for(PSSMShadowSplitTextureMap::iterator itr = _PSSMShadowSplitTextureMap.begin();
            itr != _PSSMShadowSplitTextureMap.end();
            ++itr)
    {
        itr->second.resizeGLObjectBuffers(maxSize);
    }
}

void ParallelSplitShadowMap::releaseGLObjects(osg::State* state) const
{
    for(PSSMShadowSplitTextureMap::const_iterator itr = _PSSMShadowSplitTextureMap.begin();
            itr != _PSSMShadowSplitTextureMap.end();
            ++itr)
    {
        itr->second.releaseGLObjects(state);
    }
}


void ParallelSplitShadowMap::PSSMShadowSplitTexture::resizeGLObjectBuffers(unsigned int maxSize)
{
#if OSG_VERSION_MAJOR >=3 && OSG_VERSION_MINOR >=6
    osg::resizeGLObjectBuffers(_camera, maxSize);
    osg::resizeGLObjectBuffers(_texture, maxSize);
    osg::resizeGLObjectBuffers(_stateset, maxSize);
    osg::resizeGLObjectBuffers(_debug_camera, maxSize);
    osg::resizeGLObjectBuffers(_debug_texture, maxSize);
    osg::resizeGLObjectBuffers(_debug_stateset, maxSize);
#endif
}

void ParallelSplitShadowMap::PSSMShadowSplitTexture::releaseGLObjects(osg::State* state) const
{

#if OSG_VERSION_MAJOR >=3 && OSG_VERSION_MINOR >=6
    osg::releaseGLObjects(_camera, state);
    osg::releaseGLObjects(_texture, state);
    osg::releaseGLObjects(_stateset, state);
    osg::releaseGLObjects(_debug_camera, state);
    osg::releaseGLObjects(_debug_texture, state);
    osg::releaseGLObjects(_debug_stateset, state);
#endif
}

void ParallelSplitShadowMap::setAmbientBias(const osg::Vec2& ambientBias)
{
    _ambientBias = ambientBias;
    if (_ambientBiasUniform ) _ambientBiasUniform->set(osg::Vec2f(_ambientBias.x(), _ambientBias.y()));
}

void ParallelSplitShadowMap::init()
{
    if (!_shadowedScene) return;

    osg::ref_ptr<osg::StateSet> sharedStateSet = new osg::StateSet;
    sharedStateSet->setDataVariance(osg::Object::DYNAMIC);

    unsigned int iCamerasMax=_number_of_splits;
    _iMVPT=new osg::Uniform(osg::Uniform::FLOAT_MAT4,"texgenmat",_number_of_splits);
    _cams.clear();
    for (unsigned int iCameras=0; iCameras<iCamerasMax; iCameras++)
    {
        PSSMShadowSplitTexture pssmShadowSplitTexture;
        pssmShadowSplitTexture._splitID = iCameras;
        pssmShadowSplitTexture._textureUnit = iCameras+_textureUnitOffset;

        pssmShadowSplitTexture._resolution = _resolution;

        OSG_DEBUG << "ParallelSplitShadowMap : Texture ID=" << iCameras << " Resolution=" << pssmShadowSplitTexture._resolution << std::endl;
        // set up the texture to render into
        {
            pssmShadowSplitTexture._texture = new osg::Texture2D;
            pssmShadowSplitTexture._texture->setTextureSize(pssmShadowSplitTexture._resolution, pssmShadowSplitTexture._resolution);
#ifndef SHADOW_TEXTURE_DEBUG
            pssmShadowSplitTexture._texture->setInternalFormat(GL_DEPTH_COMPONENT);
#ifndef GL3
             pssmShadowSplitTexture._texture->setShadowComparison(true);
            pssmShadowSplitTexture._texture->setShadowTextureMode(osg::Texture2D::LUMINANCE);
#endif
#else
            pssmShadowSplitTexture._texture->setInternalFormat(GL_RGBA);
#endif
            pssmShadowSplitTexture._texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
            pssmShadowSplitTexture._texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
            pssmShadowSplitTexture._texture->setBorderColor(osg::Vec4(1.0,1.0,1.0,1.0));
            pssmShadowSplitTexture._texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_BORDER);
            pssmShadowSplitTexture._texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_BORDER);
        }
        // set up the render to texture camera.
        {
            // create the camera
            pssmShadowSplitTexture._camera = new osg::Camera;

            pssmShadowSplitTexture._camera->setCullCallback(new CameraCullCallback(this));

            pssmShadowSplitTexture._camera->setReadBuffer(GL_BACK);

            pssmShadowSplitTexture._camera->setDrawBuffer(GL_BACK);
#ifndef SHADOW_TEXTURE_DEBUG
            pssmShadowSplitTexture._camera->setClearMask(GL_DEPTH_BUFFER_BIT);
           // pssmShadowSplitTexture._camera->setClearColor(osg::Vec4(1.0,1.0,1.0,1.0));
#else
            pssmShadowSplitTexture._camera->setClearMask(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
            switch(iCameras)
            {
            case 0:
                pssmShadowSplitTexture._camera->setClearColor(osg::Vec4(1.0,0.0,0.0,1.0));
                break;
            case 1:
                pssmShadowSplitTexture._camera->setClearColor(osg::Vec4(0.0,1.0,0.0,1.0));
                break;
            case 2:
                pssmShadowSplitTexture._camera->setClearColor(osg::Vec4(0.0,0.0,1.0,1.0));
                break;
            default:
                pssmShadowSplitTexture._camera->setClearColor(osg::Vec4(1.0,1.0,1.0,1.0));
                break;
            }
#endif
            pssmShadowSplitTexture._camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);// COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
            pssmShadowSplitTexture._camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF );

            // set viewport
            pssmShadowSplitTexture._camera->setViewport(0,0,pssmShadowSplitTexture._resolution,pssmShadowSplitTexture._resolution);

            // set the camera to render before the main camera.
            pssmShadowSplitTexture._camera->setRenderOrder(osg::Camera::PRE_RENDER,-100+pssmShadowSplitTexture._splitID);

            // tell the camera to use OpenGL frame buffer object where supported.
            pssmShadowSplitTexture._camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

            // attach the texture and use it as the color buffer.
#ifndef SHADOW_TEXTURE_DEBUG
            pssmShadowSplitTexture._camera->attach(osg::Camera::DEPTH_BUFFER, pssmShadowSplitTexture._texture.get());
#else
            pssmShadowSplitTexture._camera->attach(osg::Camera::COLOR_BUFFER, pssmShadowSplitTexture._texture.get());
#endif
            osg::StateSet* stateset = pssmShadowSplitTexture._camera->getOrCreateStateSet();


            /* try override only fs but fails
                        static osg::ref_ptr<osg::Program > _defaultdepthwriter;
                          if(!_defaultdepthwriter.valid()){

                                _defaultdepthwriter=new osg::Program();
                                _defaultdepthwriter->addShader(new osg::Shader(osg::Shader::FRAGMENT,"void main(){gl_FragDepth=gl_FragCoord.z;\ngl_FragColor=vec4(0);\n}"));
                            }
                           stateset->setAttributeAndModes(_defaultdepthwriter,osg::StateAttribute::OVERRIDE|osg::StateAttribute::PROTECTED);
                           */
            //////////////////////////////////////////////////////////////////////////
            if ( _user_polgyonOffset_set ) {
                float factor = _polgyonOffset.x();
                float units  = _polgyonOffset.y();
                osg::ref_ptr<osg::PolygonOffset> polygon_offset = new osg::PolygonOffset;
                polygon_offset->setFactor(factor);
                polygon_offset->setUnits(units);
                stateset->setAttribute(polygon_offset.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                stateset->setMode(GL_POLYGON_OFFSET_FILL, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }


            //////////////////////////////////////////////////////////////////////////
            if ( ! _GLSL_shadow_filtered ) {
                // if not glsl filtering enabled then we should force front face culling to reduce the number of shadow artefacts.
                osg::ref_ptr<osg::CullFace> cull_face = new osg::CullFace;
                cull_face->setMode(osg::CullFace::FRONT);
                stateset->setAttribute(cull_face.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }

            //////////////////////////////////////////////////////////////////////////
            osg::ShadeModel* shademodel = dynamic_cast<osg::ShadeModel*>(stateset->getAttribute(osg::StateAttribute::SHADEMODEL));
            if (!shademodel) {
                shademodel = new osg::ShadeModel;
                stateset->setAttribute(shademodel);
            }
            shademodel->setMode( osg::ShadeModel::FLAT );
            stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
        }

        //////////////////////////////////////////////////////////////////////////
        // set up stateset and append texture, texGen ,...
        {
            pssmShadowSplitTexture._stateset = sharedStateSet.get();//new osg::StateSet;
            pssmShadowSplitTexture._stateset->setTextureAttributeAndModes(pssmShadowSplitTexture._textureUnit,pssmShadowSplitTexture._texture.get(),osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            pssmShadowSplitTexture._stateset->setTextureMode(pssmShadowSplitTexture._textureUnit,GL_TEXTURE_GEN_S,osg::StateAttribute::ON);
            pssmShadowSplitTexture._stateset->setTextureMode(pssmShadowSplitTexture._textureUnit,GL_TEXTURE_GEN_T,osg::StateAttribute::ON);
            pssmShadowSplitTexture._stateset->setTextureMode(pssmShadowSplitTexture._textureUnit,GL_TEXTURE_GEN_R,osg::StateAttribute::ON);
            pssmShadowSplitTexture._stateset->setTextureMode(pssmShadowSplitTexture._textureUnit,GL_TEXTURE_GEN_Q,osg::StateAttribute::ON);


            pssmShadowSplitTexture._stateset->addUniform( _iMVPT);

            /// generate a TexGen object
            pssmShadowSplitTexture._texgen = new osg::TexGen;

        }

        //////////////////////////////////////////////////////////////////////////
        // set up shader (GLSL)
#ifdef SHADOW_TEXTURE_GLSL

        osg::Program* program = new osg::Program;
        pssmShadowSplitTexture._stateset->setAttribute(program);

        //////////////////////////////////////////////////////////////////////////
        // GLSL PROGRAMS
        osg::Shader* fragment_shader = new osg::Shader(osg::Shader::FRAGMENT,
                _FragmentShaderGenerator->generateGLSL_FragmentShader_BaseTex(
                    _debug_color_in_GLSL,
                    iCameras,
                    pssmShadowSplitTexture._resolution,
                    _GLSL_shadow_filtered,
                    _number_of_splits,
                    _textureUnitOffset
                ).c_str());
        program->addShader(fragment_shader);


        //////////////////////////////////////////////////////////////////////////
        // UNIFORMS
        std::stringstream strST;
        strST << "shadowTexture" << (pssmShadowSplitTexture._textureUnit-_textureUnitOffset);
        osg::Uniform* shadowTextureSampler = new osg::Uniform(strST.str().c_str(),(int)(pssmShadowSplitTexture._textureUnit));
        pssmShadowSplitTexture._stateset->addUniform(shadowTextureSampler);

        //TODO: NOT YET SUPPORTED in the current version of the shader
        if ( ! _ambientBiasUniform ) {
            _ambientBiasUniform = new osg::Uniform("ambientBias",_ambientBias);
            pssmShadowSplitTexture._stateset->addUniform(_ambientBiasUniform);
        }


        std::stringstream strzShadow;
        strzShadow << "zShadow" << (pssmShadowSplitTexture._textureUnit-_textureUnitOffset);
        pssmShadowSplitTexture._farDistanceSplit = new osg::Uniform(strzShadow.str().c_str(),1.0f);
        pssmShadowSplitTexture._stateset->addUniform(pssmShadowSplitTexture._farDistanceSplit);

        osg::Uniform* baseTextureSampler = new osg::Uniform("baseTexture",0);
        pssmShadowSplitTexture._stateset->addUniform(baseTextureSampler);

        osg::Uniform* randomTextureSampler = new osg::Uniform("randomTexture",(int)(_textureUnitOffset+_number_of_splits));
        pssmShadowSplitTexture._stateset->addUniform(randomTextureSampler);

        if ( _textureUnitOffset > 0 ) {
            osg::Uniform* enableBaseTexture = new osg::Uniform("enableBaseTexture",1.0f);
            pssmShadowSplitTexture._stateset->addUniform(enableBaseTexture);
        } else {
            osg::Uniform* enableBaseTexture = new osg::Uniform("enableBaseTexture",0.0f);
            pssmShadowSplitTexture._stateset->addUniform(enableBaseTexture);
        }

        for (unsigned int textLoop(0); textLoop<_textureUnitOffset; textLoop++)
        {
            // fake texture for baseTexture, add a fake texture
            // we support by default at least one texture layer
            // without this fake texture we can not support
            // textured and not textured scene

            // TODO: at the moment the PSSM supports just one texture layer in the GLSL shader, multitexture are
            //       not yet supported !

            osg::Image* image = new osg::Image;
            // allocate the image data, noPixels x 1 x 1 with 4 rgba floats - equivalent to a Vec4!
            int noPixels = 1;
            image->allocateImage(noPixels,1,1,GL_RGBA,GL_FLOAT);
            image->setInternalTextureFormat(GL_RGBA);
            // fill in the image data.
            osg::Vec4* dataPtr = (osg::Vec4*)image->data();
            osg::Vec4f color(1.0f,1.0f,1.0f,0.0f);
            *dataPtr = color;
            // make fake texture
            osg::Texture2D* texture = new osg::Texture2D;
            texture->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_BORDER);
            texture->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_BORDER);
            texture->setBorderColor(osg::Vec4(1.0,1.0,1.0,1.0));
            texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
            texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
            texture->setImage(image);
            // add fake texture
            pssmShadowSplitTexture._stateset->setTextureAttribute(textLoop,texture,osg::StateAttribute::ON);
            pssmShadowSplitTexture._stateset->setTextureMode(textLoop,GL_TEXTURE_1D,osg::StateAttribute::OFF);
            pssmShadowSplitTexture._stateset->setTextureMode(textLoop,GL_TEXTURE_2D,osg::StateAttribute::ON);
            pssmShadowSplitTexture._stateset->setTextureMode(textLoop,GL_TEXTURE_3D,osg::StateAttribute::OFF);
        }
#endif


        //////////////////////////////////////////////////////////////////////////
        // DEBUG
        if ( _displayTexturesGroupingNode ) {
            {
                pssmShadowSplitTexture._debug_textureUnit = 1;
                pssmShadowSplitTexture._debug_texture = new osg::Texture2D;
                pssmShadowSplitTexture._debug_texture->setTextureSize(TEXTURE_RESOLUTION, TEXTURE_RESOLUTION);
#ifdef SHOW_SHADOW_TEXTURE_DEBUG
                pssmShadowSplitTexture._debug_texture->setInternalFormat(GL_DEPTH_COMPONENT);
                pssmShadowSplitTexture._debug_texture->setShadowTextureMode(osg::Texture2D::LUMINANCE);
#else
                pssmShadowSplitTexture._debug_texture->setInternalFormat(GL_RGBA);
#endif
                pssmShadowSplitTexture._debug_texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
                pssmShadowSplitTexture._debug_texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
                // create the camera
                pssmShadowSplitTexture._debug_camera = new osg::Camera;
                pssmShadowSplitTexture._debug_camera->setCullCallback(new CameraCullCallback(this));
                pssmShadowSplitTexture._debug_camera->setClearMask(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
                pssmShadowSplitTexture._debug_camera->setClearColor(osg::Vec4(1.0,1.0,1.0,1.0));
                pssmShadowSplitTexture._debug_camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);

                // set viewport
                pssmShadowSplitTexture._debug_camera->setViewport(0,0,TEXTURE_RESOLUTION,TEXTURE_RESOLUTION);
                // set the camera to render before the main camera.
                pssmShadowSplitTexture._debug_camera->setRenderOrder(osg::Camera::PRE_RENDER);
                // tell the camera to use OpenGL frame buffer object where supported.
                pssmShadowSplitTexture._debug_camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
                // attach the texture and use it as the color buffer.
#ifdef SHOW_SHADOW_TEXTURE_DEBUG
                pssmShadowSplitTexture._debug_camera->attach(osg::Camera::DEPTH_BUFFER, pssmShadowSplitTexture._debug_texture.get());
#else
                pssmShadowSplitTexture._debug_camera->attach(osg::Camera::COLOR_BUFFER, pssmShadowSplitTexture._debug_texture.get());
#endif
                // osg::StateSet* stateset = pssmShadowSplitTexture._debug_camera->getOrCreateStateSet();

                pssmShadowSplitTexture._debug_stateset = new osg::StateSet;
                pssmShadowSplitTexture._debug_stateset->setTextureAttributeAndModes(pssmShadowSplitTexture._debug_textureUnit,pssmShadowSplitTexture._debug_texture.get(),osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                pssmShadowSplitTexture._debug_stateset->setTextureMode(pssmShadowSplitTexture._debug_textureUnit,GL_TEXTURE_GEN_S,osg::StateAttribute::ON);
                pssmShadowSplitTexture._debug_stateset->setTextureMode(pssmShadowSplitTexture._debug_textureUnit,GL_TEXTURE_GEN_T,osg::StateAttribute::ON);
                pssmShadowSplitTexture._debug_stateset->setTextureMode(pssmShadowSplitTexture._debug_textureUnit,GL_TEXTURE_GEN_R,osg::StateAttribute::ON);
                pssmShadowSplitTexture._debug_stateset->setTextureMode(pssmShadowSplitTexture._debug_textureUnit,GL_TEXTURE_GEN_Q,osg::StateAttribute::ON);
            }

            osg::Geode* geode = _displayTexturesGroupingNode[iCameras];
            geode->getOrCreateStateSet()->setTextureAttributeAndModes(0,pssmShadowSplitTexture._debug_texture.get(),osg::StateAttribute::ON);

        }
        //////////////////////////////////////////////////////////////////////////

        _PSSMShadowSplitTextureMap.insert(PSSMShadowSplitTextureMap::value_type(iCameras,pssmShadowSplitTexture));

        _cams.push_back( pssmShadowSplitTexture._camera);
    }

    _dirty = false;
}

void ParallelSplitShadowMap::update(osg::NodeVisitor& nv) {
    getShadowedScene()->osg::Group::traverse(nv);
}
//////////////////////////////////////////////////////////////////////////
// Computes corner points of a frustum
//
//
//unit box representing frustum in clip space
const osg::Vec3d const_pointFarTR(   1.0,  1.0,  1.0);
const osg::Vec3d const_pointFarBR(   1.0, -1.0,  1.0);
const osg::Vec3d const_pointFarTL(  -1.0,  1.0,  1.0);
const osg::Vec3d const_pointFarBL(  -1.0, -1.0,  1.0);
const osg::Vec3d const_pointNearTR(  1.0,  1.0, -1.0);
const osg::Vec3d const_pointNearBR(  1.0, -1.0, -1.0);
const osg::Vec3d const_pointNearTL( -1.0,  1.0, -1.0);
const osg::Vec3d const_pointNearBL( -1.0, -1.0, -1.0);

void ParallelSplitShadowMap::cull(osgUtil::CullVisitor& cv) {
    // record the traversal mask on entry so we can reapply it later.
    unsigned int traversalMask = cv.getTraversalMask();
    osgUtil::RenderStage* orig_rs = cv.getRenderStage();




    //////////////////////////////////////////////////////////////////////////
    const osg::Light* selectLight = 0;

    /// light pos and light direction
    osg::Vec4 lightpos;
    osg::Vec3 lightDirection;

    if ( ! _userLight ) {
        // try to find a light in the scene
        osgUtil::PositionalStateContainer::AttrMatrixList& aml = orig_rs->getPositionalStateContainer()->getAttrMatrixList();
        for(osgUtil::PositionalStateContainer::AttrMatrixList::iterator itr = aml.begin();
                itr != aml.end();
                ++itr)
        {
            const osg::Light* light = dynamic_cast<const osg::Light*>(itr->first.get());
            if (light)
            {
                osg::RefMatrix* matrix = itr->second.get();
                if (matrix) lightpos = light->getPosition() * (*matrix);
                else lightpos = light->getPosition();
                if (matrix) lightDirection = light->getDirection() * (*matrix);
                else lightDirection = light->getDirection();

                selectLight = light;
            }
        }

        osg::Matrix eyeToWorld;
        eyeToWorld.invert(*cv.getModelViewMatrix());

        lightpos = lightpos * eyeToWorld;
        lightDirection = lightDirection * eyeToWorld;
    } else {
        // take the user light as light source
        lightpos = _userLight->getPosition();
        lightDirection = _userLight->getDirection();
        selectLight = _userLight.get();
    }


    cv.setTraversalMask( traversalMask&  getShadowedScene()->getCastsShadowTraversalMask() );
    if (selectLight)
    {
        osg::Vec3d pCorners[8];
        lightDirection.normalize();

        for(PSSMShadowSplitTextureMap::iterator it=_PSSMShadowSplitTextureMap.begin(); it!=_PSSMShadowSplitTextureMap.end(); it++)
        {
            PSSMShadowSplitTexture &pssmShadowSplitTexture = it->second;


            //////////////////////////////////////////////////////////////////////////
            // SETUP pssmShadowSplitTexture for rendering
            //
            pssmShadowSplitTexture._lightDirection = lightDirection;
             pssmShadowSplitTexture._cameraView    = cv.getRenderInfo().getView()->getCamera()->getViewMatrix();
            pssmShadowSplitTexture._cameraProj    = cv.getRenderInfo().getView()->getCamera()->getProjectionMatrix();

            //////////////////////////////////////////////////////////////////////////
            // CALCULATE

            calculateFrustumCorners(pssmShadowSplitTexture,pCorners);

            // Init Light (Directional Light)
            //
            calculateLightInitialPosition(pssmShadowSplitTexture,pCorners);

            // Calculate near and far for light view
            //
            calculateLightNearFarFormFrustum(pssmShadowSplitTexture,pCorners);
            // Calculate view and projection matrices
            //
            calculateLightViewProjectionFormFrustum(pssmShadowSplitTexture,pCorners);

            //////////////////////////////////////////////////////////////////////////
            // set up shadow rendering camera

            //////////////////////////////////////////////////////////////////////////
            // DEBUG
            if ( _displayTexturesGroupingNode ) {
                pssmShadowSplitTexture._debug_camera->setViewMatrix(pssmShadowSplitTexture._camera->getViewMatrix());
                pssmShadowSplitTexture._debug_camera->setProjectionMatrix(pssmShadowSplitTexture._camera->getProjectionMatrix());
                pssmShadowSplitTexture._debug_camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
            }

            //////////////////////////////////////////////////////////////////////////
            // compute the matrix which takes a vertex from local coords into tex coords
            // will use this later to specify osg::TexGen..

            osg::Matrix MVPT = pssmShadowSplitTexture._camera->getViewMatrix() *
                               pssmShadowSplitTexture._camera->getProjectionMatrix() *
                               osg::Matrix::translate(1.0,1.0,1.0) *
                               osg::Matrix::scale(0.5,0.5,0.5);

            pssmShadowSplitTexture._texgen->setMode(osg::TexGen::EYE_LINEAR);
            pssmShadowSplitTexture._texgen->setPlanesFromMatrix(MVPT);
            //  MVPT.invert(MVPT);
            osg::Matrix mv=* cv.getModelViewMatrix();
            mv.invert(mv);
            MVPT =( mv* MVPT) ;
            _iMVPT->setElement(it->first,MVPT);

            //////////////////////////////////////////////////////////////////////////

            // do RTT camera traversal
             pssmShadowSplitTexture._camera->accept(cv);

            //////////////////////////////////////////////////////////////////////////
            // DEBUG
            if ( _displayTexturesGroupingNode ) {
                pssmShadowSplitTexture._debug_camera->accept(cv);
            }

#ifndef GL3
            orig_rs->getPositionalStateContainer()->addPositionedTextureAttribute(pssmShadowSplitTexture._textureUnit, cv.getModelViewMatrix(), pssmShadowSplitTexture._texgen.get());
#endif

        }
    } // if light
    cv.setTraversalMask(traversalMask&getShadowedScene()->getShadowSettings()->getReceivesShadowTraversalMask());
#ifdef SHADOW_TEXTURE_GLSL
    PSSMShadowSplitTextureMap::iterator tm_itr=_PSSMShadowSplitTextureMap.begin();
#else
    // do traversal of shadow receiving scene which does need to be decorated by the shadow map
    for (PSSMShadowSplitTextureMap::iterator tm_itr=_PSSMShadowSplitTextureMap.begin();it!=_PSSMShadowSplitTextureMap.end();it++)
#endif
    {
        PSSMShadowSplitTexture pssmShadowSplitTexture = tm_itr->second;
        cv.pushStateSet(pssmShadowSplitTexture._stateset.get());

        //////////////////////////////////////////////////////////////////////////
        // DEBUG
        if ( _displayTexturesGroupingNode ) {
            cv.pushStateSet(pssmShadowSplitTexture._debug_stateset.get());
        }
        //////////////////////////////////////////////////////////////////////////

        _shadowedScene->osg::Group::traverse(cv);

        cv.popStateSet();

    }
    // reapply the original traversal mask
    cv.setTraversalMask( traversalMask );
}

void ParallelSplitShadowMap::cleanSceneGraph() {

}



//////////////////////////////////////////////////////////////////////////
void ParallelSplitShadowMap::calculateFrustumCorners(PSSMShadowSplitTexture &pssmShadowSplitTexture, osg::Vec3d *frustumCorners)
{
    // get user cameras
    double fovy,aspectRatio,camNear,camFar;
    pssmShadowSplitTexture._cameraProj.getPerspective(fovy,aspectRatio,camNear,camFar);


    // force to max far distance to show shadow, for some scene it can be solve performance problems.
    if ((_isSetMaxFarDistance) && (_setMaxFarDistance < camFar))
        camFar = _setMaxFarDistance;


    // build camera matrix with some offsets (the user view camera)
    osg::Matrixd viewMat;
    osg::Vec3d camEye,camCenter,camUp;
    pssmShadowSplitTexture._cameraView.getLookAt(camEye,camCenter,camUp);
    osg::Vec3d viewDir = camCenter - camEye;
    viewDir.normalize(); //we can assume that viewDir is still normalized in the viewMatrix
    camEye = camEye  - viewDir * _move_vcam_behind_rcam_factor;
    camFar += _move_vcam_behind_rcam_factor * viewDir.length();
    viewMat.makeLookAt(camEye,camCenter,camUp);



    //////////////////////////////////////////////////////////////////////////
    /// CALCULATE SPLIT
    double maxFar = camFar;
    // double minNear = camNear;
    double camNearFar_Dist = maxFar - camNear;
    if ( _SplitCalcMode == SPLIT_LINEAR )
    {
        camFar  = camNear + (camNearFar_Dist)  * ((double)(pssmShadowSplitTexture._splitID+1))/((double)(_number_of_splits));
        camNear = camNear + (camNearFar_Dist) * ((double)(pssmShadowSplitTexture._splitID))/((double)(_number_of_splits));
    }
    else
    {
        // Exponential split scheme:
        //
        // Ci = (n - f)*(i/numsplits)^(bias+1) + n;
        //
        static double fSplitSchemeBias[2]= {0.25f,0.66f};
        fSplitSchemeBias[1]=Clamp(fSplitSchemeBias[1],0.0,3.0);
        double* pSplitDistances =new double[_number_of_splits+1];

        for(int i=0; i<(int)_number_of_splits; i++)
        {
            double fIDM=(double)(i)/(double)(_number_of_splits);
            pSplitDistances[i]=camNearFar_Dist*(pow(fIDM,fSplitSchemeBias[1]+1))+camNear;
        }
        // make sure border values are right
        pSplitDistances[0]=camNear;
        pSplitDistances[_number_of_splits]=camFar;

        camNear = pSplitDistances[pssmShadowSplitTexture._splitID];
        camFar  = pSplitDistances[pssmShadowSplitTexture._splitID+1];

        delete[] pSplitDistances;
    }


    pssmShadowSplitTexture._split_far = camFar;


    //////////////////////////////////////////////////////////////////////////
    /// TRANSFORM frustum corners (Optimized for Orthogonal)


    osg::Matrixd projMat;
    projMat.makePerspective(fovy,aspectRatio,camNear,camFar);
    osg::Matrixd projViewMat(viewMat*projMat);
    osg::Matrixd invProjViewMat;
    invProjViewMat.invert(projViewMat);

    //transform frustum vertices to world space
    frustumCorners[0] = const_pointFarBR * invProjViewMat;
    frustumCorners[1] = const_pointNearBR* invProjViewMat;
    frustumCorners[2] = const_pointNearTR* invProjViewMat;
    frustumCorners[3] = const_pointFarTR * invProjViewMat;
    frustumCorners[4] = const_pointFarTL * invProjViewMat;
    frustumCorners[5] = const_pointFarBL * invProjViewMat;
    frustumCorners[6] = const_pointNearBL* invProjViewMat;
    frustumCorners[7] = const_pointNearTL* invProjViewMat;

    //std::cout << "camFar : "<<pssmShadowSplitTexture._splitID << " / " << camNear << "," << camFar << std::endl;
}

//////////////////////////////////////////////////////////////////////////
//
// compute directional light initial position;
void ParallelSplitShadowMap::calculateLightInitialPosition(PSSMShadowSplitTexture &pssmShadowSplitTexture,osg::Vec3d *frustumCorners)
{
    pssmShadowSplitTexture._frustumSplitCenter = frustumCorners[0];
    for(int i=1; i<8; i++)
    {
        pssmShadowSplitTexture._frustumSplitCenter +=frustumCorners[i];
    }
    //    pssmShadowSplitTexture._frustumSplitCenter /= 8.0;
    pssmShadowSplitTexture._frustumSplitCenter *= 0.125;
}

void ParallelSplitShadowMap::calculateLightNearFarFormFrustum(
    PSSMShadowSplitTexture &pssmShadowSplitTexture,
    osg::Vec3d *frustumCorners
) {

    //calculate near, far
    double zFar(-DBL_MAX);

    // calculate zFar (as longest distance)
    for(int i=0; i<8; i++) {
        double dist_z_from_light = fabs(pssmShadowSplitTexture._lightDirection*(frustumCorners[i] -  pssmShadowSplitTexture._frustumSplitCenter));
        if ( zFar  < dist_z_from_light ) zFar  = dist_z_from_light;
    }

    // update camera position and look at center
    pssmShadowSplitTexture._lightCameraSource = pssmShadowSplitTexture._frustumSplitCenter - pssmShadowSplitTexture._lightDirection*(zFar+_split_min_near_dist);
    pssmShadowSplitTexture._lightCameraTarget = pssmShadowSplitTexture._frustumSplitCenter + pssmShadowSplitTexture._lightDirection*(zFar);

    // calculate [zNear,zFar]
    zFar = (-DBL_MAX);
    double zNear(DBL_MAX);
    for(int i=0; i<8; i++) {
        double dist_z_from_light = fabs(pssmShadowSplitTexture._lightDirection*(frustumCorners[i] -  pssmShadowSplitTexture._lightCameraSource));
        if ( zFar  < dist_z_from_light ) zFar  = dist_z_from_light;
        if ( zNear > dist_z_from_light ) zNear  = dist_z_from_light;
    }
    // update near - far plane
    pssmShadowSplitTexture._lightNear =  max(zNear - _split_min_near_dist - 0.01,0.01);
    pssmShadowSplitTexture._lightFar  =  zFar;
}

void ParallelSplitShadowMap::calculateLightViewProjectionFormFrustum(PSSMShadowSplitTexture &pssmShadowSplitTexture,osg::Vec3d *frustumCorners)
{

    // calculate the camera's coordinate system
    osg::Vec3d camEye,camCenter,camUp;
    pssmShadowSplitTexture._cameraView.getLookAt(camEye,camCenter,camUp);
    osg::Vec3d viewDir(camCenter-camEye);
    osg::Vec3d camRight(viewDir^camUp);

    // we force to have normalized vectors (camera's view)
    camUp.normalize();
    viewDir.normalize();
    camRight.normalize();

    // use quaternion -> numerical more robust
    osg::Quat qRot;
    qRot.makeRotate(viewDir,pssmShadowSplitTexture._lightDirection);
    osg::Vec3d top =  qRot * camUp;
    osg::Vec3d right = qRot * camRight;

    // calculate the camera's frustum right,right,bottom,top parameters
    double maxRight(-DBL_MAX),maxTop(-DBL_MAX);
    double minRight(DBL_MAX),minTop(DBL_MAX);

    for(int i(0); i < 8; i++)
    {
        osg::Vec3d diffCorner(frustumCorners[i] - pssmShadowSplitTexture._frustumSplitCenter);
        double lright(diffCorner*right);
        double lTop(diffCorner*top);

        if ( lright > maxRight ) maxRight  =  lright;
        if ( lTop  > maxTop  ) maxTop   =  lTop;

        if ( lright < minRight ) minRight  =  lright;
        if ( lTop  < minTop  ) minTop   =  lTop;
    }

    // make the camera view matrix
    pssmShadowSplitTexture._camera->setViewMatrixAsLookAt(pssmShadowSplitTexture._lightCameraSource,pssmShadowSplitTexture._lightCameraTarget,top);

    // use ortho projection for light (directional light only supported)
    pssmShadowSplitTexture._camera->setProjectionMatrixAsOrtho(minRight,maxRight,minTop,maxTop,pssmShadowSplitTexture._lightNear,pssmShadowSplitTexture._lightFar);


#ifdef SHADOW_TEXTURE_GLSL
    // get user cameras
    osg::Vec3d vProjCamFraValue = (camEye + viewDir * pssmShadowSplitTexture._split_far) * (pssmShadowSplitTexture._cameraView * pssmShadowSplitTexture._cameraProj);
    pssmShadowSplitTexture._farDistanceSplit->set((float)vProjCamFraValue.z());
#endif


}



