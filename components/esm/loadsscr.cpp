#include "loadsscr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int StartScript::sRecordId = REC_SSCR;

void StartScript::load(ESMReader &esm)
{
    mData = esm.getHNString("DATA");
    mScript = esm.getHNString("NAME");
}
void StartScript::save(ESMWriter &esm) const
{
    esm.writeHNString("DATA", mData);
    esm.writeHNString("NAME", mScript);
}

}
