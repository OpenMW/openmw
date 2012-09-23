#include "loadscpt.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Script::load(ESMReader &esm)
{
    esm.getHNT(mData, "SCHD", 52);

    // List of local variables
    if (esm.isNextSub("SCVR"))
    {
        int s = mData.mStringTableSize;
        char* tmp = new char[s];
        esm.getHExact(tmp, s);

        // Set up the list of variable names
        mVarNames.resize(mData.mNumShorts + mData.mNumLongs + mData.mNumFloats);

        // The tmp buffer is a null-byte separated string list, we
        // just have to pick out one string at a time.
        char* str = tmp;
        for (size_t i = 0; i < mVarNames.size(); i++)
        {
            mVarNames[i] = std::string(str);
            str += mVarNames[i].size() + 1;

            if (str - tmp > s)
                esm.fail("String table overflow");
        }
        delete[] tmp;
    }

    // Script mData
    mScriptData.resize(mData.mScriptDataSize);
    esm.getHNExact(&mScriptData[0], mScriptData.size(), "SCDT");

    // Script text
    mScriptText = esm.getHNOString("SCTX");
}
void Script::save(ESMWriter &esm)
{
    std::string varNameString;
    if (!mVarNames.empty())
        for (std::vector<std::string>::iterator it = mVarNames.begin(); it != mVarNames.end(); ++it)
            varNameString.append(*it);

    esm.writeHNT("SCHD", mData, 52);
    
    if (!mVarNames.empty())
    {
        esm.startSubRecord("SCVR");
        for (std::vector<std::string>::iterator it = mVarNames.begin(); it != mVarNames.end(); ++it)
        {
            esm.writeHCString(*it);
        }
        esm.endRecord("SCVR");
    }

    esm.startSubRecord("SCDT");
    esm.write(&mScriptData[0], mData.mScriptDataSize);
    esm.endRecord("SCDT");

    esm.writeHNOString("SCTX", mScriptText);
}

}
