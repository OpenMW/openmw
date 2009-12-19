/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (base.d) is part of the OpenMW package.

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

// Basic file structure
module nif.base;

import nif.nif;
import nif.niffile;
import nif.misc;
import monster.util.string;

//import nif.controller;
//import nif.node;
import nif.record;

debug(verbose) public import std.stdio;

void parseFile()
{
  with(nifFile)
  { // Read header
    int ver;		// Hex-encoded version
    char[40] head;	// Header string

    // Read and check header string
    getString(head);

    if(!begins(head,"NetImmerse File Format"))
      fail("Not a NetImmerse file");

    debug(verbose) writefln("Header: \"%s\"", head[0..39]);
    debug(strict)
      if(head[0..39] != "NetImmerse File Format, Version 4.0.0.2" || head[39] != 0x0a)
	fail("Unrecognized header string, most likely an unsupported NIF version");

    // Get binary coded version
    ver = getInt;

    debug(verbose)
      {
	writef("BCD version tag: ");
	for(ubyte *b = (cast(ubyte*)&ver)+3; b > (cast(ubyte*)&ver); b--)
	  writef(*b, ".");
	writefln(cast(ubyte)ver);
      }

    if(ver != 0x04_00_00_02)
      fail("Don't know how to process this version");

    // The number of records in the file
    nifMesh.records = nifRegion.allocateT!(Record)(getInt);
    debug(verbose) writefln("Number of records: ", nifMesh.records.length);

    // The format for 10.0.1.0 seems to be a bit different. After the
    // header, it contains the number of records, r (int), just like
    // 4.0.0.2, but following that it contains a short x, followed by
    // x strings. Then again by r shorts, one for each record, giving
    // which of the above strings to use to identify the record. After
    // this follows two ints (zero?) and then the record data.

    // Besides giving me more work to do, this structure suggests some
    // obvious code optimizations we can now apply to the reading
    // structure. Since we now know in advance how many records there
    // are of each type, we could allocate the structures in bulk. D
    // doesn't allow this for classes directly, but we can make a
    // custom allocator and whatnot. I doubt we save a lot of time
    // with it, though. I'll save this for later.

    // As for the record data itself, I do not know what has
    // changed. There are some new records. I might as well have
    // separate reading functions for the entire main structure.

    // EDIT 14. feb 06: Since we are concentrating on Morrowind, let
    // us drop other versions for now. It would be nice to support
    // other versions (or even other mesh formats), but the priority
    // of this is very low at the moment.
  }

 endFor:
  foreach(int i, ref Record o; nifMesh.records)
    {
      // Get type string, and update the NIFFiles error message to
      // match it.
      char[] recName = nifFile.getString;
      nifFile.setRec(i,recName);

      debug(verbose)
	{
	  writefln("\n%d: %s (%x)", i, recName, nifFile.position);
	}

      switch(recName)
	{
	  // These are the record types we know how to read

	  // Time controllers
	case "NiVisController": o = new NiVisController; break;
	case "NiGeomMorpherController": o = new NiGeomMorpherController; break;
	case "NiKeyframeController": o = new NiKeyframeController; break;
	case "NiAlphaController": o = new NiAlphaController; break;
	case "NiUVController": o = new NiUVController; break;
	case "NiPathController": o = new NiPathController; break;
	case "NiMaterialColorController": o = new NiMaterialColorController; break;
	case "NiBSPArrayController": o = new NiBSPArrayController; break;
	case "NiParticleSystemController": o = new NiParticleSystemController; break;

	  // NiNodes
	case "RootCollisionNode":
	case "NiBSParticleNode":
	case "NiBSAnimationNode":
	case "NiBillboardNode":
	case "AvoidNode":
	case "NiNode": o = new NiNode; break;

	  // Other nodes
	case "NiTriShape": o = new NiTriShape; break;
	case "NiRotatingParticles": o = new NiRotatingParticles; break;
	case "NiAutoNormalParticles": o = new NiAutoNormalParticles; break;
	case "NiCamera": o = new NiCamera; break;

	  // Effects
	case "NiAmbientLight":
	case "NiDirectionalLight": o = new Light; break;
	case "NiTextureEffect": o = new NiTextureEffect; break;

	  // Properties
	case "NiTexturingProperty": o = new NiTexturingProperty; break;
	case "NiMaterialProperty": o = new NiMaterialProperty; break;
	case "NiZBufferProperty": o = new NiZBufferProperty; break;
	case "NiAlphaProperty": o = new NiAlphaProperty; break;
	case "NiVertexColorProperty": o = new NiVertexColorProperty; break;
	case "NiShadeProperty": o = new NiShadeProperty; break;
	case "NiDitherProperty": o = new NiDitherProperty; break;
	case "NiWireframeProperty": o = new NiWireframeProperty; break;
	case "NiSpecularProperty": o = new NiSpecularProperty; break;

	  // Extra Data
	case "NiVertWeightsExtraData": o = new NiVertWeightsExtraData; break;
	case "NiTextKeyExtraData": o = new NiTextKeyExtraData; break;
	case "NiStringExtraData": o = new NiStringExtraData; break;

	case "NiGravity": o = new NiGravity; break;
	case "NiPlanarCollider": o = new NiPlanarCollider; break;
	case "NiParticleGrowFade": o = new NiParticleGrowFade; break;
	case "NiParticleColorModifier": o = new NiParticleColorModifier; break;
	case "NiParticleRotation": o = new NiParticleRotation; break;

	  // Data
	case "NiFloatData": o = new NiFloatData; break;
	case "NiTriShapeData": o = new NiTriShapeData; break;
	case "NiVisData": o = new NiVisData; break;
	case "NiColorData": o = new NiColorData; break;
	case "NiPixelData": o = new NiPixelData; break;
	case "NiMorphData": o = new NiMorphData; break;
	case "NiKeyframeData": o = new NiKeyframeData; break;
	case "NiSkinData": o = new NiSkinData; break;
	case "NiUVData": o = new NiUVData; break;
	case "NiPosData": o = new NiPosData; break;
	case "NiRotatingParticlesData": o = new NiRotatingParticlesData; break;
	case "NiAutoNormalParticlesData": o = new NiAutoNormalParticlesData; break;

	  // Other
	case "NiSequenceStreamHelper": o = new NiSequenceStreamHelper; break;
	case "NiSourceTexture": o = new NiSourceTexture; break;
	case "NiSkinInstance": o = new NiSkinInstance; break;

	default:
	  nifFile.fail("Unknown record type " ~ recName);
 	}

      if(o !is null)
	{
	  //nifFile.setRec(i,recName ~ " (" ~ o.toString ~ ")");
	  o.read();
	}
    }
  // Clear error message.
  nifFile.setRec(-1,"");

  // The 'footer', which it seems isn't always constant after all. I
  // have no clue what it is for though.
  int roots = nifFile.getInt;
  for(int i; i<roots;i++)
    nifFile.getInt();

  debug(verbose) if(nifFile.eof()) writefln("End of file");

  // A lot of files don't end where they should, so we just have to
  // accept it.
  //debug(check) if(!nifFile.eof()) nifFile.warn("End of file not reached");
}
