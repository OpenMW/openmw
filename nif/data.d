/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (data.d) is part of the OpenMW package.

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

module nif.data;
import nif.record;

class NiSourceTexture : Named
{
  byte external;
  union
  {
    char[] filename;
    NiPixelData data;
  }

  /* Pixel layout
     0 - Palettised
     1 - High color 16
     2 - True color 32
     3 - Compressed
     4 - Bumpmap
     5 - Default */
  int pixel;

  /* Mipmap format
     0 - no
     1 - yes
     2 - default */
  int mipmap;

  /* Alpha
     0 - none
     1 - binary
     2 - smooth
     3 - default (use material alpha, or multiply material with texture if present)
  */
  int alpha;

 override:
  void sortOut(Record[] list)
    {
      super.sortOut(list);
      if(!external)
	data = lookup!(NiPixelData)(list);
    }

  void read()
  {
    super.read();

    external = nifFile.getByteIs(0,1);
    debug(verbose) writefln("Use external file: ", external);
    
    if(external)
      {
	filename = nifFile.getString;
	debug(verbose) writefln("Filename: ", filename);
      }
    else
      {
	nifFile.wgetByteIs(1);
	debug(verbose) writef("Pixel Data ");
	getIndex();
      }

    pixel = nifFile.getInt;
    mipmap = nifFile.getInt;
    alpha = nifFile.getIntIs(3);

    debug(verbose)
      {
	writefln("Pixel layout: ", pixel);
	writefln("Mipmap: ", mipmap);
	writefln("Alpha: ", alpha);
      }

    nifFile.wgetByteIs(1);
  }
}

// Common ancestor for several classes
class ShapeData : Record
{
  float[] vertices, normals, colors, uvlist;
  Vector center;
  float radius;

 override:
  void read()
    {
      super.read();

      short verts = nifFile.getShort;
      debug(verbose) writefln("Vertices: ", verts);

      if(nifFile.getInt != 0)
	{
	  debug(verbose) writefln("Reading vertices");
	  if(verts == 0) nifFile.warn("Empty vertex array");
	  vertices = nifFile.getArraySize!(float)(verts*3);
	}
      else nifFile.warn("No vertices found");

      if(nifFile.getInt != 0)
	{
	  debug(verbose) writefln("Reading normals");
	  normals = nifFile.getArraySize!(float)(verts*3);
	}

      center = nifFile.getVector();
      radius = nifFile.getFloat();
      debug(verbose)
	{
	  writefln("Center: ", center.toString);
	  writefln("Radius: ", radius);
	}

      if(nifFile.getInt != 0)
	{
	  debug(verbose) writefln("Reading vertex colors");
	  colors = nifFile.getArraySize!(float)(verts*4);
	}

      short uvs = nifFile.getShort;
      debug(verbose) writefln("Number of UV sets: ", uvs);

      if(nifFile.getInt != 0)
	{
	  if(uvs == 0) nifFile.warn("Empty UV list");
	  uvlist = nifFile.getArraySize!(float)(uvs*verts*2);
	}
      else if(uvs != 0) nifFile.warn("UV list was unexpectedly missing");
    }
}

class NiAutoNormalParticlesData : ShapeData
{
  ushort activeCount;

 override:
  void read()
    {
      super.read();

      nifFile.assertWarn(uvlist.length == 0, "UV coordinates are not expected in this record");

      debug(verbose) writef("Active vertices: ");

      // Number of active vertices (always matches count?)
      activeCount = nifFile.wgetUshortIs(cast(ushort)vertices.length/3);

      nifFile.wgetFloat(); // Active radius (?)
      nifFile.wgetShort(); // Number of valid entries in the following arrays

      //writefln("%x", nifFile.position);
      if(nifFile.wgetInt == 0) nifFile.warn("Particle size list is missing");
      else
	for(int i; i<activeCount; i++)
	  {
	    float fl = nifFile.getFloat; // Particle sizes
	    debug(veryverbose)
	      writefln("  %d: ", i, fl);
	  }
    }
}

