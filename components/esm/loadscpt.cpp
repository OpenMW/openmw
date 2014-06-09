#include "loadscpt.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{

struct SCHD
{
    NAME32              mName;
    Script::SCHDstruct  mData;
};

    unsigned int Script::sRecordId = REC_SCPT;

void Script::load(ESMReader &esm)
{
    SCHD data;
    esm.getHNT(data, "SCHD", 52);
    mData = data.mData;
    mId = data.mName.toString();

    // List of local variables
    if (esm.isNextSub("SCVR"))
    {
        int s = mData.mStringTableSize;

        std::vector<char> tmp (s);
        esm.getHExact (&tmp[0], s);

        // Set up the list of variable names
        mVarNames.resize(mData.mNumShorts + mData.mNumLongs + mData.mNumFloats);

        // The tmp buffer is a null-byte separated string list, we
        // just have to pick out one string at a time.
        char* str = &tmp[0];
        for (size_t i = 0; i < mVarNames.size(); i++)
        {
            char *termsym = strchr(str, '\r');
            if(termsym) *termsym = '\0';
            mVarNames[i] = std::string(str);
            str += mVarNames[i].size() + 1;

            if (str - &tmp[0] > s)
                esm.fail("String table overflow");
        }
    }

    // Script mData
    mScriptData.resize(mData.mScriptDataSize);
    esm.getHNExact(&mScriptData[0], mScriptData.size(), "SCDT");

    // Script text
    mScriptText = esm.getHNOString("SCTX");
}
void Script::save(ESMWriter &esm) const
{
    std::string varNameString;
    if (!mVarNames.empty())
        for (std::vector<std::string>::const_iterator it = mVarNames.begin(); it != mVarNames.end(); ++it)
            varNameString.append(*it);

    SCHD data;
    memset(&data, 0, sizeof(data));

    data.mData = mData;
    memcpy(data.mName.name, mId.c_str(), mId.size());

    esm.writeHNT("SCHD", data, 52);

    if (!mVarNames.empty())
    {
        esm.startSubRecord("SCVR");
        for (std::vector<std::string>::const_iterator it = mVarNames.begin(); it != mVarNames.end(); ++it)
        {
            esm.writeHCString(*it);
        }
        esm.endRecord("SCVR");
    }

    esm.startSubRecord("SCDT");
    esm.write(reinterpret_cast<const char * >(&mScriptData[0]), mData.mScriptDataSize);
    esm.endRecord("SCDT");

    esm.writeHNOString("SCTX", mScriptText);
}

    void Script::blank()
    {
        mData.mNumShorts = mData.mNumLongs = mData.mNumFloats = 0;
        mData.mScriptDataSize = 0;
        mData.mStringTableSize = 0;

        mVarNames.clear();
        mScriptData.clear();
        mScriptText = "Begin " + mId + "\n\nEnd " + mId + "\n";
    }

}
