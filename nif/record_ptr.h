/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (record_ptr.h) is part of the OpenMW package.

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

#ifndef _NIF_RECORD_PTR_H_
#define _NIF_RECORD_PTR_H_

#include "nif_file.h"

namespace Nif
{
template <class X>
class RecordPtrT
{
  int index;
  X* ptr;
  NIFFile *nif;

 public:

  RecordPtrT() : index(-2), ptr(NULL) {}

  /// Read the index from the nif
  void read(NIFFile *_nif)
  {
    // Can only read the index once
    assert(index == -2);

    // Store the NIFFile pointer for later
    nif = _nif;

    // And the index, of course
    index = nif->getInt();
  }

  /// Look up the actual object from the index
  X* operator->()
  {
    // Have we found the pointer already?
    if(ptr == NULL)
      {
        // Get the record
        assert(index >= 0);
        Record *r = nif->getRecord(index);

        // And cast it
        ptr = dynamic_cast<X*>(r);
        assert(ptr != NULL);
      }
    return ptr;
  }

  /// Pointers are allowed to be empty
  bool empty() { return index == -1; }

  int getIndex() { return index; }
};


class Extra;
class Controller;
class Node;

typedef RecordPtrT<Extra> ExtraPtr;
typedef RecordPtrT<Controller> ControllerPtr;
typedef RecordPtrT<Node> NodePtr;

} // Namespace
#endif
