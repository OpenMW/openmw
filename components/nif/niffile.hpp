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

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

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
#include "niftypes.hpp"
#include "nifstream.hpp"

namespace Nif
{

class NIFFile
{
    enum NIFVersion {
        VER_MW    = 0x04000002    // Morrowind NIFs
    };

    /// Nif file version
    int ver;

    /// File name, used for error messages
    std::string filename;

    /// Record list
    std::vector<Record*> records;

    /// Parse the file
    void parse();

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
        std::cerr << "NIFFile Warning: " << msg <<std::endl
                  << "File: " << filename <<std::endl;
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
    size_t numRecords() { return records.size(); }
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

template<typename T, T (NIFStream::*getValue)()>
struct KeyListT {
    typedef std::vector< KeyT<T> > VecType;

    static const int sLinearInterpolation = 1;
    static const int sQuadraticInterpolation = 2;
    static const int sTBCInterpolation = 3;

    int mInterpolationType;
    VecType mKeys;

    void read(NIFStream *nif, bool force=false)
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
            nif->file->warn("Unhandled interpolation type: "+Ogre::StringConverter::toString(mInterpolationType));
    }
};
typedef KeyListT<float,&NIFStream::getFloat> FloatKeyList;
typedef KeyListT<Ogre::Vector3,&NIFStream::getVector3> Vector3KeyList;
typedef KeyListT<Ogre::Vector4,&NIFStream::getVector4> Vector4KeyList;
typedef KeyListT<Ogre::Quaternion,&NIFStream::getQuaternion> QuaternionKeyList;

} // Namespace
#endif
