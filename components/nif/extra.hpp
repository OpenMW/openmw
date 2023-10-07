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

    using BSDistantObjectExtraData = TypedExtra<uint32_t>;
    using BSDistantObjectLargeRefExtraData = TypedExtra<bool>;

    using NiBinaryExtraData = TypedVectorExtra<uint8_t>;
    using NiFloatsExtraData = TypedVectorExtra<float>;
    using NiIntegersExtraData = TypedVectorExtra<uint32_t>;

    using BSEyeCenterExtraData = TypedVectorExtra<float>;
    using BSPositionData = TypedVectorExtra<Misc::float16_t>;
    using BSWArray = TypedVectorExtra<int32_t>;

    // Distinct from NiBinaryExtraData, uses mRecordSize as its size
    struct NiExtraData : public Extra
    {
        std::vector<uint8_t> mData;

        void read(NIFStream* nif) override;
    };

    // != TypedVectorExtra<std::string>, doesn't use the string table
    struct NiStringsExtraData : public Extra
    {
        std::vector<std::string> mData;

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

    struct BSBoneLODExtraData : public Extra
    {
        struct BoneLOD
        {
            uint32_t mDistance;
            std::string mBone;

            void read(NIFStream* nif);
        };

        std::vector<BoneLOD> mData;

        void read(NIFStream* nif) override;
    };

    struct BSDecalPlacementVectorExtraData : public NiFloatExtraData
    {
        struct Block
        {
            std::vector<osg::Vec3f> mPoints;
            std::vector<osg::Vec3f> mNormals;

            void read(NIFStream* nif);
        };

        std::vector<Block> mBlocks;

        void read(NIFStream* nif) override;
    };

    struct BSExtraData : NiExtraData
    {
        void read(NIFStream* nif) override {}
    };

    struct BSClothExtraData : BSExtraData
    {
        std::vector<uint8_t> mData;

        void read(NIFStream* nif) override;
    };

    struct BSConnectPoint
    {
        struct Point
        {
            std::string mParent;
            std::string mName;
            NiQuatTransform mTransform;

            void read(NIFStream* nif);
        };

        struct Parents : NiExtraData
        {
            std::vector<Point> mPoints;

            void read(NIFStream* nif) override;
        };

        struct Children : NiExtraData
        {
            bool mSkinned;
            std::vector<std::string> mPointNames;

            void read(NIFStream* nif) override;
        };
    };

}
#endif
