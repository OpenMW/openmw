/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (memorystream.hpp) is part of the OpenMW package.

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

  Compressed BSA upgrade added by Azdul 2019

 */

#ifndef OPENMW_COMPONENTS_BSA_MEMORYSTREAM_HPP
#define OPENMW_COMPONENTS_BSA_MEMORYSTREAM_HPP

#include <istream>
#include <vector>

#include <components/files/memorystream.hpp>

namespace Bsa
{
    /**
        Class replaces Ogre memory streams without introducing any new external dependencies
        beyond standard library.

        Allows to pass memory buffer as Files::IStreamPtr.

        Memory buffer is freed once the class instance is destroyed.
     */
    class MemoryInputStream : private std::vector<char>, public Files::MemBuf, public std::istream
    {
    public:
        explicit MemoryInputStream(size_t bufferSize)
            : std::vector<char>(bufferSize)
            , Files::MemBuf(this->data(), this->size())
            , std::istream(static_cast<std::streambuf*>(this))
        {
        }

        char* getRawData() { return this->data(); }
    };

}
#endif
