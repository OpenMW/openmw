#include "custommarkerstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void CustomMarker::save(ESMWriter& esm) const
    {
        esm.writeHNT("POSX", mWorldX);
        esm.writeHNT("POSY", mWorldY);
        esm.writeCellId(mCell);
        if (!mNote.empty())
            esm.writeHNString("NOTE", mNote);
    }

    void CustomMarker::load(ESMReader& esm)
    {
        esm.getHNT(mWorldX, "POSX");
        esm.getHNT(mWorldY, "POSY");
        mCell = esm.getCellId();
        mNote = esm.getHNOString("NOTE");
    }

}
