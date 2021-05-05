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
        esm.getHNT(m.size, "DATA");
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
            esm.getExact(mSCRD.data(), mSCRD.size());
    }
    if (esm.isNextSub("SCRS"))
    {
        esm.getSubHeader();
        mSCRS.resize(esm.getSubSize());
        if (!mSCRS.empty())
            esm.getExact(mSCRS.data(), mSCRS.size());
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

    for (const Header::MasterData& data : mMaster)
    {
        esm.writeHNCString ("MAST", data.name);
        esm.writeHNT ("DATA", data.size);
    }
}
