#include "loadsscr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void StartScript::load(ESMReader &esm)
{
    mData = esm.getHNString("DATA");
    mScript = esm.getHNString("NAME");
}
void StartScript::save(ESMWriter &esm)
{
    esm.writeHNString("DATA", mData);
    esm.writeHNString("NAME", mScript);
}

}
