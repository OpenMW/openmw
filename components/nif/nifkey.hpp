///File to handle keys used by nif file records

#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
#define OPENMW_COMPONENTS_NIF_NIFKEY_HPP

#include <OgreStringConverter.h>

#include "nifstream.hpp"

namespace Nif
{

template<typename T>
struct KeyT {
    float mTime;
    T mValue;
    T mForwardValue;  // Only for Quadratic interpolation, and never for QuaternionKeyList
    T mBackwardValue; // Only for Quadratic interpolation, and never for QuaternionKeyList
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

    static const unsigned int sLinearInterpolation = 1;
    static const unsigned int sQuadraticInterpolation = 2;
    static const unsigned int sTBCInterpolation = 3;
    static const unsigned int sXYZInterpolation = 4;

    unsigned int mInterpolationType;
    VecType mKeys;

    KeyListT() : mInterpolationType(sLinearInterpolation) {}

    //Read in a KeyGroup (see http://niftools.sourceforge.net/doc/nif/NiKeyframeData.html)
    void read(NIFStream *nif, bool force=false)
    {
        assert(nif);

        mInterpolationType = 0;

        size_t count = nif->getUInt();
        if(count == 0 && !force)
            return;

        //If we aren't forcing things, make sure that read clears any previous keys
        if(!force)
            mKeys.clear();

        mInterpolationType = nif->getUInt();

        KeyT<T> key;
        NIFStream &nifReference = *nif;

        if(mInterpolationType == sLinearInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                readTimeAndValue(nifReference, key);
                mKeys.push_back(key);
            }
        }
        else if(mInterpolationType == sQuadraticInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                readQuadratic(nifReference, key);
                mKeys.push_back(key);
            }
        }
        else if(mInterpolationType == sTBCInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                readTBC(nifReference, key);
                mKeys.push_back(key);
            }
        }
        //XYZ keys aren't actually read here.
        //data.hpp sees that the last type read was sXYZInterpolation and:
        //    Eats a floating point number, then
        //    Re-runs the read function 3 more times, with force enabled so that the previous values aren't cleared.
        //        When it does that it's reading in a bunch of sLinearInterpolation keys, not sXYZInterpolation.
        else if(mInterpolationType == sXYZInterpolation)
        {
            //Don't try to read XYZ keys into the wrong part
            if ( count != 1 )
                nif->file->fail("XYZ_ROTATION_KEY count should always be '1' .  Retrieved Value: "+Ogre::StringConverter::toString(count));
        }
        else if (0 == mInterpolationType)
        {
            if (count != 0)
                nif->file->fail("Interpolation type 0 doesn't work with keys");
        }
        else
            nif->file->fail("Unhandled interpolation type: "+Ogre::StringConverter::toString(mInterpolationType));
    }

private:
    static void readTimeAndValue(NIFStream &nif, KeyT<T> &key)
    {
        key.mTime = nif.getFloat();
        key.mValue = (nif.*getValue)();
    }

    static void readQuadratic(NIFStream &nif, KeyT<Ogre::Quaternion> &key)
    {
        readTimeAndValue(nif, key);
    }

    template <typename U>
    static void readQuadratic(NIFStream &nif, KeyT<U> &key)
    {
        readTimeAndValue(nif, key);
        key.mForwardValue = (nif.*getValue)();
        key.mBackwardValue = (nif.*getValue)();
    }

    static void readTBC(NIFStream &nif, KeyT<T> &key)
    {
        readTimeAndValue(nif, key);
        key.mTension = nif.getFloat();
        key.mBias = nif.getFloat();
        key.mContinuity = nif.getFloat();
    }
};
typedef KeyListT<float,&NIFStream::getFloat> FloatKeyList;
typedef KeyListT<Ogre::Vector3,&NIFStream::getVector3> Vector3KeyList;
typedef KeyListT<Ogre::Vector4,&NIFStream::getVector4> Vector4KeyList;
typedef KeyListT<Ogre::Quaternion,&NIFStream::getQuaternion> QuaternionKeyList;

} // Namespace
#endif //#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP