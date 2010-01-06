/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (extra.h) is part of the OpenMW package.

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

#ifndef _NIF_EXTRA_H_
#define _NIF_EXTRA_H_

#include "record.h"
#include "nif_file.h"
#include "record_ptr.h"

namespace Nif
{

/** A record that can have extra data. The extra data objects
    themselves decend from the Extra class, and all the extra data
    connected to an object form a linked list
*/
struct Extra : Record
{
  ExtraPtr extra;

  void read(NIFFile *nif) { extra.read(nif); }
};

} // Namespace
#endif
