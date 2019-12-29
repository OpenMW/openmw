///File to handle keys used by nif file records

#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
#define OPENMW_COMPONENTS_NIF_NIFKEY_HPP

#include "nifstream.hpp"

#include <sstream>
#include <map>

#include "niffile.hpp"

namespace Nif
{

template<typename T>
struct KeyT {
    T mValue;

    // FIXME: Implement Quadratic and TBC interpolation
    /*
    T mForwardValue;  // Only for Quadratic interpolation, and never for QuaternionKeyList
    T mBackwardValue; // Only for Quadratic interpolation, and never for QuaternionKeyList
    float mTension;    // Only for TBC interpolation
    float mBias;       // Only for TBC interpolation
    float mContinuity; // Only for TBC interpolation
    */
};
using FloatKey = KeyT<float>;
using Vector3Key = KeyT<osg::Vec3f>;
using Vector4Key = KeyT<osg::Vec4f>;
using QuaternionKey = KeyT<osg::Quat>;

template<typename T, T (NIFStream::*getValue)()>
struct KeyMapT {
    using MapType = std::map<float, KeyT<T>>;

    using ValueType = T;
    using KeyType = KeyT<T>;

    enum InterpolationType
    {
        Unknown = 0,
        Linear = 1,
        Quadratic = 2,
        TBC = 3,
        XYZ = 4,
        Constant = 5
    };

    unsigned int mInterpolationType = Linear;
    MapType mKeys;

    //Read in a KeyGroup (see http://niftools.sourceforge.net/doc/nif/NiKeyframeData.html)
    void read(NIFStream *nif, bool force=false)
    {
        assert(nif);

        mInterpolationType = Unknown;

        size_t count = nif->getUInt();
        if(count == 0 && !force)
            return;

        mKeys.clear();

        mInterpolationType = nif->getUInt();

        KeyT<T> key;
        NIFStream &nifReference = *nif;

        if (mInterpolationType == Linear || mInterpolationType == Constant)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readValue(nifReference, key);
                mKeys[time] = key;
            }
        }
        else if (mInterpolationType == Quadratic)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readQuadratic(nifReference, key);
                mKeys[time] = key;
            }
        }
        else if (mInterpolationType == TBC)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readTBC(nifReference, key);
                mKeys[time] = key;
            }
        }
        //XYZ keys aren't actually read here.
        //data.hpp sees that the last type read was sXYZInterpolation and:
        //    Eats a floating point number, then
        //    Re-runs the read function 3 more times.
        //        When it does that it's reading in a bunch of sLinearInterpolation keys, not sXYZInterpolation.
        else if(mInterpolationType == XYZ)
        {
            //Don't try to read XYZ keys into the wrong part
            if ( count != 1 )
            {
                std::stringstream error;
                error << "XYZ_ROTATION_KEY count should always be '1' .  Retrieved Value: "
                      << count;
                nif->file->fail(error.str());
            }
        }
        else if (mInterpolationType == Unknown)
        {
            if (count != 0)
                nif->file->fail("Interpolation type 0 doesn't work with keys");
        }
        else
        {
            std::stringstream error;
            error << "Unhandled interpolation type: " << mInterpolationType;
            nif->file->fail(error.str());
        }
    }

private:
    static void readValue(NIFStream &nif, KeyT<T> &key)
    {
        key.mValue = (nif.*getValue)();
    }

    template <typename U>
    static void readQuadratic(NIFStream &nif, KeyT<U> &key)
    {
        readValue(nif, key);
        /*key.mForwardValue = */(nif.*getValue)();
        /*key.mBackwardValue = */(nif.*getValue)();
    }

    static void readQuadratic(NIFStream &nif, KeyT<osg::Quat> &key)
    {
        readValue(nif, key);
    }

    static void readTBC(NIFStream &nif, KeyT<T> &key)
    {
        readValue(nif, key);
        /*key.mTension = */nif.getFloat();
        /*key.mBias = */nif.getFloat();
        /*key.mContinuity = */nif.getFloat();
    }
};
using FloatKeyMap = KeyMapT<float,&NIFStream::getFloat>;
using Vector3KeyMap = KeyMapT<osg::Vec3f,&NIFStream::getVector3>;
using Vector4KeyMap = KeyMapT<osg::Vec4f,&NIFStream::getVector4>;
using QuaternionKeyMap = KeyMapT<osg::Quat,&NIFStream::getQuaternion>;
using ByteKeyMap = KeyMapT<char,&NIFStream::getChar>;

using FloatKeyMapPtr = std::shared_ptr<FloatKeyMap>;
using Vector3KeyMapPtr = std::shared_ptr<Vector3KeyMap>;
using Vector4KeyMapPtr = std::shared_ptr<Vector4KeyMap>;
using QuaternionKeyMapPtr = std::shared_ptr<QuaternionKeyMap>;
using ByteKeyMapPtr = std::shared_ptr<ByteKeyMap>;

} // Namespace
#endif //#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
