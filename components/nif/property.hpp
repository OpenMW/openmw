/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

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
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_PROPERTY_HPP
#define OPENMW_COMPONENTS_NIF_PROPERTY_HPP

#include "controlled.hpp"

namespace Nif
{

class Property : public Named
{
public:
    // The meaning of these depends on the actual property type.
    int flags;

    void read(NIFStream *nif)
    {
        Named::read(nif);
        flags = nif->getUShort();
    }
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

        /* Filter:
        0 - nearest
        1 - bilinear
        2 - trilinear
        3, 4, 5 - who knows
        */
        bool inUse;
        NiSourceTexturePtr texture;

        int clamp, set, filter;
        short unknown2;

        void read(NIFStream *nif)
        {
            inUse = !!nif->getInt();
            if(!inUse) return;

            texture.read(nif);
            clamp = nif->getInt();
            filter = nif->getInt();
            set = nif->getInt();

            // I have no idea, but I think these are actually two
            // PS2-specific shorts (ps2L and ps2K), followed by an unknown
            // short.
            nif->skip(6);
        }

        void post(NIFFile *nif)
        {
            texture.post(nif);
        }
    };

    /* Apply mode:
        0 - replace
        1 - decal
        2 - modulate
        3 - hilight  // These two are for PS2 only?
        4 - hilight2
    */
    int apply;

    /*
     * The textures in this list are as follows:
     *
     * 0 - Base texture
     * 1 - Dark texture
     * 2 - Detail texture
     * 3 - Gloss texture (never used?)
     * 4 - Glow texture
     * 5 - Bump map texture
     * 6 - Decal texture
     */
    Texture textures[7];

    void read(NIFStream *nif)
    {
        Property::read(nif);
        apply = nif->getInt();

        // Unknown, always 7. Probably the number of textures to read
        // below
        nif->getInt();

        textures[0].read(nif); // Base
        textures[1].read(nif); // Dark
        textures[2].read(nif); // Detail
        textures[3].read(nif); // Gloss (never present)
        textures[4].read(nif); // Glow
        textures[5].read(nif); // Bump map
        if(textures[5].inUse)
        {
            // Ignore these at the moment
            /*float lumaScale =*/ nif->getFloat();
            /*float lumaOffset =*/ nif->getFloat();
            /*const Vector4 *lumaMatrix =*/ nif->getVector4();
        }
        textures[6].read(nif); // Decal
    }

    void post(NIFFile *nif)
    {
        Property::post(nif);
        for(int i = 0;i < 7;i++)
            textures[i].post(nif);
    }
};

// These contain no other data than the 'flags' field in Property
typedef Property NiShadeProperty;
typedef Property NiDitherProperty;
typedef Property NiZBufferProperty;
typedef Property NiSpecularProperty;
typedef Property NiWireframeProperty;

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
    Ogre::Vector3 ambient, diffuse, specular, emissive;
    float glossiness, alpha;

    void read(NIFStream *nif)
    {
        ambient = nif->getVector3();
        diffuse = nif->getVector3();
        specular = nif->getVector3();
        emissive = nif->getVector3();
        glossiness = nif->getFloat();
        alpha = nif->getFloat();
    }
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

    void read(NIFStream *nif)
    {
        vertmode = nif->getInt();
        lightmode = nif->getInt();
    }
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

        Right now we only use standard alpha blending (see the Ogre code
        that sets it up) and it appears that this is the only blending
        used in the original game. Bloodmoon (along with several mods) do
        however use other settings, such as discarding pixel values with
        alpha < 1.0. This is faster because we don't have to mess with the
        depth stuff like we did for blending. And OGRE has settings for
        this too.
    */

    // Tested against when certain flags are set (see above.)
    unsigned char threshold;

    void read(NIFStream *nif)
    {
        threshold = nif->getChar();
    }
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
        7   TEST_ALWAYS
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

    void read(NIFStream *nif)
    {
        enabled = nif->getChar();
        compareFunc = nif->getInt();
        stencilRef = nif->getUInt();
        stencilMask = nif->getUInt();
        failAction = nif->getInt();
        zFailAction = nif->getInt();
        zPassAction = nif->getInt();
        drawMode = nif->getInt();
    }
};

typedef StructPropT<S_AlphaProperty> NiAlphaProperty;
typedef StructPropT<S_MaterialProperty> NiMaterialProperty;
typedef StructPropT<S_VertexColorProperty> NiVertexColorProperty;
typedef StructPropT<S_StencilProperty> NiStencilProperty;

} // Namespace
#endif