class NiRotatingParticlesData : NiAutoNormalParticlesData
{
 override:
  void read()
    {
      super.read();

      nifFile.assertWarn(normals.length == 0, "Normals are not expected in this record");

      Vector4 v;

      if(nifFile.wgetInt == 0) nifFile.warn("Rotation data is not present");
      else
	// This is either activeCount or vertices.length/3, until I
	// find a file in which the two differs (if one exists), we
	// can't know which is correct.
	for(int i; i<activeCount; i++)
	  {
	    v = nifFile.getVector4; // Quaternions
	    debug(veryverbose)
	      writefln("  %d: ", i, v.toString);
	  }
      //writefln("%x", nifFile.position);
    }
}

class NiPosData : Record
{
 override:
  void read()
    {
      super.read();

      int count = nifFile.getInt;
      debug(verbose) writefln("Count: ", count);
      int type = nifFile.wgetIntIs(1,2);
      for(int i; i<count; i++)
	{
	  // TODO: Make a generalized struct of this? Seems to be the
	  // same as in NiKeyframeData.
	  float time = nifFile.getFloat;
	  debug(verbose) writef("Time %f: ", time);
	  Vector v;
	  if(type == 1)
	    {
	      v = nifFile.getVector;
	      debug(verbose) writef(v.toString);
	    }
	  else if(type == 2)
	    {
	      v = nifFile.getVector;
	      debug(verbose) writef(v.toString, " ");
	      v = nifFile.getVector;
	      debug(verbose) writef(v.toString, " ");
	      v = nifFile.getVector;
	      debug(verbose) writef(v.toString);
	    }
	  else nifFile.fail("Unknown type");
	  debug(verbose) writefln();
	}
    }
}

class NiUVData : Record
{
 override:
  void read()
    {
      super.read();

      debug(verbose) writef("Count: ");
      int count = nifFile.wgetInt();

      // TODO: This is claimed to be a "float animation key", which is
      // also used in FloatData and KeyframeData
      if(count != 5 && count != 3 && count != 2 && count != 0)
	nifFile.warn("Untested count");

      if(count)
	{
	  nifFile.wgetIntIs(2);
	  debug(verbose) writefln("%d entries in list 1:", count);
	  for(int i; i<count; i++)
	    {
	      float time = nifFile.getFloat;
	      Vector v = nifFile.getVector;
	      debug(verbose)
		writefln("  Time %f: ", time, v.toString);
	    }
	}

      count = nifFile.getInt;

      if(count)
	{
	  nifFile.wgetIntIs(2);
	  debug(verbose) writefln("%d entries in list 2:", count);
	  for(int i; i<count; i++)
	    {
	      float time = nifFile.getFloat;
	      Vector v = nifFile.getVector;
	      debug(verbose)
		writefln("  Time %f: ", time, v.toString);
		 
	    }
	}

      nifFile.getIntIs(0);
      nifFile.getIntIs(0);
    }
}

class NiPixelData : Record
{
  uint rmask, gmask, bmask, amask;
  int bpp;
  int mips;

