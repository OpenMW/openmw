/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (controlled.h) is part of the OpenMW package.

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

#ifndef _NIF_CONTROLLED_H_
#define _NIF_CONTROLLED_H_

#include "extra.h"

namespace Nif
{

/// Anything that has a controller
struct Controlled : Extra
{
  ControllerPtr controller;

  void read(NIFFile *nif)
  {
    Extra::read(nif);
    controller.read(nif);
  }
};

/// Has name, extra-data and controller
struct Named : Controlled
{
  SString name;

  void read(NIFFile *nif)
  {
    name = nif->getString();
    Controlled::read(nif);
  }
};

} // Namespace
#endif
