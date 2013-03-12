
#include "loadtes3.hpp"

#include "esmcommon.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::Header::blank()
{
    mData.version = ESM::VER_13;
    mData.type = 0;
    mData.author.assign ("");
    mData.desc.assign ("");
    mData.records = 0;
}

void ESM::Header::load (ESMReader &esm)
{
    esm.getHNT (mData, "HEDR", 300);

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

    for (std::vector<Header::MasterData>::iterator iter = mMaster.begin();
         iter != mMaster.end(); ++iter)
    {
        esm.writeHNCString ("MAST", iter->name);
        esm.writeHNT ("DATA", iter->size);
    }
}