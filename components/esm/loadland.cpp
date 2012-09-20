#include "loadland.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

void Land::LandData::save(ESMWriter &esm)
{
    // TODO: Make this actually work.

    if (mDataTypes & Land::DATA_VNML) {
        esm.writeHNT("VNML", mNormals, sizeof(VNML));
    }

    if (mDataTypes & Land::DATA_VHGT) {
    VHGT offsets;
    offsets.mHeightOffset = mHeights[0] / HEIGHT_SCALE;
    offsets.mUnknown1 = mUnk1;
    offsets.mUnknown2 = mUnk2;
    
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
    //esm.writeHNT("WNAM", 0, 81);
/*    esm.startSubRecord("WNAM");
    for (int i = 0; i < 81; i++)
        esm.writeT((char)0x80, 1);
    esm.endRecord("WNAM");
*/
    if (mDataTypes & Land::DATA_WNAM) {
        esm.writeHNT("WNAM", mWnam, 81);
    }
    
    if (mDataTypes & Land::DATA_VCLR)
        esm.writeHNT("VCLR", mColours, 3*LAND_NUM_VERTS);
    if (mDataTypes & Land::DATA_VTEX) {
        uint16_t vtex[256];

        int readPos = 0; //bit ugly, but it works
        for ( int y1 = 0; y1 < 4; y1++ )
            for ( int x1 = 0; x1 < 4; x1++ )
                for ( int y2 = 0; y2 < 4; y2++)
                    for ( int x2 = 0; x2 < 4; x2++ )
                        vtex[(y1*4+y2)*16+(x1*4+x2)] = mTextures[readPos++];

        esm.writeHNT("VTEX", vtex, 512);
    }
}

Land::Land()
    : mFlags(0)
    , mX(0)
    , mY(0)
    , mEsm(NULL)
//    , hasData(false)
    , mDataTypes(0)
    , mDataLoaded(false)
    , mLandData(NULL)
{
}

Land::~Land()
{
    delete mLandData;
}

void Land::load(ESMReader &esm)
{
    mEsm = &esm;

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
    mHasData = mDataTypes & (DATA_VNML|DATA_VHGT|DATA_WNAM|DATA_VTEX|DATA_VCLR);

    mDataLoaded = false;
    mLandData = NULL;
}

void Land::save(ESMWriter &esm)
{
    esm.startSubRecord("INTV");
    esm.writeT(mX);
    esm.writeT(mY);
    esm.endRecord("INTV");

    esm.writeHNT("DATA", mFlags);

    // TODO: Land!
    bool wasLoaded = mDataLoaded;
    if (mHasData)
        loadData(); // I think it might be a good idea to have 
                    // the data loaded before trying to save it
                    
    if (mDataLoaded)
        mLandData->save(esm);

    if (!wasLoaded)
        unloadData(); // Don't need to keep the data loaded if it wasn't already
}

void Land::loadData()
{
    if (mDataLoaded)
    {
        return;
    }

    mLandData = new LandData;

    if (mHasData)
    {
        mEsm->restoreContext(mContext);

        memset(mLandData->mNormals, 0, LAND_NUM_VERTS * 3);
        
        if (mEsm->isNextSub("VNML")) {
            mEsm->getHExact(mLandData->mNormals, sizeof(VNML));
        }/*
        if (mEsm->isNextSub("VNML"))
        {
            mEsm->skipHSubSize(12675);
        } */

        if (mEsm->isNextSub("VHGT")) {
        VHGT rawHeights;

        mEsm->getHExact(&rawHeights, sizeof(VHGT));
        float currentHeightOffset = rawHeights.mHeightOffset;
        for (int y = 0; y < LAND_SIZE; y++)
        {
            currentHeightOffset += rawHeights.mHeightData[y * LAND_SIZE];
            mLandData->mHeights[y * LAND_SIZE] = currentHeightOffset * HEIGHT_SCALE;

            float tempOffset = currentHeightOffset;
            for (int x = 1; x < LAND_SIZE; x++)
            {
                tempOffset += rawHeights.mHeightData[y * LAND_SIZE + x];
                mLandData->mHeights[x + y * LAND_SIZE] = tempOffset * HEIGHT_SCALE;
            }
        }
        mLandData->mUnk1 = rawHeights.mUnknown1;
        mLandData->mUnk2 = rawHeights.mUnknown2;
        }

        if (mEsm->isNextSub("WNAM"))
        {
            mEsm->getHExact(mLandData->mWnam, 81);
        }
        if (mEsm->isNextSub("VCLR"))
        {
            mLandData->mUsingColours = true;
            mEsm->getHExact(&mLandData->mColours, 3*LAND_NUM_VERTS);
        }else{
            mLandData->mUsingColours = false;
        }
        if (mEsm->isNextSub("VTEX"))
        {
            //TODO fix magic numbers
            uint16_t vtex[256];
            mEsm->getHExact(&vtex, 512);

            int readPos = 0; //bit ugly, but it works
            for ( int y1 = 0; y1 < 4; y1++ )
                for ( int x1 = 0; x1 < 4; x1++ )
                    for ( int y2 = 0; y2 < 4; y2++)
                        for ( int x2 = 0; x2 < 4; x2++ )
                            mLandData->mTextures[(y1*4+y2)*16+(x1*4+x2)] = vtex[readPos++];
        }
    }
    else
    {
        mLandData->mUsingColours = false;
        memset(mLandData->mTextures, 0, sizeof(mLandData->mTextures));
        for (int i = 0; i < LAND_NUM_VERTS; i++)
        {
            mLandData->mHeights[i] = -256.0f * HEIGHT_SCALE;
        }
    }

    mLandData->mDataTypes = mDataTypes;
    mDataLoaded = true;
}

void Land::unloadData()
{
    if (mDataLoaded)
    {
        delete mLandData;
        mLandData = NULL;
        mDataLoaded = false;
    }
}

}
