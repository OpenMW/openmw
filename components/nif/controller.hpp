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

#ifndef OPENMW_COMPONENTS_NIF_CONTROLLER_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLER_HPP

#include "record.hpp"
#include "niffile.hpp"
#include "recordptr.hpp"

namespace Nif
{

class Controller : public Record
{
public:
    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    ControlledPtr target;

    void read(NIFStream *nif)
    {
        next.read(nif);

        flags = nif->getUShort();

        frequency = nif->getFloat();
        phase = nif->getFloat();
        timeStart = nif->getFloat();
        timeStop = nif->getFloat();

        target.read(nif);
    }

    void post(NIFFile *nif)
    {
        Record::post(nif);
        next.post(nif);
        target.post(nif);
    }
};

class NiBSPArrayController : public Controller
{
public:
    void read(NIFStream *nif)
    {
        Controller::read(nif);

        // At the moment, just skip it all
        nif->skip(111);
        int s = nif->getUShort();
        nif->skip(15 + s*40);
    }
};
typedef NiBSPArrayController NiParticleSystemController;

class NiMaterialColorController : public Controller
{
public:
    NiPosDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

class NiPathController : public Controller
{
public:
    NiPosDataPtr posData;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif)
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

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        posData.post(nif);
        floatData.post(nif);
    }
};

class NiUVController : public Controller
{
public:
    NiUVDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        nif->getUShort(); // always 0
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

class NiKeyframeController : public Controller
{
public:
    NiKeyframeDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

class NiAlphaController : public Controller
{
public:
    NiFloatDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

class NiGeomMorpherController : public Controller
{
public:
    NiMorphDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
        nif->getChar(); // always 0
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

class NiVisController : public Controller
{
public:
    NiVisDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

} // Namespace
#endif
