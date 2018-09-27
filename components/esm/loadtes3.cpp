#include "loadtes3.hpp"

#include "esmcommon.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

void ESM::Header::blank()
{
    mData.version = ESM::VER_13;
    mData.type = 0;
    mData.author.clear();
    mData.desc.clear();
    mData.records = 0;
    mFormat = CurrentFormat;
    mMaster.clear();
}

void ESM::Header::load (ESMReader &esm)
{
    if (esm.isNextSub ("FORM"))
    {
        esm.getHT (mFormat);
        if (mFormat<0)
            esm.fail ("invalid format code");
    }
    else
        mFormat = 0;

    if (esm.isNextSub("HEDR"))
    {
      esm.getSubHeader();
      esm.getT(mData.version);
      esm.getT(mData.type);
      mData.author.assign( esm.getString(32) );
      mData.desc.assign( esm.getString(256) );
      esm.getT(mData.records);
    }

    while (esm.isNextSub ("MAST"))
    {
        MasterData m;
        m.name = esm.getHString();
        m.size = esm.getHNLong ("DATA");
        mMaster.push_back (m);
    }

    if (esm.isNextSub("GMDT"))
    {
        esm.getHT(mGameData);
    }
    if (esm.isNextSub("SCRD"))
    {
        esm.getSubHeader();
        mSCRD.resize(esm.getSubSize());
        if (!mSCRD.empty())
            esm.getExact(&mSCRD[0], mSCRD.size());
    }
    if (esm.isNextSub("SCRS"))
    {
        esm.getSubHeader();
        mSCRS.resize(esm.getSubSize());
        if (!mSCRS.empty())
            esm.getExact(&mSCRS[0], mSCRS.size());
    }
}

void ESM::Header::save (ESMWriter &esm)
{
    if (mFormat>0)
        esm.writeHNT ("FORM", mFormat);

    esm.startSubRecord("HEDR");
    esm.writeT(mData.version);
    esm.writeT(mData.type);
    esm.writeFixedSizeString(mData.author, 32);
    esm.writeFixedSizeString(mData.desc, 256);
    esm.writeT(mData.records);
    esm.endRecord("HEDR");

    for (std::vector<Header::MasterData>::iterator iter = mMaster.begin();
         iter != mMaster.end(); ++iter)
    {
        esm.writeHNCString ("MAST", iter->name);
        esm.writeHNT ("DATA", iter->size);
    }
}
