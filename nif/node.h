/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (node.h) is part of the OpenMW package.

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

#ifndef _NIF_NODE_H_
#define _NIF_NODE_H_

#include "controlled.h"

namespace Nif
{

/** A Node is an object that's part of the main NIF tree. It has
    parent node (unless it's the root), and transformation (location
    and rotation) relative to it's parent.
 */
struct Node : Named
{
  // Node flags. Interpretation depends somewhat on the type of node.
  int flags;
  const Transformation *trafo;
  PropertyList props;

  // Bounding box info
  bool hasBounds;
  const Vector *boundPos;
  const Matrix *boundRot;
  const Vector *boundXYZ;

  void read(NIFFile *nif)
  {
    Named::read(nif);

    flags = nif->getUshort();
    trafo = nif->getTrafo();
    props.read(nif);

    hasBounds = nif->getInt();
    if(hasBounds)
      {
        nif->getInt();
        boundPos = nif->getVector();
        boundRot = nif->getMatrix();
        boundXYZ = nif->getVector();
      }
  }
};

struct NiNode : Node
{
  NodeList children;
  NodeList effects;

  void read(NIFFile *nif)
  {
    Node::read(nif);
    children.read(nif);
    effects.read(nif);
  }
};

struct NiTriShape : Node
{
  NiTriShapeDataPtr data;
  NiSkinInstancePtr skin;

  void read(NIFFile *nif)
  {
    Node::read(nif);
    data.read(nif);
    skin.read(nif);
  }
};

} // Namespace
#endif
