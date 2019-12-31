///File to handle keys used by nif file records

#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
#define OPENMW_COMPONENTS_NIF_NIFKEY_HPP

#include "nifstream.hpp"

#include <sstream>
#include <map>

#include "niffile.hpp"

namespace Nif
{

enum InterpolationType
{
    InterpolationType_Unknown = 0,
    InterpolationType_Linear = 1,
    InterpolationType_Quadratic = 2,
    InterpolationType_TBC = 3,
    InterpolationType_XYZ = 4,
    InterpolationType_Constant = 5
};

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

    unsigned int mInterpolationType = InterpolationType_Linear;
    MapType mKeys;

    //Read in a KeyGroup (see http://niftools.sourceforge.net/doc/nif/NiKeyframeData.html)
    void read(NIFStream *nif, bool force=false)
    {
        assert(nif);

        mInterpolationType = InterpolationType_Unknown;

        size_t count = nif->getUInt();
        if(count == 0 && !force)
            return;

        mKeys.clear();

        mInterpolationType = nif->getUInt();

        KeyT<T> key;
        NIFStream &nifReference = *nif;

        if (mInterpolationType == InterpolationType_Linear
         || mInterpolationType == InterpolationType_Constant)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readValue(nifReference, key);
                mKeys[time] = key;
            }
        }
        else if (mInterpolationType == InterpolationType_Quadratic)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readQuadratic(nifReference, key);
                mKeys[time] = key;
            }
        }
        else if (mInterpolationType == InterpolationType_TBC)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readTBC(nifReference, key);
                mKeys[time] = key;
            }
        }
        //XYZ keys aren't actually read here.
        //data.hpp sees that the last type read was InterpolationType_XYZ and:
        //    Eats a floating point number, then
        //    Re-runs the read function 3 more times.
        //        When it does that it's reading in a bunch of InterpolationType_Linear keys, not InterpolationType_XYZ.
        else if(mInterpolationType == InterpolationType_XYZ)
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
        else if (mInterpolationType == InterpolationType_Unknown)
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
