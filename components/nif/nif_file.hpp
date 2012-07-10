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

    uint32_t read_le32()
    {
        uint8_t buffer[4];
        if(inp->read(buffer, 4) != 4) return 0;
        return buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
    }
    uint16_t read_le16()
    {
        uint8_t buffer[2];
        if(inp->read(buffer, 2) != 2) return 0;
        return buffer[0] | (buffer[1]<<8);
    }
    uint8_t read_byte()
    {
        uint8_t byte;
        if(inp->read(&byte, 1) != 1) return 0;
        return byte;
    }
    std::string read_string(size_t length)
    {
        std::string str;
        str.resize(length);
        if(inp->read(&str[0], length) != length)
            return std::string();
        return str.substr(0, str.find('\0'));
    }


    char& load(char &c) { c = read_byte(); return c; }
    unsigned char& load(unsigned char &c) { c = read_byte(); return c; }
    short& load(short &s) { s = read_le16(); return s; }
    unsigned short& load(unsigned short &s) { s = read_le16(); return s; }
    int& load(int &i) { i = read_le32(); return i; }
    unsigned int& load(unsigned int &i) { i = read_le32(); return i; }
    float& load(float &f)
    {
        union {
            int i;
            float f;
        } u = { read_le32() };
        f = u.f;
        return f;
    }

    template<typename T, size_t N>
    T* load(T (&a)[N])
    {
        for(size_t i = 0;i < N;i++)
            load(a[i]);
        return a;
    }

    template<typename T>
    std::vector<T>& load(std::vector<T> &v, size_t size)
    {
        v.resize(size);
        for(size_t i = 0;i < size;i++)
            load(v[i]);
        return v;
    }


    template<typename X>
    std::vector<X> getArrayLen(size_t num)
    {
        std::vector<X> v(num);
        if(inp->read(&v[0], num*sizeof(X)) != num*sizeof(X))
            fail("Failed to read from NIF");
        return v;
    }

    template<typename X>
    std::vector<X> getArray()
    {
        size_t len = read_le32();
        return getArrayLen<X>(len);
    }

    char getByte() { char c; return load(c); }
    unsigned short getShort() { unsigned short s; return load(s); }
    int getInt() { int i; return load(i); }
    float getFloat() { float f; return load(f); }
    Vector getVector()
    {
        Vector v;
        load(v.array);
        return v;
    }
    Vector4 getVector4()
    {
        Vector4 v;
        load(v.array);
        return v;
    }
    Matrix getMatrix()
    {
        Matrix m;
        m.v[0] = getVector();
        m.v[1] = getVector();
        m.v[2] = getVector();
        return m;
    }
    Transformation getTrafo()
    {
        Transformation t;
        t.pos = getVector();
        t.rotation = getMatrix();
        load(t.scale);
        t.velocity = getVector();
        return t;
    }


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

template<>
inline std::vector<float> NIFFile::getArrayLen<float>(size_t num)
{
    std::vector<float> v(num);
    for(size_t i = 0;i < num;i++)
        load(v[i]);
    return v;
}

} // Namespace
#endif
