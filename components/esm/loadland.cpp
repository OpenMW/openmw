#include "loadland.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Land::sRecordId = REC_LAND;

void Land::LandData::save(ESMWriter &esm)
{
    if (mDataTypes & Land::DATA_VNML) {
        esm.writeHNT("VNML", mNormals, sizeof(VNML));
    }
    if (mDataTypes & Land::DATA_VHGT) {
        VHGT offsets;
        offsets.mHeightOffset = mHeights[0] / HEIGHT_SCALE;
        offsets.mUnk1 = mUnk1;
        offsets.mUnk2 = mUnk2;

        float prevY = mHeights[0], prevX;
        int number = 0; // avoid multiplication
        for (int i = 0; i < LAND_SIZE; ++i) {
            float diff = (mHeights[number] - prevY) / HEIGHT_SCALE;
            offsets.mHeightData[number] =
                (diff >= 0) ? (int8_t) (diff + 0.5) : (int8_t) (diff - 0.5);

            prevX = prevY = mHeights[number];
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

void Land::LandData::transposeTextureData(uint16_t *in, uint16_t *out)
{
    int readPos = 0; //bit ugly, but it works
    for ( int y1 = 0; y1 < 4; y1++ )
        for ( int x1 = 0; x1 < 4; x1++ )
            for ( int y2 = 0; y2 < 4; y2++)
                for ( int x2 = 0; x2 < 4; x2++ )
                    out[(y1*4+y2)*16+(x1*4+x2)] = in[readPos++];
}

Land::Land()
    : mFlags(0)
    , mX(0)
    , mY(0)
    , mEsm(NULL)
    , mDataTypes(0)
    , mDataLoaded(false)
    , mLandData(NULL)
    , mPlugin(0)
    , mHasData(false)
{
}

Land::~Land()
{
    delete mLandData;
}

void Land::load(ESMReader &esm)
{
    mEsm = &esm;
    mPlugin = mEsm->getIndex();

    // Get the grid location
    esm.getSubNameIs("INTV");
    esm.getSubHeaderIs(8);
    esm.getT<int>(mX);
    esm.getT<int>(mY);

    esm.getHNT(mFlags, "DATA");

    // Store the file position
    mContext = esm.getContext();

    mHasData = false;

    // Skip these here. Load the actual data when the cell is loaded.
    if (esm.isNextSub("VNML"))
    {
        esm.skipHSubSize(12675);
        mDataTypes |= DATA_VNML;
    }
    if (esm.isNextSub("VHGT"))
    {
        esm.skipHSubSize(4232);
        mDataTypes |= DATA_VHGT;
    }
    if (esm.isNextSub("WNAM"))
    {
        esm.skipHSubSize(81);
        mDataTypes |= DATA_WNAM;
    }
    if (esm.isNextSub("VCLR"))
    {
        esm.skipHSubSize(12675);
        mDataTypes |= DATA_VCLR;
    }
    if (esm.isNextSub("VTEX"))
    {
        esm.skipHSubSize(512);
        mDataTypes |= DATA_VTEX;
    }

    // We need all three of VNML, VHGT and VTEX in order to use the
    // landscape. (Though Morrowind seems to accept terrain without VTEX/VCLR entries)
    mHasData = mDataTypes & (DATA_VNML|DATA_VHGT|DATA_WNAM);

    mDataLoaded = 0;
    mLandData = NULL;
}

void Land::save(ESMWriter &esm) const
{
    esm.startSubRecord("INTV");
    esm.writeT(mX);
    esm.writeT(mY);
    esm.endRecord("INTV");

    esm.writeHNT("DATA", mFlags);
}

/// \todo remove memory allocation when only defaults needed
void Land::loadData(int flags)
{
    // Try to load only available data
    int actual = flags & mDataTypes;
    // Return if all required data is loaded
    if (flags == 0 || (actual != 0 && (mDataLoaded & actual) == actual)) {
        return;
    }
    // Create storage if nothing is loaded
    if (mLandData == NULL) {
        mLandData = new LandData;
        mLandData->mDataTypes = mDataTypes;
    }
    mEsm->restoreContext(mContext);

    memset(mLandData->mNormals, 0, sizeof(mLandData->mNormals));

    if (mEsm->isNextSub("VNML")) {
        condLoad(actual, DATA_VNML, mLandData->mNormals, sizeof(mLandData->mNormals));
    }

    if (mEsm->isNextSub("VHGT")) {
        static VHGT vhgt;
        if (condLoad(actual, DATA_VHGT, &vhgt, sizeof(vhgt))) {
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
    } else if ((flags & DATA_VHGT) && (mDataLoaded & DATA_VHGT) == 0) {
        for (int i = 0; i < LAND_NUM_VERTS; ++i) {
            mLandData->mHeights[i] = -256.0f * HEIGHT_SCALE;
        }
        mDataLoaded |= DATA_VHGT;
    }

    if (mEsm->isNextSub("WNAM")) {
        condLoad(actual, DATA_WNAM, mLandData->mWnam, 81);
    }
    if (mEsm->isNextSub("VCLR")) {
        mLandData->mUsingColours = true;
        condLoad(actual, DATA_VCLR, mLandData->mColours, 3 * LAND_NUM_VERTS);
    } else {
        mLandData->mUsingColours = false;
    }
    if (mEsm->isNextSub("VTEX")) {
        static uint16_t vtex[LAND_NUM_TEXTURES];
        if (condLoad(actual, DATA_VTEX, vtex, sizeof(vtex))) {
            LandData::transposeTextureData(vtex, mLandData->mTextures);
        }
    } else if ((flags & DATA_VTEX) && (mDataLoaded & DATA_VTEX) == 0) {
        memset(mLandData->mTextures, 0, sizeof(mLandData->mTextures));
        mDataLoaded |= DATA_VTEX;
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

bool Land::condLoad(int flags, int dataFlag, void *ptr, unsigned int size)
{
    if ((mDataLoaded & dataFlag) == 0 && (flags & dataFlag) != 0) {
        mEsm->getHExact(ptr, size);
        mDataLoaded |= dataFlag;
        return true;
    }
    mEsm->skipHSubSize(size);
    return false;
}

}
