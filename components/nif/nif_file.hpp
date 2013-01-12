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
#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>
#include <OgreQuaternion.h>
#include <OgreStringConverter.h>

#include <stdexcept>
#include <vector>
#include <cassert>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/detail/endian.hpp>

#include <libs/platform/stdint.h>

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

    class LoadedCache;
    friend class LoadedCache;

    // attempt to protect NIFFile from misuse...
    struct psudo_private_modifier {}; // this dirty little trick should optimize out
    NIFFile (NIFFile const &);
    void operator = (NIFFile const &);

public:
    /// Used for error handling
    void fail(const std::string &msg)
    {
        std::string err = "NIFFile Error: " + msg;
        err += "\nFile: " + filename;
        throw std::runtime_error(err);
    }

    void warn(const std::string &msg)
    {
        std::cerr<< "NIFFile Warning: "<<msg <<std::endl
                 << "File: "<<filename <<std::endl;
    }

    typedef boost::shared_ptr <NIFFile> ptr;

    /// Open a NIF stream. The name is used for error messages.
    NIFFile(const std::string &name, psudo_private_modifier);
    ~NIFFile();

    static ptr create (const std::string &name);
    static void lockCache ();
    static void unlockCache ();

    struct CacheLock
    {
        CacheLock () { lockCache (); }
        ~CacheLock () { unlockCache (); }
    };

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
    Ogre::Vector2 getVector2()
    {
        float a[2];
        for(size_t i = 0;i < 2;i++)
            a[i] = getFloat();
        return Ogre::Vector2(a);
    }
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
    Ogre::Quaternion getQuaternion()
    {
        float a[4];
        for(size_t i = 0;i < 4;i++)
            a[i] = getFloat();
        return Ogre::Quaternion(a);
    }
    Transformation getTrafo()
    {
        Transformation t;
        t.pos = getVector3();
        t.rotation = getMatrix3();
        t.scale = getFloat();
        return t;
    }

    std::string getString(size_t length)
    {
        std::vector<char> str (length+1, 0);

        if(inp->read(&str[0], length) != length)
            throw std::runtime_error ("string length in NIF file does not match");

        return &str[0];
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
    void getVector2s(std::vector<Ogre::Vector2> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getVector2();
    }
    void getVector3s(std::vector<Ogre::Vector3> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getVector3();
    }
    void getVector4s(std::vector<Ogre::Vector4> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getVector4();
    }
};


template<typename T>
struct KeyT {
    float mTime;
    T mValue;
    T mForwardValue;  // Only for Quadratic interpolation
    T mBackwardValue; // Only for Quadratic interpolation
    float mTension;    // Only for TBC interpolation
    float mBias;       // Only for TBC interpolation
    float mContinuity; // Only for TBC interpolation
};
typedef KeyT<float> FloatKey;
typedef KeyT<Ogre::Vector3> Vector3Key;
typedef KeyT<Ogre::Vector4> Vector4Key;
typedef KeyT<Ogre::Quaternion> QuaternionKey;

template<typename T, T (NIFFile::*getValue)()>
struct KeyListT {
    typedef std::vector< KeyT<T> > VecType;

    static const int sLinearInterpolation = 1;
    static const int sQuadraticInterpolation = 2;
    static const int sTBCInterpolation = 3;

    int mInterpolationType;
    VecType mKeys;

    void read(NIFFile *nif, bool force=false)
    {
        size_t count = nif->getInt();
        if(count == 0 && !force)
            return;

        mInterpolationType = nif->getInt();
        mKeys.resize(count);
        if(mInterpolationType == sLinearInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                KeyT<T> &key = mKeys[i];
                key.mTime = nif->getFloat();
                key.mValue = (nif->*getValue)();
            }
        }
        else if(mInterpolationType == sQuadraticInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                KeyT<T> &key = mKeys[i];
                key.mTime = nif->getFloat();
                key.mValue = (nif->*getValue)();
                key.mForwardValue = (nif->*getValue)();
                key.mBackwardValue = (nif->*getValue)();
            }
        }
        else if(mInterpolationType == sTBCInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                KeyT<T> &key = mKeys[i];
                key.mTime = nif->getFloat();
                key.mValue = (nif->*getValue)();
                key.mTension = nif->getFloat();
                key.mBias = nif->getFloat();
                key.mContinuity = nif->getFloat();
            }
        }
        else
            nif->warn("Unhandled interpolation type: "+Ogre::StringConverter::toString(mInterpolationType));
    }
};
typedef KeyListT<float,&NIFFile::getFloat> FloatKeyList;
typedef KeyListT<Ogre::Vector3,&NIFFile::getVector3> Vector3KeyList;
typedef KeyListT<Ogre::Vector4,&NIFFile::getVector4> Vector4KeyList;
typedef KeyListT<Ogre::Quaternion,&NIFFile::getQuaternion> QuaternionKeyList;

} // Namespace
#endif
