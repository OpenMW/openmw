/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (nif_file.h) is part of the OpenMW package.

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

#ifndef _NIF_FILE_H_
#define _NIF_FILE_H_

#include <OgreResourceGroupManager.h>
#include <OgreDataStream.h>

#include <stdexcept>
#include <vector>
#include <string>
#include <assert.h>

#include "record.hpp"
#include "nif_types.hpp"

namespace Nif
{

class NIFFile
{
    enum NIFVersion {
        VER_MW    = 0x04000002    // Morrowind NIFs
    };

    /// Nif file version
    int ver;

    /// Input stream
    Ogre::DataStreamPtr inp;

    /// File name, used for error messages
    std::string filename;

    /// Record list
    std::vector<Record*> records;

    /// Parse the file
    void parse();

public:
    /// Used for error handling
    void fail(const std::string &msg)
    {
        std::string err = "NIFFile Error: " + msg;
        err += "\nFile: " + filename;
        throw std::runtime_error(err);
    }

    /// Open a NIF stream. The name is used for error messages.
    NIFFile(const std::string &name)
      : filename(name)
    {
        inp = Ogre::ResourceGroupManager::getSingleton().openResource(name);
        parse();
    }

    ~NIFFile()
    {
        for(std::size_t i=0; i<records.size(); i++)
            delete records[i];
    }

    /// Get a given record
    Record *getRecord(size_t index)
    {
        Record *res = records.at(index);
        assert(res != NULL);
        return res;
    }

    /// Number of records
    int numRecords() { return records.size(); }

    /*************************************************
               Parser functions
    ****************************************************/

    void skip(size_t size) { inp->skip(size); }

    template<class X> X getType()
    {
        X obj;
        if(inp->read(&obj, sizeof(X)) != sizeof(X))
            fail("Failed to read from NIF");
        return obj;
    }
    unsigned short getShort() { return getType<unsigned short>(); }
    int getInt() { return getType<int>(); }
    float getFloat() { return getType<float>(); }
    char getByte() { return getType<char>(); }

    template<class X>
    std::vector<X> getArrayLen(int num)
    {
        std::vector<X> v(num);
        if(inp->read(&v[0], num*sizeof(X)) != num*sizeof(X))
            fail("Failed to read from NIF");
        return v;
    }

    template<class X>
    std::vector<X> getArray()
    {
        int len = getInt();
        return getArrayLen<X>(len);
    }

    Vector getVector() { return getType<Vector>(); }
    Matrix getMatrix() { return getType<Matrix>(); }
    Transformation getTrafo() { return getType<Transformation>(); }
    Vector4 getVector4() { return getType<Vector4>(); }

    std::vector<float> getFloatLen(int num)
    { return getArrayLen<float>(num); }

    // For fixed-size strings where you already know the size
    std::string getString(size_t size)
    {
        std::string str;
        str.resize(size);
        if(inp->read(&str[0], size) != size)
            fail("Failed to read from NIF");
        return str.substr(0, str.find('\0'));
    }
    std::string getString()
    {
        size_t size = getInt();
        return getString(size);
    }
};

} // Namespace
#endif
