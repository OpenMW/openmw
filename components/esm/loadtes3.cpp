
#include "loadtes3.hpp"

#include "esmcommon.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

void ESM::Header::blank()
{
    mData.version = ESM::VER_13;
    mData.type = 0;
    mData.author.assign ("");
    mData.desc.assign ("");
    mData.records = 0;
    mFormat = CurrentFormat;
    mMaster.clear();
}

void ESM::Header::load (ESMReader &esm)
{
    if (esm.isNextSub("HEDR"))
    {
      esm.getSubHeader();
      esm.getT(mData.version);
      esm.getT(mData.type);
      mData.author.assign(esm.getString(sizeof(mData.author.name)));
      mData.desc.assign(esm.getString(sizeof(mData.desc.name)));
      esm.getT(mData.records);
    }

    if (esm.isNextSub ("FORM"))
    {
        esm.getHT (mFormat);
        if (mFormat<0)
            esm.fail ("invalid format code");
    }
    else
        mFormat = 0;

    while (esm.isNextSub ("MAST"))
    {
        MasterData m;
        m.name = esm.getHString();
        m.size = esm.getHNLong ("DATA");
        mMaster.push_back (m);
    }
}

void ESM::Header::save (ESMWriter &esm)
{
    esm.writeHNT ("HEDR", mData, 300);

    if (mFormat>0)
        esm.writeHNT ("FORM", mFormat);

    for (std::vector<Header::MasterData>::iterator iter = mMaster.begin();
         iter != mMaster.end(); ++iter)
    {
        esm.writeHNCString ("MAST", iter->name);
        esm.writeHNT ("DATA", iter->size);
    }
}
