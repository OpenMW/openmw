#include "loadtes3.hpp"

#include "components/esm/esmcommon.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void Header::blank()
    {
        mData.version.ui = VER_130;
        mData.type = 0;
        mData.author.clear();
        mData.desc.clear();
        mData.records = 0;
        mFormatVersion = CurrentContentFormatVersion;
        mMaster.clear();
    }

    void Header::load(ESMReader& esm)
    {
        mFormatVersion = DefaultFormatVersion;
        esm.getHNOT("FORM", mFormatVersion);

        if (esm.isNextSub("HEDR"))
        {
            esm.getSubHeader();
            esm.getT(mData.version.ui);
            esm.getT(mData.type);
            mData.author = esm.getMaybeFixedStringSize(32);
            mData.desc = esm.getMaybeFixedStringSize(256);
            esm.getT(mData.records);
        }

        while (esm.isNextSub("MAST"))
        {
            MasterData m;
            m.name = esm.getHString();
            esm.getHNT(m.size, "DATA");
            mMaster.push_back(std::move(m));
        }

        esm.getHNOT("GMDT", mGameData.mCurrentHealth, mGameData.mMaximumHealth, mGameData.mHour, mGameData.unknown1,
            mGameData.mCurrentCell.mData, mGameData.unknown2, mGameData.mPlayerName.mData);
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

    void Header::save(ESMWriter& esm)
    {
        if (mFormatVersion > DefaultFormatVersion)
            esm.writeHNT("FORM", mFormatVersion);

        esm.startSubRecord("HEDR");
        esm.writeT(mData.version);
        esm.writeT(mData.type);
        esm.writeMaybeFixedSizeString(mData.author, 32);
        esm.writeMaybeFixedSizeString(mData.desc, 256);
        esm.writeT(mData.records);
        esm.endRecord("HEDR");

        for (const Header::MasterData& data : mMaster)
        {
            esm.writeHNCString("MAST", data.name);
            esm.writeHNT("DATA", data.size);
        }
    }

}
