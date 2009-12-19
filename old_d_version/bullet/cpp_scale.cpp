/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_scale.cpp) is part of the OpenMW package.

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

// WARNING: This file does NOT work, and it is not used yet.

class ScaleCallback : public btTriangleCallback
{
  btTriangleCallback *call;
  float factor;

public:
  ScaleCallback(btTriangleCallback *c, float f)
  { call = c; factor = f; }

  void processTriangle(btVector3 *tri, int partid, int triindex)
  {
    btVector3 vecs[3];
    vecs[0] = tri[0]*factor;
    vecs[1] = tri[1]*factor;
    vecs[2] = tri[2]*factor;

    call->processTriangle(vecs, partid, triindex);
  }
};

// This class is used to uniformly scale a triangle mesh by a
// factor. It wraps around an existing shape and does not copy the
// data.
class ScaleShape : public btConcaveShape
{
  btConcaveShape* child;
  float factor, fact3, facthalf;
	
public:

  ScaleShape(btConcaveShape* ch, float ft)
  {
    child = ch;
    factor = ft;
    fact3 = factor*factor*factor;
    facthalf = factor*0.5;
  }

  void calculateLocalInertia(btScalar mass,btVector3& inertia) const
  {
    btVector3 tmpInertia;
    child->calculateLocalInertia(mass,tmpInertia);
    inertia = tmpInertia * fact3;
  }

  const char* getName()const { return "ScaleShape"; }

  void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
  {
    child->getAabb(t,aabbMin,aabbMax);
    btVector3 aabbCenter = (aabbMax+aabbMin)*0.5;
    btVector3 scaledAabbHalfExtends = (aabbMax-aabbMin)*facthalf;

    aabbMin = aabbCenter - scaledAabbHalfExtends;
    aabbMax = aabbCenter + scaledAabbHalfExtends;
  }

  void processAllTriangles(btTriangleCallback *callback,const btVector3& aabbMin,const btVector3& aabbMax) const
  {
    ScaleCallback scb(callback, factor);

    child->processAllTriangles(&scb, aabbMin, aabbMax);
  }

  void setLocalScaling(const btVector3& scaling)
  { child->setLocalScaling(scaling); }

  const btVector3& getLocalScaling() const
  { return child->getLocalScaling(); }

  int getShapeType() const
  { return TRIANGLE_MESH_SHAPE_PROXYTYPE; }
};
