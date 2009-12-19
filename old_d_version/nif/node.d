/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (node.d) is part of the OpenMW package.

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

module nif.node;

import nif.record;

// Tree node
abstract class Node : Named
{
  ushort flags;
  Transformation trafo;
  //Extra properties[];
  Property properties[];

  Node parent;

  // Bounding box info
  bool hasBounding;
  Vector boundPos;
  Matrix boundRot;
  Vector boundXYZ;

  void setParent(Node p)
    {
      debug(veryverbose)
	writefln("Setting parent of ", toString, " to ", p);

      if(parent !is null)
	{
	  char str[] = toString() ~ " already has parent " ~ parent.toString()
	    ~ ", but setting ";
	  if(p !is null) str ~= p.toString;
	  else str ~= "to null";

	  nifFile.warn(str);
	}

      parent = p;
    }

 override:
  void read()
    {
      super.read();

      flags = nifFile.getUshort();
      nifFile.getTransformation(trafo);
  
      debug(verbose)
	{
	  writefln("Flags: %x", flags);
	  writefln("Transformation:\n", trafo.toString);
	  writef("Properties: ");
	}

      properties = nifRegion.allocateT!(Property)(getIndexList());

      if(nifFile.getInt != 0)
	{
	  hasBounding = true;
	  nifFile.getIntIs(1);
	  boundPos = nifFile.getVector;
	  nifFile.getMatrix(boundRot);
	  boundXYZ = nifFile.getVector;
	  //if(name != "Bounding Box") nifFile.warn("Node name is not 'Bounding Box'");
	  debug(verbose)
	    {
	      writefln("Bounding box: ", boundPos.toString);
	      writefln(boundRot.toString);
	      writefln("XYZ: ", boundXYZ.toString);
	    }
	}
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      lookupList!(Property)(list,properties);
    }

}

class NiCamera : Node
{
  float left, right, top, bottom, near, far,
    vleft, vright, vtop, vbottom, LOD;

 override:
  void read()
    {
      super.read();

      left = nifFile.getFloat;
      right = nifFile.getFloat;
      top = nifFile.getFloat;
      bottom = nifFile.getFloat;
      near = nifFile.getFloat;
      far = nifFile.getFloat;
      
      vleft = nifFile.getFloat;
      vright = nifFile.getFloat;
      vtop = nifFile.getFloat;
      vbottom = nifFile.getFloat;

      LOD = nifFile.getFloat;

      nifFile.getIntIs(-1);
      nifFile.getIntIs(0);

      debug(verbose)
	{
	  writefln("Camera frustrum:");
	  writefln("  Left: ", left);
	  writefln("  Right: ", right);
	  writefln("  Top: ", top);
	  writefln("  Bottom: ", bottom);
	  writefln("  Near: ", near);
	  writefln("  Far: ", far);

	  writefln("View port:");
	  writefln("  Left: ", vleft);
	  writefln("  Right: ", vright);
	  writefln("  Top: ", vtop);
	  writefln("  Bottom: ", vbottom);

	  writefln("LOD adjust: ", LOD);
	}
    }
}

class NiAutoNormalParticles : Node
{
  NiAutoNormalParticlesData data;

 override:
  void read()
    {
      super.read();

      debug(check)
	{
	  if(flags & 0xffff-6)
	    nifFile.warn("Unknown flags");
	}

      debug(verbose) writef("Particle Data ");
      getIndex();

      nifFile.getIntIs(-1);
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiAutoNormalParticlesData)(list);

      debug(check)
	{
	  if(castCheck!(NiParticleSystemController)(controller)
	     && castCheck!(NiBSPArrayController)(controller))
	    nifFile.warn("In " ~ toString ~ ": did not expect controller "
		   ~ controller.toString);
	}
    }
}

class NiRotatingParticles : Node
{
  NiRotatingParticlesData data;

 override:
  void read()
    {
      super.read();

      debug(check)
	{
	  if(flags & 0xffff-6)
	    nifFile.warn("Unknown flags");
	}

      //nifFile.getIntIs(0);

      debug(verbose) writef("Particle Data ");
      getIndex();

      nifFile.getIntIs(-1);
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiRotatingParticlesData)(list);

      debug(check)
	{
	  if(castCheck!(NiParticleSystemController)(controller)
	     && castCheck!(NiBSPArrayController)(controller) )
	    nifFile.warn("In " ~ toString ~ ": did not expect controller "
		   ~ controller.toString);
	}
      //castWarn!(NiParticleSystemController)(controller);
    }
}

class NiTriShape : Node
{
  NiTriShapeData data;
  NiSkinInstance skin;

 override:
  void read()
    {
      super.read();

      debug(verbose)
	{
	  // If this is correct, it suggests how one might decode
	  // other flags. Check if this one is correct in sortOut().
	  if(flags&0x40) writefln("Mesh has no normals?");
	}
      debug(check)
	{
	  if(flags & (0xffff-0x47))
	    nifFile.warn("Unknown flags were set");
	}

      //nifFile.getIntIs(0);
      debug(verbose) writef("Mesh index: ");
      getIndex();
      debug(verbose) writef("Skin index: ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiTriShapeData)(list);
      skin = lookup!(NiSkinInstance)(list);

      debug(check)
	{
	  if(data is null) nifFile.warn("Data missing from NiTriShape");

	  if(castCheck!(NiGeomMorpherController)(controller)
	     && castCheck!(NiUVController)(controller)
	     && castCheck!(NiVisController)(controller) )
	    nifFile.warn("In " ~ toString ~ ": did not expect controller "
		   ~ controller.toString);

	  if((data.normals.length == 0) && (flags & 0x40 == 0) ||
	     (data.normals.length != 0) && (flags & 0x40))
	    nifFile.warn("0x40 present but normals missing, or vice versa");
	}
    }
}

class NiNode : Node
{
  Node children[];
  Effect effects[];

 override:
  void read()
    {
      super.read();

      debug(verbose)
	{
	  if(flags & 1) writefln("  0x01 Hidden");
	  if(flags & 2) writefln("  0x02 Collision detection?");
	  if(flags & 4) writefln("  0x04 Collision detection2 ?");
	  if(flags & 8) writefln("  0x08 Unknown but common");
	  if(flags & 0x20) writefln("  0x20 Unknown");
	  if(flags & 0x40) writefln("  0x40 Unknown");
	  if(flags & 0x80) writefln("  0x80 Unknown");
	}
      debug(check)
	if(flags & 0x10) nifFile.warn("Unknown flags were set");

      debug(verbose) writef("Child nodes: ");
      children = nifRegion.allocateT!(Node)(getIndexList());
      debug(verbose) writef("Effects: ");
      effects = nifRegion.allocateT!(Effect)(getIndexList());
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);

      lookupList!(Node)(list,children);
      lookupList!(Effect)(list,effects);

      if(castCheck!(NiKeyframeController)(controller)
	 && castCheck!(NiVisController)(controller)
	 && castCheck!(NiPathController)(controller))
	nifFile.warn("In " ~ toString ~ ": did not expect controller "
	       ~ controller.toString);

      foreach(Node n; children)	
	n.setParent(this);
    }
}