 override:
  void read()
    {
      super.read();

      nifFile.wgetIntIs(0,1);
      rmask = nifFile.getUintIs(0xff);
      gmask = nifFile.getUintIs(0xff00);
      bmask = nifFile.getUintIs(0xff0000);
      amask = nifFile.getUintIs(0xff000000,0);

      bpp = nifFile.getIntIs(24,32);

      nifFile.wgetByte();

      nifFile.wgetByteIs(8);
      nifFile.wgetUbyteIs(130);

      nifFile.wgetByteIs(0,32);

      nifFile.wgetByteIs(0);
      nifFile.wgetByteIs(65);

      nifFile.wgetByteIs(0,12);

      nifFile.wgetByteIs(0);
      nifFile.wgetIntIs(-1);

      mips = nifFile.getInt;
      int bytes = nifFile.getIntIs(bpp>>3);

      debug(verbose)
	{
	  writefln("Red   mask %8xh", rmask);
	  writefln("Green mask %8xh", gmask);
	  writefln("Blue  mask %8xh", bmask);
	  writefln("Alpha mask %8xh", amask);
	  writefln("Bits per pixel: ", bpp);
	  writefln("Number of mipmaps: ", mips);
	  writefln("Bytes per pixel: ", bytes);
	}

      for(int i; i<mips; i++)
	{
	  int x = nifFile.getInt;
	  int y = nifFile.getInt;
	  uint offset = nifFile.getUint;
	  debug(verbose)
	    {
	      writefln("Mipmap %d:", i);
	      writefln("  Size: %dx%d", x, y);
	      writefln("  Offset in data field: %xh", offset);
	    }
	}

      uint dataSize = nifFile.getUint;
      nifFile.seekCur(dataSize);
    }
}

class NiColorData : Record
{
  struct ColorData
  {
    float time, R, G, B, A;
  }
  static assert(ColorData.sizeof == 20);

  ColorData colors[];

 override:
  void read()
    {
      super.read();
      int count = nifFile.getInt;
      nifFile.getIntIs(1);
      colors = nifFile.getArraySize!(ColorData)(count);
    }
}

class NiVisData : Record
{
  align(1)
  struct VisData
  {
    float time;
    byte isSet;

    static assert(VisData.sizeof == 5);
  }

  VisData vis[];

 override:
  void read()
    {
      super.read();
      int count = nifFile.getInt;
      vis = nifFile.getArraySize!(VisData)(count);
      debug(check)
	foreach(VisData v; vis)
	  if(v.isSet!=0 && v.isSet!=1) nifFile.warn("Unknown bool value");
    }
}

class NiFloatData : Record
{
  struct FloatData
  {
    float time;
    float[3] alpha;
  }
  static assert(FloatData.sizeof == 16);

  FloatData data[];

 override:
  void read()
    {
      super.read();

      int cnt = nifFile.getInt;

      nifFile.getIntIs(2);

      data = nifFile.getArraySize!(FloatData)(cnt);

      // This might be the "keyfloat" type, ie. a value and forward
      // and backward tangents. Might be used for animating in the
      // alpha space?
      debug(verbose)
	foreach(FloatData d; data)
	  writefln("Time: %f Alpha: %f, %f, %f", d.time,
		   d.alpha[0], d.alpha[1], d.alpha[2]);
    }
}

class NiSkinData : Record
{
  align(1)
  struct Weight
  {
    ushort vertex;
    float weight;

    static assert(Weight.sizeof == 6);
  }

  Weight weights[][];

 override:
  void read()
    {
      super.read();

      Matrix m;
      Vector v;
      Vector4 v4;

      nifFile.getMatrix(m);
      v = nifFile.getVector();
      nifFile.getFloatIs(1);
      debug(verbose)
	{
	  writefln("Matrix:\n", m.toString);
	  writefln("Vector: ", v.toString);
	  writefln("Float: 1 (always)");
	}

      int count = nifFile.getInt;
      debug(verbose) writefln("Bones: ", count);
      nifFile.getIntIs(-1);

      nifFile.fitArray(70,count);

      weights = nifRegion.allocateT!(Weight[])(count);

      foreach(int i, ref Weight[] wl; weights)
	{
	  nifFile.getMatrix(m); // Rotation offset of the skin from this
			  // bone in bind position.
	  v = nifFile.getVector(); // Translation -ditto-
	  nifFile.getFloatIs(1); // Scale -ditto- ?
	  debug(verbose)
	    {
	      writefln("Bone #%d:", i);
	      writefln("  Rotation:\n", m.toString);
	      writefln("  Translation: ", v.toString);
	      writefln("  Scale: 1 (always)");
	    }
	  v4 = nifFile.getVector4;

	  // Number of vertex weights
	  ushort cnt2 = nifFile.getUshort;

	  debug(verbose)
	    {
	      writefln("  4-Vector: ", v4.toString);
	      writefln("  Number of vertex weights: ", cnt2);
	    }

	  wl = nifFile.getArraySize!(Weight)(cnt2);

	  debug(veryverbose)
	    foreach(ref Weight w; wl)
	      writefln("    %d: ", w.vertex, w.weight);
	}
    }
}

