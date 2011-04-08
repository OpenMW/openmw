#include "loadcell.hpp"

namespace ESM
{

void Cell::load(ESMReader &esm)
{
    // Ignore this for now, it might mean we should delete the entire
    // cell?
    if (esm.isNextSub("DELE"))
        esm.skipHSub();

    esm.getHNT(data, "DATA", 12);

    // Water level
    water = 0;

    if (data.flags & Interior)
    {
        // Interior cells

        if (esm.isNextSub("INTV") || esm.isNextSub("WHGT"))
            esm.getHT(water);

        // Quasi-exterior cells have a region (which determines the
        // weather), pure interior cells have ambient lighting
        // instead.
        if (data.flags & QuasiEx)
            region = esm.getHNOString("RGNN");
        else
            esm.getHNT(ambi, "AMBI", 16);
    }
    else
    {
        // Exterior cells
        region = esm.getHNOString("RGNN");
        esm.getHNOT(mapColor, "NAM5");
    }

    // Save position of the cell references and move on
    context = esm.getContext();
    esm.skipRecord();
}

void Cell::restore(ESMReader &esm) const
{
    esm.restoreContext(context);
}

bool Cell::getNextRef(ESMReader &esm, CellRef &ref)
{
    if (!esm.hasMoreSubs())
        return false;

    // Number of references in the cell? Maximum once in each cell,
    // but not always at the beginning, and not always right. In other
    // words, completely useless.
    {
        int i;
        esm.getHNOT(i, "NAM0");
    }

    esm.getHNT(ref.refnum, "FRMR");
    ref.refID = esm.getHNString("NAME");

    // getHNOT will not change the existing value if the subrecord is
    // missing
    ref.scale = 1.0;
    esm.getHNOT(ref.scale, "XSCL");

    ref.owner = esm.getHNOString("ANAM");
    ref.glob = esm.getHNOString("BNAM");
    ref.soul = esm.getHNOString("XSOL");

    ref.faction = esm.getHNOString("CNAM");
    ref.factIndex = -1;
    esm.getHNOT(ref.factIndex, "INDX");

    ref.charge = -1.0;
    esm.getHNOT(ref.charge, "XCHG");

    ref.intv = 0;
    ref.nam9 = 0;
    esm.getHNOT(ref.intv, "INTV");
    esm.getHNOT(ref.nam9, "NAM9");

    // Present for doors that teleport you to another cell.
    if (esm.isNextSub("DODT"))
    {
        ref.teleport = true;
        esm.getHT(ref.doorDest);
        ref.destCell = esm.getHNOString("DNAM");
    }
    else
        ref.teleport = false;

    // Integer, despite the name suggesting otherwise
    ref.lockLevel = 0;
    esm.getHNOT(ref.lockLevel, "FLTV");
    ref.key = esm.getHNOString("KNAM");
    ref.trap = esm.getHNOString("TNAM");

    ref.unam = 0;
    ref.fltv = 0;
    esm.getHNOT(ref.unam, "UNAM");
    esm.getHNOT(ref.fltv, "FLTV");

    esm.getHNT(ref.pos, "DATA", 24);

    return true;
}

}
