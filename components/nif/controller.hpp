/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (controller.h) is part of the OpenMW package.

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

#ifndef _NIF_CONTROLLER_H_
#define _NIF_CONTROLLER_H_

#include "record.hpp"
#include "nif_file.hpp"
#include "record_ptr.hpp"

namespace Nif
{

struct Controller : Record
{
  ControllerPtr next;
  int flags;
  float frequency, phase;
  float timeStart, timeStop;
  ControlledPtr target;

  void read(NIFFile *nif)
  {
    next.read(nif);

    flags = nif->getShort();

    frequency = nif->getFloat();
    phase = nif->getFloat();
    timeStart = nif->getFloat();
    timeStop = nif->getFloat();

    target.read(nif);
  }
};

struct NiBSPArrayController : Controller
{
  void read(NIFFile *nif)
  {
    Controller::read(nif);

    // At the moment, just skip it all
    nif->skip(111);
    int s = nif->getShort();
    nif->skip(15 + s*40);
  }
};
typedef NiBSPArrayController NiParticleSystemController;

struct NiMaterialColorController : Controller
{
  NiPosDataPtr data;

  void read(NIFFile *nif)
  {
    Controller::read(nif);
    data.read(nif);
  }
};

struct NiPathController : Controller
{
  NiPosDataPtr posData;
  NiFloatDataPtr floatData;

  void read(NIFFile *nif)
  {
    Controller::read(nif);

    /*
      int = 1
      2xfloat
      short = 0 or 1
     */
    nif->skip(14);
    posData.read(nif);
    floatData.read(nif);
  }
};

struct NiUVController : Controller
{
  NiUVDataPtr data;

  void read(NIFFile *nif)
  {
    Controller::read(nif);

    nif->getShort(); // always 0
    data.read(nif);
  }
};

struct NiKeyframeController : Controller
{
  NiKeyframeDataPtr data;

  void read(NIFFile *nif)
  {
    Controller::read(nif);
    data.read(nif);
  }
};

struct NiAlphaController : Controller
{
  NiFloatDataPtr data;

  void read(NIFFile *nif)
  {
    Controller::read(nif);
    data.read(nif);
  }
};

struct NiGeomMorpherController : Controller
{
  NiMorphDataPtr data;

  void read(NIFFile *nif)
  {
    Controller::read(nif);
    data.read(nif);
    nif->getByte(); // always 0
  }
};

struct NiVisController : Controller
{
  NiVisDataPtr data;

  void read(NIFFile *nif)
  {
    Controller::read(nif);
    data.read(nif);
  }
};

} // Namespace
#endif
