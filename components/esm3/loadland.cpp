#include "loadland.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>

#include <components/esm/defs.hpp>
#include <components/misc/concepts.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        struct VHGT
        {
            float mHeightOffset;
            std::int8_t mHeightData[LandRecordData::sLandNumVerts];
        };

        template <Misc::SameAsWithoutCvref<VHGT> T>
        void decompose(T&& v, const auto& f)
        {
            char padding[3] = { 0, 0, 0 };
            f(v.mHeightOffset, v.mHeightData, padding);
        }

        void transposeTextureData(const std::uint16_t* in, std::uint16_t* out)
        {
            size_t readPos = 0; // bit ugly, but it works
            for (size_t y1 = 0; y1 < 4; y1++)
                for (size_t x1 = 0; x1 < 4; x1++)
                    for (size_t y2 = 0; y2 < 4; y2++)
                        for (size_t x2 = 0; x2 < 4; x2++)
                            out[(y1 * 4 + y2) * 16 + (x1 * 4 + x2)] = in[readPos++];
        }

        // Loads data and marks it as loaded. Return true if data is actually loaded from reader, false otherwise
        // including the case when data is already loaded.
        bool condLoad(ESMReader& reader, int flags, int& targetFlags, int dataFlag, auto& in)
        {
            if ((targetFlags & dataFlag) == 0 && (flags & dataFlag) != 0)
            {
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(in)>, VHGT>)
                    reader.getSubComposite(in);
                else
                    reader.getHT(in);
                targetFlags |= dataFlag;
                return true;
            }
            reader.skipHSub();
            return false;
        }
    }

    void Land::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;

        bool hasLocation = false;
        bool isLoaded = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("INTV"):
                    esm.getHT(mX, mY);
                    hasLocation = true;
                    break;
                case fourCC("DATA"):
                    esm.getHT(mFlags);
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.cacheSubName();
                    isLoaded = true;
                    break;
            }
        }

        if (!hasLocation)
            esm.fail("Missing INTV subrecord");

        mContext = esm.getContext();

        mLandData = nullptr;
        std::fill(std::begin(mWnam), std::end(mWnam), 0);

        // Skip the land data here. Load it when the cell is loaded.
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("VNML"):
                    esm.skipHSub();
                    if (mFlags & Flag_HeightsNormals)
                        mDataTypes |= DATA_VNML;
                    break;
                case fourCC("VHGT"):
                    esm.skipHSub();
                    if (mFlags & Flag_HeightsNormals)
                        mDataTypes |= DATA_VHGT;
                    break;
                case fourCC("WNAM"):
                    esm.getHExact(mWnam.data(), mWnam.size());
                    if (mFlags & Flag_HeightsNormals)
                        mDataTypes |= DATA_WNAM;
                    break;
                case fourCC("VCLR"):
                    esm.skipHSub();
                    if (mFlags & Flag_Colors)
                        mDataTypes |= DATA_VCLR;
                    break;
                case fourCC("VTEX"):
                    esm.skipHSub();
                    if (mFlags & Flag_Textures)
                        mDataTypes |= DATA_VTEX;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void Land::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.startSubRecord("INTV");
        esm.writeT(mX);
        esm.writeT(mY);
        esm.endRecord("INTV");

        esm.writeHNT("DATA", mFlags);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        if (mLandData)
        {
            if (mDataTypes & Land::DATA_VNML)
            {
                esm.writeHNT("VNML", mLandData->mNormals);
            }
            if (mDataTypes & Land::DATA_VHGT)
            {
                VHGT offsets;
                offsets.mHeightOffset = mLandData->mHeights[0] / HEIGHT_SCALE;

                float prevY = mLandData->mHeights[0];
                size_t number = 0; // avoid multiplication
                for (unsigned i = 0; i < LandRecordData::sLandSize; ++i)
                {
                    float diff = (mLandData->mHeights[number] - prevY) / HEIGHT_SCALE;
                    offsets.mHeightData[number]
                        = diff >= 0 ? static_cast<std::int8_t>(diff + 0.5) : static_cast<std::int8_t>(diff - 0.5);

                    float prevX = prevY = mLandData->mHeights[number];
                    ++number;

                    for (unsigned j = 1; j < LandRecordData::sLandSize; ++j)
                    {
                        diff = (mLandData->mHeights[number] - prevX) / HEIGHT_SCALE;
                        offsets.mHeightData[number]
                            = diff >= 0 ? static_cast<std::int8_t>(diff + 0.5) : static_cast<std::int8_t>(diff - 0.5);

                        prevX = mLandData->mHeights[number];
                        ++number;
                    }
                }
                esm.writeNamedComposite("VHGT", offsets);
            }
            if (mDataTypes & Land::DATA_WNAM)
            {
                // Generate WNAM record
                std::int8_t wnam[LAND_GLOBAL_MAP_LOD_SIZE];
                constexpr float max = std::numeric_limits<std::int8_t>::max();
                constexpr float min = std::numeric_limits<std::int8_t>::min();
                constexpr float vertMult
                    = static_cast<float>(LandRecordData::sLandSize - 1) / LAND_GLOBAL_MAP_LOD_SIZE_SQRT;
                for (unsigned row = 0; row < LAND_GLOBAL_MAP_LOD_SIZE_SQRT; ++row)
                {
                    for (unsigned col = 0; col < LAND_GLOBAL_MAP_LOD_SIZE_SQRT; ++col)
                    {
                        float height
                            = mLandData->mHeights[static_cast<size_t>(row * vertMult) * LandRecordData::sLandSize
                                + static_cast<size_t>(col * vertMult)];
                        height /= height > 0 ? 128.f : 16.f;
                        height = std::clamp(height, min, max);
                        wnam[row * LAND_GLOBAL_MAP_LOD_SIZE_SQRT + col] = static_cast<std::int8_t>(height);
                    }
                }
                esm.writeHNT("WNAM", wnam);
            }
            if (mDataTypes & Land::DATA_VCLR)
            {
                esm.writeHNT("VCLR", mLandData->mColours);
            }
            if (mDataTypes & Land::DATA_VTEX)
            {
                uint16_t vtex[LandRecordData::sLandNumTextures];
                transposeTextureData(mLandData->mTextures.data(), vtex);
                esm.writeHNT("VTEX", vtex);
            }
        }
    }

    void Land::blank()
    {
        setPlugin(0);

        mWnam.fill(0);

        if (mLandData == nullptr)
            mLandData = std::make_unique<LandData>();

        mLandData->mHeightOffset = 0;
        std::fill(std::begin(mLandData->mHeights), std::end(mLandData->mHeights), DEFAULT_HEIGHT);
        mLandData->mMinHeight = 0;
        mLandData->mMaxHeight = 0;
        for (size_t i = 0; i < LandRecordData::sLandNumVerts; ++i)
        {
            mLandData->mNormals[i * 3 + 0] = 0;
            mLandData->mNormals[i * 3 + 1] = 0;
            mLandData->mNormals[i * 3 + 2] = 127;
        }
        mLandData->mTextures.fill(0);
        mLandData->mColours.fill(255);
        mLandData->mDataLoaded
            = Land::DATA_VNML | Land::DATA_VHGT | Land::DATA_WNAM | Land::DATA_VCLR | Land::DATA_VTEX;
        mDataTypes = mLandData->mDataLoaded;

        // No file associated with the land now
        mContext.filename.clear();
    }

    void Land::loadData(int flags) const
    {
        if (mLandData == nullptr)
            mLandData = std::make_unique<LandData>();

        loadData(flags, *mLandData);
    }

    void Land::loadData(int flags, LandData& data) const
    {
        // Try to load only available data
        flags = flags & mDataTypes;
        // Return if all required data is loaded
        if ((data.mDataLoaded & flags) == flags)
        {
            return;
        }

        if (mContext.filename.empty())
        {
            // Make sure there is data, and that it doesn't point to the same object.
            if (mLandData != nullptr && mLandData.get() != &data)
                data = *mLandData;

            return;
        }

        ESMReader reader;
        reader.restoreContext(mContext);

        if (reader.isNextSub("VNML"))
        {
            condLoad(reader, flags, data.mDataLoaded, DATA_VNML, data.mNormals);
        }

        if (reader.isNextSub("VHGT"))
        {
            VHGT vhgt;
            if (condLoad(reader, flags, data.mDataLoaded, DATA_VHGT, vhgt))
            {
                data.mMinHeight = std::numeric_limits<float>::max();
                data.mMaxHeight = -std::numeric_limits<float>::max();
                float rowOffset = vhgt.mHeightOffset;
                for (unsigned y = 0; y < LandRecordData::sLandSize; y++)
                {
                    rowOffset += vhgt.mHeightData[y * LandRecordData::sLandSize];

                    data.mHeights[y * LandRecordData::sLandSize] = rowOffset * HEIGHT_SCALE;
                    if (rowOffset * HEIGHT_SCALE > data.mMaxHeight)
                        data.mMaxHeight = rowOffset * HEIGHT_SCALE;
                    if (rowOffset * HEIGHT_SCALE < data.mMinHeight)
                        data.mMinHeight = rowOffset * HEIGHT_SCALE;

                    float colOffset = rowOffset;
                    for (unsigned x = 1; x < LandRecordData::sLandSize; x++)
                    {
                        colOffset += vhgt.mHeightData[y * LandRecordData::sLandSize + x];
                        data.mHeights[x + y * LandRecordData::sLandSize] = colOffset * HEIGHT_SCALE;

                        if (colOffset * HEIGHT_SCALE > data.mMaxHeight)
                            data.mMaxHeight = colOffset * HEIGHT_SCALE;
                        if (colOffset * HEIGHT_SCALE < data.mMinHeight)
                            data.mMinHeight = colOffset * HEIGHT_SCALE;
                    }
                }
            }
        }

        if (reader.isNextSub("WNAM"))
            reader.skipHSub();

        if (reader.isNextSub("VCLR"))
            condLoad(reader, flags, data.mDataLoaded, DATA_VCLR, data.mColours);
        if (reader.isNextSub("VTEX"))
        {
            uint16_t vtex[LandRecordData::sLandNumTextures];
            if (condLoad(reader, flags, data.mDataLoaded, DATA_VTEX, vtex))
            {
                transposeTextureData(vtex, data.mTextures.data());
            }
        }
    }

    void Land::unloadData()
    {
        mLandData = nullptr;
    }

    bool Land::isDataLoaded(int flags) const
    {
        return mLandData && (mLandData->mDataLoaded & flags) == flags;
    }

    Land::Land(const Land& land)
        : mFlags(land.mFlags)
        , mX(land.mX)
        , mY(land.mY)
        , mContext(land.mContext)
        , mDataTypes(land.mDataTypes)
        , mWnam(land.mWnam)
        , mLandData(land.mLandData != nullptr ? std::make_unique<LandData>(*land.mLandData) : nullptr)
    {
    }

    Land& Land::operator=(const Land& land)
    {
        Land copy(land);
        *this = std::move(copy);
        return *this;
    }

    const Land::LandData* Land::getLandData(int flags) const
    {
        if (!(flags & mDataTypes))
            return nullptr;

        loadData(flags);
        return mLandData.get();
    }

    void Land::add(int flags)
    {
        if (mLandData == nullptr)
            mLandData = std::make_unique<LandData>();

        mDataTypes |= flags;
        mLandData->mDataLoaded |= flags;
    }
}
