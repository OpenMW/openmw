#include "loadscpt.hpp"

namespace ESM
{

void Script::load(ESMReader &esm)
{
    esm.getHNT(data, "SCHD", 52);

    // List of local variables
    if (esm.isNextSub("SCVR"))
    {
        int s = data.stringTableSize;
        char* tmp = new char[s];
        esm.getHExact(tmp, s);

        // Set up the list of variable names
        varNames.resize(data.numShorts + data.numLongs + data.numFloats);

        // The tmp buffer is a null-byte separated string list, we
        // just have to pick out one string at a time.
        char* str = tmp;
        for (size_t i = 0; i < varNames.size(); i++)
        {
            varNames[i] = std::string(str);
            str += varNames[i].size() + 1;

            if (str - tmp > s)
                esm.fail("String table overflow");
        }
        delete[] tmp;
    }

    // Script data
    scriptData.resize(data.scriptDataSize);
    esm.getHNExact(&scriptData[0], scriptData.size(), "SCDT");

    // Script text
    scriptText = esm.getHNOString("SCTX");
}
void Script::save(ESMWriter &esm)
{
    esm.writeHNT("SCHD", data, 52);
    
    if (!varNames.empty())
    {
        esm.writeName("SCVR");
        for (std::vector<std::string>::iterator it = varNames.begin(); it != varNames.end(); ++it)
        {
            esm.writeT(it->c_str(), it->size());
        }
    }

    esm.writeHNT("SCDT", &scriptData[0], scriptData.size());
    esm.writeHNOString("SCDT", scriptText);
}

}
