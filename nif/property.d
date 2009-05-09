/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (property.d) is part of the OpenMW package.

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

module nif.property;
import nif.record;

abstract class Property : Named
{
  // The meaning of these depends on the actual property type.
  ushort flags;

 override:
  void read()
    {
      super.read();
      flags = nifFile.getUshort;
      debug(verbose) writefln("Flags: %x", flags);
    }
}

// Check the specs on this one again
class NiTexturingProperty : Property
{
  /* Apply mode:
     0 - replace
     1 - decal
     2 - modulate
     3 - hilight  // These two are for PS2 only?
     4 - hilight2
  */
  int apply;

  struct Texture
  {
    bool inUse;
    NiSourceTexture texture;

    /* Clamp mode (i don't understand this)
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

    int clamp, set, filter;
    short ps2L, ps2K, unknown2;

    void read(NiTexturingProperty caller)
    {
      if(nifFile.getInt == 0)
	{
	  debug(verbose) writefln("No texture");
	  return;
	}

      inUse = 1;

      debug(verbose) writef("  Texture ");
      caller.getIndex();

      clamp = nifFile.getIntIs(0,1,2,3);
      filter = nifFile.getIntIs(0,1,2);
      set = nifFile.getInt;

      ps2L = nifFile.getShortIs(0);
      ps2K = nifFile.getShortIs(-75,-2);

      debug(verbose)
	{
	  writefln("  Clamp ", clamp);
	  writefln("  Filter ", filter);
	  writefln("  Set? ", set);
	  writefln("  ps2L ", ps2L);
	  writefln("  ps2K ", ps2K);
	}

      unknown2 = nifFile.wgetShortIs(0,257);
    }
  }

  /*
   * The textures in this list is as follows:
   *
   * 0 - Base texture
   * 1 - Dark texture
   * 2 - Detail texture
   * 3 - Gloss texture (never used?)
   * 4 - Glow texture
   * 5 - Bump map texture
   * 6 - Decal texture
   */
  Texture[7] textures;

  override:
  void read()
    {
      super.read();
      if(flags > 1) nifFile.warn("Unknown flags");

      apply = nifFile.getInt;
      debug(verbose) writefln("Apply mode: ", apply);
      nifFile.getIntIs(7);

      textures[0].read(this); // Base
      textures[1].read(this); // Dark
      textures[2].read(this); // Detail

      nifFile.getIntIs(0); // Gloss

      textures[4].read(this); // Glow
      textures[5].read(this); // Bump map

      if(textures[5].inUse)
	{
	  float lumaScale = nifFile.wgetFloat;
	  float lumaOffset = nifFile.wgetFloat;
	  Vector4 lumaMatrix = nifFile.wgetVector4;
	}

      textures[6].read(this); // Decal
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      foreach(ref Texture t; textures)
	if(t.inUse) t.texture = lookup!(NiSourceTexture)(list);
    }
}

class NiMaterialProperty : Property
{
  Vector ambient, diffuse, specular, emissive;
  float glossiness, alpha;

  override:
  void read()
    {
      super.read();
      if(flags != 1) nifFile.warn("Unknown flags");

      ambient = nifFile.getVector;
      diffuse = nifFile.getVector;
      specular = nifFile.getVector;
      emissive = nifFile.getVector;
      glossiness = nifFile.getFloat;
      alpha = nifFile.getFloat; // Use Alpha Property when this is not 1

      debug(verbose)
	{
	  writefln("Ambient: ", ambient.toString);
	  writefln("Diffuse: ", diffuse.toString);
	  writefln("Specular: ", specular.toString);
	  writefln("Emissive: ", emissive.toString);
	  writefln("Glossiness: ", glossiness);
	  writefln("Alpha: ", alpha);
	}
    }
}
class NiVertexColorProperty : Property
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

  override:
  void read()
    {
      super.read();
      if(flags != 0) nifFile.warn("Unknown flags");
      vertmode = nifFile.getIntIs(0,1,2);
      lightmode = nifFile.getIntIs(0,1);
      debug(verbose)
	{
	  writefln("Vertex mode: ", vertmode);
	  writefln("Lighting mode: ", lightmode);
	}
    }
}

alias NiWireframeProperty NiDitherProperty;
alias NiWireframeProperty NiSpecularProperty;

class NiWireframeProperty : Property
{
  override:
  void read()
    {
      super.read();
      if(flags != 1) nifFile.warn("Unknown flags");
    }
}

class NiShadeProperty : Property
{
  override:
  void read()
    {
      super.read();
      if(flags != 0) nifFile.warn("Unknown flags");
    }
}

class NiAlphaProperty : Property
{
  /*
    In NiAlphaProperty, the flags have the following meaning.

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
  ubyte threshold;

  override:
  void read()
    {
      super.read();
      ubyte b = nifFile.getUbyte;
      debug(verbose) writefln("Unknown byte: ", b);
    }
}

class NiZBufferProperty : Property
{
 override:
  void read()
    {
      super.read();
      debug(verbose)
	{
	  if(flags&1) writefln("  0x01 ZTest");
	  if(flags&2) writefln("  0x02 ZWrite");
	}
      if(flags & 0xfc) nifFile.warn("Unknown flags");
    }
}