class NiTriShapeData : ShapeData
{
  short[] triangles;
  short[][] matches;

 override:
  void read()
    {
      super.read();

      ushort verts = cast(ushort)(vertices.length/3);

      short tris = nifFile.getShort;
      debug(verbose) writefln("Number of faces: ", tris);
      if(tris)
	{
	  int cnt = nifFile.getIntIs(tris*3);
	  triangles = nifFile.getArraySize!(short)(cnt);
	}

      short match = nifFile.getShort;
      if(match)
	{
	  nifFile.fitArray(match,2);
	  matches = nifRegion.allocateT!(short[])(match);
	  if(match != verts) nifFile.warn("Expected verts and match to be equal");
	  foreach(ref short[] s; matches)
	    {
	      short sh = nifFile.getShort;
	      if(sh>=verts || sh < 0)
		nifFile.warn("Presumed vertex index was out of bounds");
	      s = nifFile.getArraySize!(short)(sh);
	    }
	}
    }
}

class NiMorphData : Record
{
  //float[] vertexList;

 override:
  void read()
    {
      super.read();

      int morphCount = nifFile.getInt;

      int vertCount = nifFile.getInt;

      debug(verbose)
	{
	  writefln("Number of morphs: ", morphCount);
	  writefln("Vertices: ", vertCount);
	}

      nifFile.wgetByteIs(1);

      //nifFile.getIntIs(0);
      //nifFile.getIntIs(1);
      //vertexList = nifFile.getArraySize!(float)(3*vertCount);

      for(int i=0; i<morphCount; i++)
	{
	  int magic = nifFile.getInt;

	  debug(veryverbose)
	    {
	      writefln("Getting morph batch %d", i);
	      writefln("  Number of 4-vectors ", magic);
	    }

	  nifFile.wgetIntIs(0,1,2); // Is always 1 for i=0, 0 or 2 otherwise

	  float[] l;

	  // Time, data, forward, backward tangents
	  if(magic)
	    l = nifFile.getArraySize!(float)(magic*4);

	  debug(veryverbose)
	    for(int j; j<magic; j++)
	      writefln("  Time %f :  [", l[4*j], l[4*j+1],",", l[4*j+2],",", l[4*j+3],"]");

	  l = nifFile.getArraySize!(float)(vertCount*3);

	  debug(veryverbose)
	    for(int j; j<vertCount; j++)
	      writefln("  Vertex morph %d: ", j, " [%f, %f, %f]",
		       l[j*3], l[j*3+1], l[j*3+2]);
	}
    }
}

class NiKeyframeData : Record
{
  // These should be moved out of this class, given their own reader
  // functions, and used elsewhere. But it currently works, and I
  // haven't really confirmed that these ARE used elsewhere, yet.

  struct Rotation1
  {
    float time;
    float[4] quaternion;

    static assert(Rotation1.sizeof == 5*4);
  }

  struct Rotation3
  {
    float time;
    float[4] quaternion;
    float tension;
    float bias;
    float continuity;

    static assert(Rotation3.sizeof == 8*4);
  }

  struct Rotation4
  {
    float time;

    struct R
    {
      struct R1
      {
	float time;
	float unknown;

	static assert(R1.sizeof == 8);
      }

      struct R2
      {
	float time;
	float unknown[3];

	static assert(R2.sizeof == 16);
      }

      int type;
      union
      {
	R1[] r1;
	R2[] r2;
      }
    }

