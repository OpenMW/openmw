///File to handle keys used by nif file records

#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
#define OPENMW_COMPONENTS_NIF_NIFKEY_HPP

#include "nifstream.hpp"

#include <sstream>
#include <map>

#include <boost/shared_ptr.hpp>

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
typedef KeyT<float> FloatKey;
typedef KeyT<osg::Vec3f> Vector3Key;
typedef KeyT<osg::Vec4f> Vector4Key;
typedef KeyT<osg::Quat> QuaternionKey;

template<typename T, T (NIFStream::*getValue)()>
struct KeyMapT {
    typedef std::map< float, KeyT<T> > MapType;

    static const unsigned int sLinearInterpolation = 1;
    static const unsigned int sQuadraticInterpolation = 2;
    static const unsigned int sTBCInterpolation = 3;
    static const unsigned int sXYZInterpolation = 4;

    unsigned int mInterpolationType;
    MapType mKeys;

    KeyMapT() : mInterpolationType(sLinearInterpolation) {}

    //Read in a KeyGroup (see http://niftools.sourceforge.net/doc/nif/NiKeyframeData.html)
    void read(NIFStream *nif, bool force=false)
    {
        assert(nif);

        mInterpolationType = 0;

        size_t count = nif->getUInt();
        if(count == 0 && !force)
            return;

        mKeys.clear();

        mInterpolationType = nif->getUInt();

        KeyT<T> key;
        NIFStream &nifReference = *nif;

        if(mInterpolationType == sLinearInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readValue(nifReference, key);
                mKeys[time] = key;
            }
        }
        else if(mInterpolationType == sQuadraticInterpolation)
        {
            for(size_t i = 0;i < count;i++)
            {
                float time = nif->getFloat();
                readQuadratic(nifReference, key);
                mKeys[time] = key;
            }
        }
        else if(mInterpolationType == sTBCInterpolation)
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
        else if(mInterpolationType == sXYZInterpolation)
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
        else if (0 == mInterpolationType)
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

    static void readTBC(NIFStream &nif, KeyT<T> &key)
    {
        readValue(nif, key);
        /*key.mTension = */nif.getFloat();
        /*key.mBias = */nif.getFloat();
        /*key.mContinuity = */nif.getFloat();
    }
};
typedef KeyMapT<float,&NIFStream::getFloat> FloatKeyMap;
typedef KeyMapT<osg::Vec3f,&NIFStream::getVector3> Vector3KeyMap;
typedef KeyMapT<osg::Vec4f,&NIFStream::getVector4> Vector4KeyMap;
typedef KeyMapT<osg::Quat,&NIFStream::getQuaternion> QuaternionKeyMap;

typedef boost::shared_ptr<FloatKeyMap> FloatKeyMapPtr;
typedef boost::shared_ptr<Vector3KeyMap> Vector3KeyMapPtr;
typedef boost::shared_ptr<Vector4KeyMap> Vector4KeyMapPtr;
typedef boost::shared_ptr<QuaternionKeyMap> QuaternionKeyMapPtr;

} // Namespace
#endif //#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
