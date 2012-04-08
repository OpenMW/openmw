#include "loadcrea.hpp"

namespace ESM {

void Creature::setID(const std::string& id)
{
    mId = id;
}

void Creature::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    original = esm.getHNOString("CNAM");
    name = esm.getHNOString("FNAM");
    script = esm.getHNOString("SCRI");

    esm.getHNT(data, "NPDT", 96);

    esm.getHNT(flags, "FLAG");
    scale = 1.0;
    esm.getHNOT(scale, "XSCL");

    inventory.load(esm);

    // More subrecords:

    // AIDT - data (12 bytes, unknown)
    // AI_W - wander (14 bytes, i don't understand it)
    //    short distance
    //    byte duration
    //    byte timeOfDay
    //    byte idle[10]
    //
    // Rest is optional:
    // AI_T - travel?
    // AI_F - follow?
    // AI_E - escort?
    // AI_A - activate?

    esm.skipRecord();
}

void Creature::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNOString("CNAM", original);
    esm.writeHNOString("FNAM", name);
    esm.writeHNOString("SCRI", script);
    esm.writeHNT("NPDT", data, 96);
    esm.writeHNT("FLAG", flags);
    if (scale != 1.0)
        esm.writeHNT("XSCL", scale);
    inventory.save(esm);
}

}
