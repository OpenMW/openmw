#include "importproj.h"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

void ESSImport::PROJ::load(ESM::ESMReader& esm)
{
    while (esm.isNextSub("PNAM"))
    {
        PNAM pnam;
        esm.getHT(pnam);
        mProjectiles.push_back(pnam);
    }
}

}
