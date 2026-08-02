/// File to handle keys used by nif file records

#ifndef OPENMW_COMPONENTS_NIF_NIFKEY_HPP
#define OPENMW_COMPONENTS_NIF_NIFKEY_HPP

#include <utility>
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
        float mA; // Coefficients based on the TCB parameters
        float mB; // Only used by tangent calculations
        float mC;
        float mD;
    };

    template <typename T, T (NIFStream::*getValue)()>
    struct KeyMapT
    {
        // This is theoretically a "flat map" sorted by time
        using MapType = std::vector<std::pair<float, KeyT<T>>>;

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

            const uint32_t count = nif->get<uint32_t>();

            if (count == 0 && !morph)
                return;

            nif->read(mInterpolationType);

            mKeys.reserve(count);

            if (mInterpolationType == InterpolationType_Linear || mInterpolationType == InterpolationType_Constant)
            {
                nif->readVectorOfRecords(count, readValuePair, mKeys);
            }
            else if (mInterpolationType == InterpolationType_Quadratic)
            {
                nif->readVectorOfRecords(count, readQuadraticPair, mKeys);
            }
            else if (mInterpolationType == InterpolationType_TCB)
            {
                std::vector<TCBKey<T>> tcbKeys;
                nif->readVectorOfRecords(count, readTCBKey, tcbKeys);
                generateTCBTangents(tcbKeys);
                for (TCBKey<T>& tcbKey : tcbKeys)
                    mKeys.emplace_back(tcbKey.mTime,
                        KeyType{ std::move(tcbKey.mValue), std::move(tcbKey.mInTan), std::move(tcbKey.mOutTan) });
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

            // Note: NetImmerse does NOT sort keys or remove duplicates
        }

    private:
        static void readValue(NIFStream& nif, KeyType& key) { key.mValue = (nif.*getValue)(); }

        static void readValuePair(NIFStream& nif, std::pair<float, KeyType>& value)
        {
            nif.read(value.first);
            readValue(nif, value.second);
        }

        static void readQuadratic(NIFStream& nif, KeyType& key)
        {
            if constexpr (std::is_same_v<T, osg::Quat>)
            {
                readValue(nif, key);
            }
            else
            {
                readValue(nif, key);
                key.mInTan = (nif.*getValue)();
                key.mOutTan = (nif.*getValue)();
            }
        }

        static void readQuadraticPair(NIFStream& nif, std::pair<float, KeyType>& value)
        {
            nif.read(value.first);
            readQuadratic(nif, value.second);
        }

        static void readTCBKey(NIFStream& nif, TCBKey<T>& value)
        {
            float tension;
            float continuity;
            float bias;

            nif.read(value.mTime);
            value.mValue = (nif.*getValue)();
            nif.read(tension);
            nif.read(continuity);
            nif.read(bias);

            value.mA = (1.f - tension) * (1.f - continuity) * (1.f + bias);
            value.mB = (1.f - tension) * (1.f + continuity) * (1.f - bias);
            value.mC = (1.f - tension) * (1.f + continuity) * (1.f + bias);
            value.mD = (1.f - tension) * (1.f - continuity) * (1.f - bias);
        }

        template <typename U>
        static void generateTCBTangents(std::vector<TCBKey<U>>& keys)
        {
            if (keys.size() <= 1)
                return;

            {
                TCBKey<U>& first = keys[0];
                const U delta = keys[1].mValue - first.mValue;
                first.mInTan = delta * ((first.mA + first.mB) * 0.5f);
                first.mOutTan = delta * ((first.mC + first.mD) * 0.5f);
            }

            for (std::size_t i = 1; i < keys.size() - 1; ++i)
            {
                const TCBKey<U>& prev = keys[i - 1];
                const TCBKey<U>& next = keys[i + 1];
                const float timeSpan = next.mTime - prev.mTime;
                if (timeSpan == 0.f)
                    continue;
                TCBKey<U>& key = keys[i];
                const U prevDelta = key.mValue - prev.mValue;
                const U nextDelta = next.mValue - key.mValue;
                key.mInTan = (prevDelta * key.mA + nextDelta * key.mB) * ((key.mTime - prev.mTime) / timeSpan);
                key.mOutTan = (prevDelta * key.mC + nextDelta * key.mD) * ((next.mTime - key.mTime) / timeSpan);
            }

            {
                TCBKey<U>& last = keys.back();
                const U delta = last.mValue - keys[keys.size() - 2].mValue;
                last.mInTan = delta * ((last.mA + last.mB) * 0.5f);
                last.mOutTan = delta * ((last.mC + last.mD) * 0.5f);
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

    template <class Key, class Value>
    void readKeyMapPair(NIFStream& stream, std::pair<Key, KeyT<Value>>& value);

    template <>
    inline void readKeyMapPair(NIFStream& stream, std::pair<float, KeyT<bool>>& value)
    {
        stream.read(value.first);
        value.second.mValue = stream.get<uint8_t>() != 0;
    }

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
