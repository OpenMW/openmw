/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (cpp_bsaarchive.h) is part of the OpenMW package.

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

#include <string>
#include <algorithm>

#ifndef BSA_BSA_ARCHIVE_H
#define BSA_BSA_ARCHIVE_H

namespace Bsa
{

/// Add the given BSA file as an input archive in the Ogre resource
/// system.
void addBSA(const std::string& file, const std::string& group="General");
void addDir(const std::string& file, const bool& fs, const std::string& group="General");

}

#endif
