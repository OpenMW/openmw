/// File to handle keys used by nif file records

#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
#define OPENMW_COMPONENTS_NIF_NIFKEY_HPP

#include <algorithm>
#include <map>
#include <vector>

#include "exception.hpp"
#include "niffile.hpp"
#include "nifstream.hpp"

namespace Nif
{

    enum InterpolationType
    {
        InterpolationType_Unknown = 0,
        InterpolationType_Linear = 1,
        InterpolationType_Quadratic = 2,
        InterpolationType_TCB = 3,
        InterpolationType_XYZ = 4,
        InterpolationType_Constant = 5
    };

    template <typename T>
    struct KeyT
    {
        T mValue;
        T mInTan; // Only for Quadratic interpolation, and never for QuaternionKeyList
        T mOutTan; // Only for Quadratic interpolation, and never for QuaternionKeyList
    };

    template <typename T>
    struct TCBKey
    {
        float mTime;
        T mValue{};
        T mInTan{};
        T mOutTan{};
        float mTension;
        float mContinuity;
        float mBias;
    };

    template <typename T, T (NIFStream::*getValue)()>
    struct KeyMapT
    {
        using MapType = std::map<float, KeyT<T>>;

        using ValueType = T;
        using KeyType = KeyT<T>;

        std::string mFrameName;
        float mLegacyWeight;
        uint32_t mInterpolationType = InterpolationType_Unknown;
        MapType mKeys;

        // Read in a KeyGroup (see http://niftools.sourceforge.net/doc/nif/NiKeyframeData.html)
        void read(NIFStream* nif, bool morph = false)
        {
            assert(nif);

            if (morph)
            {
                if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 106))
                    nif->read(mFrameName);

                if (nif->getVersion() > NIFStream::generateVersion(10, 1, 0, 0))
                {
                    if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 104)
                        && nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 2) && nif->getBethVersion() < 10)
                        nif->read(mLegacyWeight);
                    return;
                }
            }

            uint32_t count;
            nif->read(count);

            if (count != 0 || morph)
                nif->read(mInterpolationType);

            KeyType key = {};

            if (mInterpolationType == InterpolationType_Linear || mInterpolationType == InterpolationType_Constant)
            {
                for (size_t i = 0; i < count; i++)
                {
                    float time;
                    nif->read(time);
                    readValue(*nif, key);
                    mKeys[time] = key;
                }
            }
            else if (mInterpolationType == InterpolationType_Quadratic)
            {
                for (size_t i = 0; i < count; i++)
                {
                    float time;
                    nif->read(time);
                    readQuadratic(*nif, key);
                    mKeys[time] = key;
                }
            }
            else if (mInterpolationType == InterpolationType_TCB)
            {
                std::vector<TCBKey<T>> tcbKeys(count);
                for (TCBKey<T>& key : tcbKeys)
                {
                    nif->read(key.mTime);
                    key.mValue = ((*nif).*getValue)();
                    nif->read(key.mTension);
                    nif->read(key.mContinuity);
                    nif->read(key.mBias);
                }
                generateTCBTangents(tcbKeys);
                for (TCBKey<T>& key : tcbKeys)
                    mKeys[key.mTime] = KeyType{ std::move(key.mValue), std::move(key.mInTan), std::move(key.mOutTan) };
            }
            else if (mInterpolationType == InterpolationType_XYZ)
            {
                // XYZ keys aren't actually read here.
                // data.cpp sees that the last type read was InterpolationType_XYZ and:
                //     Eats a floating point number, then
                //     Re-runs the read function 3 more times.
                //         When it does that it's reading in a bunch of InterpolationType_Linear keys, not
                //         InterpolationType_XYZ.
            }
            else if (count != 0)
            {
                throw Nif::Exception("Unhandled interpolation type: " + std::to_string(mInterpolationType),
                    nif->getFile().getFilename());
            }
        }

    private:
        static void readValue(NIFStream& nif, KeyT<T>& key) { key.mValue = (nif.*getValue)(); }

        template <typename U>
        static void readQuadratic(NIFStream& nif, KeyT<U>& key)
        {
            readValue(nif, key);
            key.mInTan = (nif.*getValue)();
            key.mOutTan = (nif.*getValue)();
        }

        static void readQuadratic(NIFStream& nif, KeyT<osg::Quat>& key) { readValue(nif, key); }

        template <typename U>
        static void generateTCBTangents(std::vector<TCBKey<U>>& keys)
        {
            if (keys.size() <= 1)
                return;

            std::sort(keys.begin(), keys.end(), [](const auto& a, const auto& b) { return a.mTime < b.mTime; });
            for (size_t i = 0; i < keys.size(); ++i)
            {
                TCBKey<U>& curr = keys[i];
                const TCBKey<U>& prev = (i == 0) ? curr : keys[i - 1];
                const TCBKey<U>& next = (i == keys.size() - 1) ? curr : keys[i + 1];
                const float prevLen = curr.mTime - prev.mTime;
                const float nextLen = next.mTime - curr.mTime;
                if (prevLen + nextLen <= 0.f)
                    continue;

                const U prevDelta = curr.mValue - prev.mValue;
                const U nextDelta = next.mValue - curr.mValue;
                const float t = curr.mTension;
                const float c = curr.mContinuity;
                const float b = curr.mBias;

                U x{}, y{}, z{}, w{};
                if (prevLen > 0.f)
                    x = prevDelta / prevLen * (1 - t) * (1 - c) * (1 + b);
                if (nextLen > 0.f)
                    y = nextDelta / nextLen * (1 - t) * (1 + c) * (1 - b);
                if (prevLen > 0.f)
                    z = prevDelta / prevLen * (1 - t) * (1 + c) * (1 + b);
                if (nextLen > 0.f)
                    w = nextDelta / nextLen * (1 - t) * (1 - c) * (1 - b);

                curr.mInTan = (x + y) * prevLen / (prevLen + nextLen);
                curr.mOutTan = (z + w) * nextLen / (prevLen + nextLen);
            }
        }

        static void generateTCBTangents(std::vector<TCBKey<bool>>& keys)
        {
            // TODO: is this even legal?
        }

        static void generateTCBTangents(std::vector<TCBKey<osg::Quat>>& keys)
        {
            // TODO: implement TCB interpolation for quaternions
        }
    };
    using FloatKeyMap = KeyMapT<float, &NIFStream::get<float>>;
    using Vector3KeyMap = KeyMapT<osg::Vec3f, &NIFStream::get<osg::Vec3f>>;
    using Vector4KeyMap = KeyMapT<osg::Vec4f, &NIFStream::get<osg::Vec4f>>;
    using QuaternionKeyMap = KeyMapT<osg::Quat, &NIFStream::get<osg::Quat>>;
    using BoolKeyMap = KeyMapT<bool, &NIFStream::get<bool>>;

    using FloatKeyMapPtr = std::shared_ptr<FloatKeyMap>;
    using Vector3KeyMapPtr = std::shared_ptr<Vector3KeyMap>;
    using Vector4KeyMapPtr = std::shared_ptr<Vector4KeyMap>;
    using QuaternionKeyMapPtr = std::shared_ptr<QuaternionKeyMap>;
    using BoolKeyMapPtr = std::shared_ptr<BoolKeyMap>;

} // Namespace
#endif //#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
