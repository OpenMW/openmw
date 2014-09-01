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
            // Support '\r' terminated strings like vanilla.  See Bug #1324.
            char *termsym = strchr(str, '\r');
            if(termsym) *termsym = '\0';
            mVarNames[i] = std::string(str);
            str += mVarNames[i].size() + 1;

            if (str - &tmp[0] > s)
            {
                // Apparently SCVR subrecord is not used and variable names are
                // determined on the fly from the script text.  Therefore don't throw
                // an exeption, just log an error and continue.
                std::stringstream ss;

                ss << "ESM Error: " << "String table overflow";
                ss << "\n  File: " << esm.getName();
                ss << "\n  Record: " << esm.getContext().recName.toString();
                ss << "\n  Subrecord: " << "SCVR";
                ss << "\n  Offset: 0x" << std::hex << esm.getFileOffset();
                std::cerr << ss.str() << std::endl;
                break;
            }

        }
    }

    // Script mData
    mScriptData.resize(mData.mScriptDataSize);
    esm.getHNExact(&mScriptData[0], mScriptData.size(), "SCDT");

    // Script text
    mScriptText = esm.getHNOString("SCTX");

    // NOTE: A minor hack/workaround...
    //
    // MAO_Containers.esp from Morrowind Acoustic Overhaul has SCVR records
    // at the end (see Bug #1849). Since OpenMW does not use SCVR subrecords
    // for variable names just skip these as a quick fix.  An alternative
    // solution would be to decode and validate SCVR subrecords even if they
    // appear here.
    if (esm.isNextSub("SCVR")) {
        esm.skipHSub();
    }
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