    R r3[3];
  }

  struct Translation1
  {
    float time;
    float[3] translation;
    static assert(Translation1.sizeof==4*4);
  }
  struct Translation2
  {
    float time;
    float[3] translation;
    float[3] forward;
    float[3] backward;
    static assert(Translation2.sizeof==4*10);
  }
  struct Translation3
  {
    float time;
    float[3] translation;
    float tension;
    float bias;
    float continuity;
    static assert(Translation3.sizeof==4*7);
  }

  struct Scale1
  {
    float time;
    float scale;
    static assert(Scale1.sizeof == 4*2);
  }
  struct Scale2
  {
    float time;
    float scale;
    float forward;
    float backward;
    static assert(Scale2.sizeof == 4*4);
  }
  struct Scale3
  {
    float time;
    float scale;
    float tension;
    float bias;
    float continuity;
    static assert(Scale3.sizeof == 4*5);
  }

  // Data arrays. Only one of each type will be used. I used to have
  // them in unions, but then I realized that was just asking for trouble.

  Rotation1 rot1[];
  Rotation3 rot3[];
  Rotation4 rot4[];

  Translation1 tra1[];
  Translation2 tra2[];
  Translation3 tra3[];

  Scale1 sca1[];
  Scale2 sca2[];
  Scale3 sca3[];

  int rotType, traType, scaleType;

 override:
  void read()
    {
      super.read();

      int count;

      count = nifFile.getInt;
      debug(verbose) writefln("Number of rotations: ", count);

      if(count)
	{
	  rotType = nifFile.getInt;

	  if(rotType == 1)
	    rot1 = nifFile.getArraySize!(Rotation1)(count);
	  else if(rotType == 3)
	    rot3 = nifFile.getArraySize!(Rotation3)(count);
	  else if(rotType == 4)
	    {
	      if(count!=1) nifFile.warn("Rotation type 4, but count was not 1, it was "
				  ~ .toString(count));
	      nifFile.fitArray(count,6*4); // Minimum size
	      rot4 = nifRegion.allocateT!(Rotation4)(count);

	      foreach(ref Rotation4 rr; rot4)
		{
		  rr.time = nifFile.getFloat;
		  foreach(ref rr.R r; rr.r3)
		    {
		      int cnt = nifFile.getInt;
		      r.type = nifFile.getInt;
		      if(r.type == 1)
			nifFile.getArraySize!(r.R1)(cnt);
		      else if(r.type == 2)
			nifFile.getArraySize!(r.R2)(cnt);
		      else nifFile.fail("Unknown rotation subtype " ~ .toString(r.type));
		    }
		}
	    }
	  else nifFile.fail("Don't know how to handle rotation type " ~ .toString(rotType));
	} // End of rotations

      count = nifFile.getInt;
      debug(verbose) writefln("Number of translations: ", count);

      if(count)
	{
	  traType = nifFile.getInt;

	  if(traType == 1)
	    tra1 = nifFile.getArraySize!(Translation1)(count);
	  else if(traType == 2)
	    tra2 = nifFile.getArraySize!(Translation2)(count);
	  else if(traType == 3)
	    tra3 = nifFile.getArraySize!(Translation3)(count);
	  else nifFile.fail("Don't know how to handle translation type "
		      ~ .toString(traType));
	} // End of translations

      count = nifFile.getInt;
      debug(verbose) writefln("Number of scalings: ", count);

      if(count)
	{
	  int scaleType = nifFile.getInt;
	  if(scaleType == 1) sca1 = nifFile.getArraySize!(Scale1)(count);
	  else if(scaleType == 2) sca2 = nifFile.getArraySize!(Scale2)(count);
	  else if(scaleType == 3) sca3 = nifFile.getArraySize!(Scale3)(count);
	  else nifFile.fail("Don't know how to handle scaling type "
		      ~ .toString(scaleType));
	} // End of scalings
      
    }
}
