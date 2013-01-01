/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (bsa_file.cpp) is part of the OpenMW package.

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

#include "constrainedfiledatastream.hpp"

#include <stdexcept>

class ConstrainedDataStream : public Ogre::DataStream {
    std::ifstream mStream;
    const size_t mStart;
    size_t mPos;
    bool mIsEOF;

public:
    ConstrainedDataStream(const Ogre::String &fname, size_t start, size_t length)
      : mStream(fname.c_str(), std::ios_base::binary), mStart(start), mPos(0), mIsEOF(false)
    {
        mSize = length;
        if(!mStream.seekg(mStart, std::ios_base::beg))
            throw std::runtime_error("Error seeking to start of BSA entry");
    }

    ConstrainedDataStream(const Ogre::String &name, const Ogre::String &fname,
                          size_t start, size_t length)
      : Ogre::DataStream(name), mStream(fname.c_str(), std::ios_base::binary),
        mStart(start), mPos(0), mIsEOF(false)
    {
        mSize = length;
        if(!mStream.seekg(mStart, std::ios_base::beg))
            throw std::runtime_error("Error seeking to start of BSA entry");
    }


    virtual size_t read(void *buf, size_t count)
    {
        mStream.clear();

        if(count > mSize-mPos)
        {
            count = mSize-mPos;
            mIsEOF = true;
        }
        mStream.read(reinterpret_cast<char*>(buf), count);

        count = mStream.gcount();
        mPos += count;
        return count;
    }

    virtual void skip(long count)
    {
        if((count >= 0 && (size_t)count <= mSize-mPos) ||
           (count < 0 && (size_t)-count <= mPos))
        {
            mStream.clear();
            if(mStream.seekg(count, std::ios_base::cur))
            {
                mPos += count;
                mIsEOF = false;
            }
        }
    }

    virtual void seek(size_t pos)
    {
        if(pos < mSize)
        {
            mStream.clear();
            if(mStream.seekg(pos+mStart, std::ios_base::beg))
            {
                mPos = pos;
                mIsEOF = false;
            }
        }
    }

    virtual size_t tell() const
    { return mPos; }

    virtual bool eof() const
    { return mIsEOF; }

    virtual void close()
    { mStream.close(); }
};

Ogre::DataStreamPtr openConstrainedFileDataStream (char const * filename, size_t offset, size_t length)
{
	assert (length != 0xFFFFFFFF); // reserved for future use...

	return Ogre::DataStreamPtr(new ConstrainedDataStream(filename, offset, length));
}
