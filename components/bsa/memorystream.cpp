/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (memorystream.cpp) is part of the OpenMW package.

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
#include "memorystream.hpp"


namespace Bsa
{
MemoryInputStreamBuf::MemoryInputStreamBuf(size_t bufferSize) : mBufferPtr(bufferSize)
{
    this->setg(mBufferPtr.data(), mBufferPtr.data(), mBufferPtr.data() + bufferSize);
}

char* MemoryInputStreamBuf::getRawData() {
    return mBufferPtr.data();
}

MemoryInputStream::MemoryInputStream(size_t bufferSize) : 
                   MemoryInputStreamBuf(bufferSize),
    std::istream(static_cast<std::streambuf*>(this)) {

}

char* MemoryInputStream::getRawData() {
    return MemoryInputStreamBuf::getRawData();
}
}
