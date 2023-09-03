#ifndef OPENMW_COMPONENTS_NIF_EXTRA_HPP
#define OPENMW_COMPONENTS_NIF_EXTRA_HPP

#include "base.hpp"

namespace Nif
{

    template <typename T>
    struct TypedExtra : public Extra
    {
        T mData;

        void read(NIFStream* nif) override
        {
            Extra::read(nif);

            nif->read(mData);
        }
    };

    template <typename T>
    struct TypedVectorExtra : public Extra
    {
        std::vector<T> mData;

        void read(NIFStream* nif) override
        {
            Extra::read(nif);

            nif->readVector(mData, nif->get<uint32_t>());
        }
    };

    using NiBooleanExtraData = TypedExtra<bool>;
    using NiFloatExtraData = TypedExtra<float>;
    using NiIntegerExtraData = TypedExtra<uint32_t>;
    using NiStringExtraData = TypedExtra<std::string>;
    using NiVectorExtraData = TypedExtra<osg::Vec4f>;

    using NiBinaryExtraData = TypedVectorExtra<uint8_t>;
    using NiFloatsExtraData = TypedVectorExtra<float>;
    using NiIntegersExtraData = TypedVectorExtra<uint32_t>;

    // Distinct from NiBinaryExtraData, uses mRecordSize as its size
    struct NiExtraData : public Extra
    {
        std::vector<uint8_t> mData;

        void read(NIFStream* nif) override;
    };

    struct NiVertWeightsExtraData : public Extra
    {
        void read(NIFStream* nif) override;
    };

    struct NiTextKeyExtraData : public Extra
    {
        struct TextKey
        {
            float mTime;
            std::string mText;
        };
        std::vector<TextKey> mList;

        void read(NIFStream* nif) override;
    };

    struct BSBound : public Extra
    {
        osg::Vec3f mCenter, mExtents;

        void read(NIFStream* nif) override;
    };

    struct BSFurnitureMarker : public Extra
    {
        struct LegacyFurniturePosition
        {
            osg::Vec3f mOffset;
            uint16_t mOrientation;
            uint8_t mPositionRef;
            void read(NIFStream* nif);
        };

        struct FurniturePosition
        {
            osg::Vec3f mOffset;
            float mHeading;
            uint16_t mType;
            uint16_t mEntryPoint;
            void read(NIFStream* nif);
        };

        std::vector<LegacyFurniturePosition> mLegacyMarkers;
        std::vector<FurniturePosition> mMarkers;

        void read(NIFStream* nif) override;
    };

    struct BSInvMarker : public Extra
    {
        osg::Quat mRotation;
        float mScale;

        void read(NIFStream* nif) override;
    };

    struct BSBehaviorGraphExtraData : public Extra
    {
        std::string mFile;
        bool mControlsBaseSkeleton;

        void read(NIFStream* nif) override;
    };

}
#endif
