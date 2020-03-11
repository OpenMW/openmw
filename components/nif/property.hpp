/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (property.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  https://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_PROPERTY_HPP
#define OPENMW_COMPONENTS_NIF_PROPERTY_HPP

#include "base.hpp"

namespace Nif
{

class Property : public Named
{
public:
    // The meaning of these depends on the actual property type.
    unsigned int flags;

    void read(NIFStream *nif);
};

class NiTexturingProperty : public Property
{
public:
    // A sub-texture
    struct Texture
    {
        /* Clamp mode
        0 - clampS clampT
        1 - clampS wrapT
        2 - wrapS clampT
        3 - wrapS wrapT
        */

        bool inUse;
        NiSourceTexturePtr texture;

        unsigned int clamp, uvSet;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    /* Apply mode:
        0 - replace
        1 - decal
        2 - modulate
        3 - hilight  // These two are for PS2 only?
        4 - hilight2
    */
    unsigned int apply;

    /*
     * The textures in this list are as follows:
     *
     * 0 - Base texture
     * 1 - Dark texture
     * 2 - Detail texture
     * 3 - Gloss texture
     * 4 - Glow texture
     * 5 - Bump map texture
     * 6 - Decal texture
     */
    enum TextureType
    {
        BaseTexture = 0,
        DarkTexture = 1,
        DetailTexture = 2,
        GlossTexture = 3,
        GlowTexture = 4,
        BumpTexture = 5,
        DecalTexture = 6,
    };

    std::vector<Texture> textures;

    osg::Vec2f envMapLumaBias;
    osg::Vec4f bumpMapMatrix;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiFogProperty : public Property
{
public:
    float mFogDepth;
    osg::Vec3f mColour;

    void read(NIFStream *nif);
};

// These contain no other data than the 'flags' field in Property
class NiShadeProperty : public Property { };
class NiDitherProperty : public Property { };
class NiZBufferProperty : public Property { };
class NiSpecularProperty : public Property { };
class NiWireframeProperty : public Property { };

// The rest are all struct-based
template <typename T>
struct StructPropT : Property
{
    T data;

    void read(NIFStream *nif)
    {
        Property::read(nif);
        data.read(nif);
    }
};

struct S_MaterialProperty
{
    // The vector components are R,G,B
    osg::Vec3f ambient, diffuse, specular, emissive;
    float glossiness, alpha;

    void read(NIFStream *nif);
};

struct S_VertexColorProperty
{
    /* Vertex mode:
        0 - source ignore
        1 - source emmisive
        2 - source amb diff

        Lighting mode
        0 - lighting emmisive
        1 - lighting emmisive ambient/diffuse
    */
    int vertmode, lightmode;

    void read(NIFStream *nif);
};

struct S_AlphaProperty
{
    /*
        In NiAlphaProperty, the flags have the following meaning:

        Bit 0 : alpha blending enable
        Bits 1-4 : source blend mode
        Bits 5-8 : destination blend mode
        Bit 9 : alpha test enable
        Bit 10-12 : alpha test mode
        Bit 13 : no sorter flag ( disables triangle sorting )

        blend modes (glBlendFunc):
        0000 GL_ONE
        0001 GL_ZERO
        0010 GL_SRC_COLOR
        0011 GL_ONE_MINUS_SRC_COLOR
        0100 GL_DST_COLOR
        0101 GL_ONE_MINUS_DST_COLOR
        0110 GL_SRC_ALPHA
        0111 GL_ONE_MINUS_SRC_ALPHA
        1000 GL_DST_ALPHA
        1001 GL_ONE_MINUS_DST_ALPHA
        1010 GL_SRC_ALPHA_SATURATE

        test modes (glAlphaFunc):
        000 GL_ALWAYS
        001 GL_LESS
        010 GL_EQUAL
        011 GL_LEQUAL
        100 GL_GREATER
        101 GL_NOTEQUAL
        110 GL_GEQUAL
        111 GL_NEVER

        Taken from:
        http://niftools.sourceforge.net/doc/nif/NiAlphaProperty.html
    */

    // Tested against when certain flags are set (see above.)
    unsigned char threshold;

    void read(NIFStream *nif);
};

/*
    Docs taken from:
    http://niftools.sourceforge.net/doc/nif/NiStencilProperty.html
 */
struct S_StencilProperty
{
    // Is stencil test enabled?
    unsigned char enabled;

    /*
        0   TEST_NEVER
        1   TEST_LESS
        2   TEST_EQUAL
        3   TEST_LESS_EQUAL
        4   TEST_GREATER
        5   TEST_NOT_EQUAL
        6   TEST_GREATER_EQUAL
        7   TEST_NEVER (though nifskope comment says TEST_ALWAYS, but ingame it is TEST_NEVER)
     */
    int compareFunc;
    unsigned stencilRef;
    unsigned stencilMask;
    /*
        Stencil test fail action, depth test fail action and depth test pass action:
        0   ACTION_KEEP
        1   ACTION_ZERO
        2   ACTION_REPLACE
        3   ACTION_INCREMENT
        4   ACTION_DECREMENT
        5   ACTION_INVERT
     */
    int failAction;
    int zFailAction;
    int zPassAction;
    /*
        Face draw mode:
        0   DRAW_CCW_OR_BOTH
        1   DRAW_CCW        [default]
        2   DRAW_CW
        3   DRAW_BOTH
     */
    int drawMode;

    void read(NIFStream *nif);
};

class NiAlphaProperty : public StructPropT<S_AlphaProperty> { };
class NiMaterialProperty : public StructPropT<S_MaterialProperty> { };
class NiVertexColorProperty : public StructPropT<S_VertexColorProperty> { };
class NiStencilProperty : public StructPropT<S_StencilProperty> { };

} // Namespace
#endif
