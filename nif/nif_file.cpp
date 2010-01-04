/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (nif_file.cpp) is part of the OpenMW package.

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

#include "nif_file.h"

#include "../tools/stringops.h"

#include <iostream>
using namespace std;

void NIFFile::parse()
{
  // Check the header string
  const char* head = getString(40);
  if(!begins(head, "NetImmerse File Format"))
    fail("Invalid NIF header");

  // Get BCD version
  ver = getInt();
  if(ver != VER_MW)
    fail("Unsupported NIF version");

  // Number of records
  int recNum = getInt();
  records.resize(recNum);

  for(int i=0;i<recNum;i++)
    {
      SString rec = getString();
      
      cout << i << ": " << rec.toString() << endl;

      if(rec == "NiNode") cout << " got a node!\n";

      break;
    }
}
