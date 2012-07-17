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
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>

#include <stdexcept>
#include <vector>
#include <cassert>

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

    uint8_t read_byte()
    {
        uint8_t byte;
        if(inp->read(&byte, 1) != 1) return 0;
        return byte;
    }
    uint16_t read_le16()
    {
        uint8_t buffer[2];
        if(inp->read(buffer, 2) != 2) return 0;
        return buffer[0] | (buffer[1]<<8);
    }
    uint32_t read_le32()
    {
        uint8_t buffer[4];
        if(inp->read(buffer, 4) != 4) return 0;
        return buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
    }
    float read_le32f()
    {
        union {
            uint32_t i;
            float f;
        } u = { read_le32() };
        return u.f;
    }

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

    char getChar() { return read_byte(); }
    short getShort() { return read_le16(); }
    unsigned short getUShort() { return read_le16(); }
    int getInt() { return read_le32(); }
    float getFloat() { return read_le32f(); }
    Ogre::Vector3 getVector3()
    {
        float a[3];
        for(size_t i = 0;i < 3;i++)
            a[i] = getFloat();
        return Ogre::Vector3(a);
    }
    Ogre::Vector4 getVector4()
    {
        float a[4];
        for(size_t i = 0;i < 4;i++)
            a[i] = getFloat();
        return Ogre::Vector4(a);
    }
    Ogre::Matrix3 getMatrix3()
    {
        Ogre::Real a[3][3];
        for(size_t i = 0;i < 3;i++)
        {
            for(size_t j = 0;j < 3;j++)
                a[i][j] = Ogre::Real(getFloat());
        }
        return Ogre::Matrix3(a);
    }
    Transformation getTrafo()
    {
        Transformation t;
        t.pos = getVector3();
        t.rotation = getMatrix3();
        t.scale = getFloat();
        t.velocity = getVector3();
        return t;
    }

    std::string getString(size_t length)
    {
        std::string str;
        str.resize(length);
        if(inp->read(&str[0], length) != length)
            return std::string();
        return str.substr(0, str.find('\0'));
    }
    std::string getString()
    {
        size_t size = read_le32();
        return getString(size);
    }

    void getShorts(std::vector<short> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getShort();
    }
    void getFloats(std::vector<float> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getFloat();
    }
};

} // Namespace
#endif
