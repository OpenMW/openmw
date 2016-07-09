#include "loadland.hpp"

#include <utility>

#include <OpenThreads/ScopedLock>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Land::sRecordId = REC_LAND;

    void Land::LandData::save(ESMWriter &esm) const
    {
        if (mDataTypes & Land::DATA_VNML) {
            esm.writeHNT("VNML", mNormals, sizeof(mNormals));
        }
        if (mDataTypes & Land::DATA_VHGT) {
            VHGT offsets;
            offsets.mHeightOffset = mHeights[0] / HEIGHT_SCALE;
            offsets.mUnk1 = mUnk1;
            offsets.mUnk2 = mUnk2;

            float prevY = mHeights[0];
            int number = 0; // avoid multiplication
            for (int i = 0; i < LAND_SIZE; ++i) {
                float diff = (mHeights[number] - prevY) / HEIGHT_SCALE;
                offsets.mHeightData[number] =
                    (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

                float prevX = prevY = mHeights[number];
                ++number;

                for (int j = 1; j < LAND_SIZE; ++j) {
                    diff = (mHeights[number] - prevX) / HEIGHT_SCALE;
                    offsets.mHeightData[number] =
                        (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

                    prevX = mHeights[number];
                    ++number;
                }
            }
            esm.writeHNT("VHGT", offsets, sizeof(VHGT));
        }
        if (mDataTypes & Land::DATA_WNAM) {
            esm.writeHNT("WNAM", mWnam, 81);
        }
        if (mDataTypes & Land::DATA_VCLR) {
            esm.writeHNT("VCLR", mColours, 3*LAND_NUM_VERTS);
        }
        if (mDataTypes & Land::DATA_VTEX) {
            static uint16_t vtex[LAND_NUM_TEXTURES];
            transposeTextureData(mTextures, vtex);
            esm.writeHNT("VTEX", vtex, sizeof(vtex));
        }
    }

    Land::Land()
        : mFlags(0)
        , mX(0)
        , mY(0)
        , mPlugin(0)
        , mDataTypes(0)
        , mDataLoaded(false)
        , mLandData(NULL)
    {
    }

    void Land::LandData::transposeTextureData(const uint16_t *in, uint16_t *out)
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
                    esm.skipHSub();
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

        mDataLoaded = 0;
        mLandData = NULL;
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
            mLandData->save(esm);
        }
    }

    void Land::loadData(int flags) const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

        // Try to load only available data
        flags = flags & mDataTypes;
        // Return if all required data is loaded
        if ((mDataLoaded & flags) == flags) {
            return;
        }
        // Create storage if nothing is loaded
        if (mLandData == NULL) {
            mLandData = new LandData;
            mLandData->mDataTypes = mDataTypes;
        }

        ESM::ESMReader reader;
        reader.restoreContext(mContext);

        if (reader.isNextSub("VNML")) {
            condLoad(reader, flags, DATA_VNML, mLandData->mNormals, sizeof(mLandData->mNormals));
        }

        if (reader.isNextSub("VHGT")) {
            static VHGT vhgt;
            if (condLoad(reader, flags, DATA_VHGT, &vhgt, sizeof(vhgt))) {
                float rowOffset = vhgt.mHeightOffset;
                for (int y = 0; y < LAND_SIZE; y++) {
                    rowOffset += vhgt.mHeightData[y * LAND_SIZE];

                    mLandData->mHeights[y * LAND_SIZE] = rowOffset * HEIGHT_SCALE;

                    float colOffset = rowOffset;
                    for (int x = 1; x < LAND_SIZE; x++) {
                        colOffset += vhgt.mHeightData[y * LAND_SIZE + x];
                        mLandData->mHeights[x + y * LAND_SIZE] = colOffset * HEIGHT_SCALE;
                    }
                }
                mLandData->mUnk1 = vhgt.mUnk1;
                mLandData->mUnk2 = vhgt.mUnk2;
            }
        }

        if (reader.isNextSub("WNAM")) {
            condLoad(reader, flags, DATA_WNAM, mLandData->mWnam, 81);
        }
        if (reader.isNextSub("VCLR"))
            condLoad(reader, flags, DATA_VCLR, mLandData->mColours, 3 * LAND_NUM_VERTS);
        if (reader.isNextSub("VTEX")) {
            static uint16_t vtex[LAND_NUM_TEXTURES];
            if (condLoad(reader, flags, DATA_VTEX, vtex, sizeof(vtex))) {
                LandData::transposeTextureData(vtex, mLandData->mTextures);
            }
        }
    }

    void Land::unloadData()
    {
        if (mDataLoaded)
        {
            delete mLandData;
            mLandData = NULL;
            mDataLoaded = 0;
        }
    }

    bool Land::condLoad(ESM::ESMReader& reader, int flags, int dataFlag, void *ptr, unsigned int size) const
    {
        if ((mDataLoaded & dataFlag) == 0 && (flags & dataFlag) != 0) {
            reader.getHExact(ptr, size);
            mDataLoaded |= dataFlag;
            return true;
        }
        reader.skipHSubSize(size);
        return false;
    }

    bool Land::isDataLoaded(int flags) const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        return (mDataLoaded & flags) == (flags & mDataTypes);
    }

    Land::Land (const Land& land)
    : mFlags (land.mFlags), mX (land.mX), mY (land.mY), mPlugin (land.mPlugin),
      mContext (land.mContext), mDataTypes (land.mDataTypes),
      mDataLoaded (land.mDataLoaded),
      mLandData (land.mLandData ? new LandData (*land.mLandData) : 0)
    {}

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
        std::swap (mDataLoaded, land.mDataLoaded);
        std::swap (mLandData, land.mLandData);
    }

    const Land::LandData *Land::getLandData (int flags) const
    {
        if (!(flags & mDataTypes))
            return 0;

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
        mDataLoaded |= flags;
    }

    void Land::remove (int flags)
    {
        mDataTypes &= ~flags;
        mDataLoaded &= ~flags;

        if (!mDataLoaded)
        {
            delete mLandData;
            mLandData = 0;
        }
    }
}
