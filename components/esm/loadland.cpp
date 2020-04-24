#include "loadland.hpp"

#include <limits>
#include <utility>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Land::sRecordId = REC_LAND;

    Land::Land()
        : mFlags(0)
        , mX(0)
        , mY(0)
        , mPlugin(0)
        , mDataTypes(0)
        , mLandData(nullptr)
    {
    }

    void transposeTextureData(const uint16_t *in, uint16_t *out)
    {
        int readPos = 0; //bit ugly, but it works
        for ( int y1 = 0; y1 < 4; y1++ )
            for ( int x1 = 0; x1 < 4; x1++ )
                for ( int y2 = 0; y2 < 4; y2++)
                    for ( int x2 = 0; x2 < 4; x2++ )
                        out[(y1*4+y2)*16+(x1*4+x2)] = in[readPos++];
    }

    Land::~Land()
    {
        delete mLandData;
    }

    void Land::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPlugin = esm.getIndex();

        bool hasLocation = false;
        bool isLoaded = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'I','N','T','V'>::value:
                    esm.getSubHeaderIs(8);
                    esm.getT<int>(mX);
                    esm.getT<int>(mY);
                    hasLocation = true;
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mFlags);
                    break;
                case ESM::SREC_DELE:
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
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'V','N','M','L'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VNML;
                    break;
                case ESM::FourCC<'V','H','G','T'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VHGT;
                    break;
                case ESM::FourCC<'W','N','A','M'>::value:
                    esm.getHExact(mWnam, sizeof(mWnam));
                    mDataTypes |= DATA_WNAM;
                    break;
                case ESM::FourCC<'V','C','L','R'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VCLR;
                    break;
                case ESM::FourCC<'V','T','E','X'>::value:
                    esm.skipHSub();
                    mDataTypes |= DATA_VTEX;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void Land::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.startSubRecord("INTV");
        esm.writeT(mX);
        esm.writeT(mY);
        esm.endRecord("INTV");

        esm.writeHNT("DATA", mFlags);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        if (mLandData)
        {
            if (mDataTypes & Land::DATA_VNML) {
                esm.writeHNT("VNML", mLandData->mNormals);
            }
            if (mDataTypes & Land::DATA_VHGT) {
                VHGT offsets;
                offsets.mHeightOffset = mLandData->mHeights[0] / HEIGHT_SCALE;
                offsets.mUnk1 = mLandData->mUnk1;
                offsets.mUnk2 = mLandData->mUnk2;

                float prevY = mLandData->mHeights[0];
                int number = 0; // avoid multiplication
                for (int i = 0; i < LAND_SIZE; ++i) {
                    float diff = (mLandData->mHeights[number] - prevY) / HEIGHT_SCALE;
                    offsets.mHeightData[number] =
                        (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

                    float prevX = prevY = mLandData->mHeights[number];
                    ++number;

                    for (int j = 1; j < LAND_SIZE; ++j) {
                        diff = (mLandData->mHeights[number] - prevX) / HEIGHT_SCALE;
                        offsets.mHeightData[number] =
                            (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

                        prevX = mLandData->mHeights[number];
                        ++number;
                    }
                }
                esm.writeHNT("VHGT", offsets, sizeof(VHGT));
            }
            if (mDataTypes & Land::DATA_WNAM)
            {
                // Generate WNAM record
                signed char wnam[LAND_GLOBAL_MAP_LOD_SIZE];
                float max = std::numeric_limits<signed char>::max();
                float min = std::numeric_limits<signed char>::min();
                float vertMult = static_cast<float>(ESM::Land::LAND_SIZE - 1) / LAND_GLOBAL_MAP_LOD_SIZE_SQRT;
                for (int row = 0; row < LAND_GLOBAL_MAP_LOD_SIZE_SQRT; ++row)
                {
                    for (int col = 0; col < LAND_GLOBAL_MAP_LOD_SIZE_SQRT; ++col)
                    {
                        float height = mLandData->mHeights[int(row * vertMult) * ESM::Land::LAND_SIZE + int(col * vertMult)];
                        height /= height > 0 ? 128.f : 16.f;
                        height = std::min(max, std::max(min, height));
                        wnam[row * LAND_GLOBAL_MAP_LOD_SIZE_SQRT + col] = static_cast<signed char>(height);
                    }
                }
                esm.writeHNT("WNAM", wnam);
            }
            if (mDataTypes & Land::DATA_VCLR) {
                esm.writeHNT("VCLR", mLandData->mColours);
            }
            if (mDataTypes & Land::DATA_VTEX) {
                uint16_t vtex[LAND_NUM_TEXTURES];
                transposeTextureData(mLandData->mTextures, vtex);
                esm.writeHNT("VTEX", vtex);
            }
        }

    }

    void Land::blank()
    {
        mPlugin = 0;

        std::fill(std::begin(mWnam), std::end(mWnam), 0);

        if (!mLandData)
            mLandData = new LandData;

        mLandData->mHeightOffset = 0;
        std::fill(std::begin(mLandData->mHeights), std::end(mLandData->mHeights), 0);
        mLandData->mMinHeight = 0;
        mLandData->mMaxHeight = 0;
        for (int i = 0; i < LAND_NUM_VERTS; ++i)
        {
            mLandData->mNormals[i*3+0] = 0;
            mLandData->mNormals[i*3+1] = 0;
            mLandData->mNormals[i*3+2] = 127;
        }
        std::fill(std::begin(mLandData->mTextures), std::end(mLandData->mTextures), 0);
        std::fill(std::begin(mLandData->mColours), std::end(mLandData->mColours), 255);
        mLandData->mUnk1 = 0;
        mLandData->mUnk2 = 0;
        mLandData->mDataLoaded = Land::DATA_VNML | Land::DATA_VHGT | Land::DATA_WNAM |
            Land::DATA_VCLR | Land::DATA_VTEX;
        mDataTypes = mLandData->mDataLoaded;

        // No file associated with the land now
        mContext.filename.clear();
    }

    void Land::loadData(int flags, LandData* target) const
    {
        // Create storage if nothing is loaded
        if (!target && !mLandData)
        {
            mLandData = new LandData;
        }

        if (!target)
            target = mLandData;

        // Try to load only available data
        flags = flags & mDataTypes;
        // Return if all required data is loaded
        if ((target->mDataLoaded & flags) == flags) {
            return;
        }

        // Copy data to target if no file
        if (mContext.filename.empty())
        {
            // Make sure there is data, and that it doesn't point to the same object.
            if (mLandData && mLandData != target)
                *target = *mLandData;

            return;
        }

        ESM::ESMReader reader;
        reader.restoreContext(mContext);

        if (reader.isNextSub("VNML")) {
            condLoad(reader, flags, target->mDataLoaded, DATA_VNML, target->mNormals, sizeof(target->mNormals));
        }

        if (reader.isNextSub("VHGT")) {
            VHGT vhgt;
            if (condLoad(reader, flags, target->mDataLoaded, DATA_VHGT, &vhgt, sizeof(vhgt))) {
                target->mMinHeight = std::numeric_limits<float>::max();
                target->mMaxHeight = -std::numeric_limits<float>::max();
                float rowOffset = vhgt.mHeightOffset;
                for (int y = 0; y < LAND_SIZE; y++) {
                    rowOffset += vhgt.mHeightData[y * LAND_SIZE];

                    target->mHeights[y * LAND_SIZE] = rowOffset * HEIGHT_SCALE;
                    if (rowOffset * HEIGHT_SCALE > target->mMaxHeight)
                        target->mMaxHeight = rowOffset * HEIGHT_SCALE;
                    if (rowOffset * HEIGHT_SCALE < target->mMinHeight)
                        target->mMinHeight = rowOffset * HEIGHT_SCALE;

                    float colOffset = rowOffset;
                    for (int x = 1; x < LAND_SIZE; x++) {
                        colOffset += vhgt.mHeightData[y * LAND_SIZE + x];
                        target->mHeights[x + y * LAND_SIZE] = colOffset * HEIGHT_SCALE;

                        if (colOffset * HEIGHT_SCALE > target->mMaxHeight)
                            target->mMaxHeight = colOffset * HEIGHT_SCALE;
                        if (colOffset * HEIGHT_SCALE < target->mMinHeight)
                            target->mMinHeight = colOffset * HEIGHT_SCALE;
                    }
                }
                target->mUnk1 = vhgt.mUnk1;
                target->mUnk2 = vhgt.mUnk2;
            }
        }

        if (reader.isNextSub("WNAM"))
            reader.skipHSub();

        if (reader.isNextSub("VCLR"))
            condLoad(reader, flags, target->mDataLoaded, DATA_VCLR, target->mColours, 3 * LAND_NUM_VERTS);
        if (reader.isNextSub("VTEX")) {
            uint16_t vtex[LAND_NUM_TEXTURES];
            if (condLoad(reader, flags, target->mDataLoaded, DATA_VTEX, vtex, sizeof(vtex))) {
                transposeTextureData(vtex, target->mTextures);
            }
        }
    }

    void Land::unloadData() const
    {
        if (mLandData)
        {
            delete mLandData;
            mLandData = nullptr;
        }
    }

    bool Land::condLoad(ESM::ESMReader& reader, int flags, int& targetFlags, int dataFlag, void *ptr, unsigned int size) const
    {
        if ((targetFlags & dataFlag) == 0 && (flags & dataFlag) != 0) {
            reader.getHExact(ptr, size);
            targetFlags |= dataFlag;
            return true;
        }
        reader.skipHSubSize(size);
        return false;
    }

    bool Land::isDataLoaded(int flags) const
    {
        return mLandData && (mLandData->mDataLoaded & flags) == flags;
    }

    Land::Land (const Land& land)
    : mFlags (land.mFlags), mX (land.mX), mY (land.mY), mPlugin (land.mPlugin),
      mContext (land.mContext), mDataTypes (land.mDataTypes),
      mLandData (land.mLandData ? new LandData (*land.mLandData) : nullptr)
    {
        std::copy(land.mWnam, land.mWnam + LAND_GLOBAL_MAP_LOD_SIZE, mWnam);
    }

    Land& Land::operator= (Land land)
    {
        swap (land);
        return *this;
    }

    void Land::swap (Land& land)
    {
        std::swap (mFlags, land.mFlags);
        std::swap (mX, land.mX);
        std::swap (mY, land.mY);
        std::swap (mPlugin, land.mPlugin);
        std::swap (mContext, land.mContext);
        std::swap (mDataTypes, land.mDataTypes);
        std::swap (mLandData, land.mLandData);
        std::swap (mWnam, land.mWnam);
    }

    const Land::LandData *Land::getLandData (int flags) const
    {
        if (!(flags & mDataTypes))
            return nullptr;

        loadData (flags);
        return mLandData;
    }

    const Land::LandData *Land::getLandData() const
    {
        return mLandData;
    }

    Land::LandData *Land::getLandData()
    {
        return mLandData;
    }

    void Land::add (int flags)
    {
        if (!mLandData)
            mLandData = new LandData;

        mDataTypes |= flags;
        mLandData->mDataLoaded |= flags;
    }

    void Land::remove (int flags)
    {
        mDataTypes &= ~flags;

        if (mLandData)
        {
            mLandData->mDataLoaded &= ~flags;

            if (!mLandData->mDataLoaded)
            {
                delete mLandData;
                mLandData = nullptr;
            }
        }
    }
}
