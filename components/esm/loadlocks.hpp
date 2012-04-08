#ifndef _ESM_LOCKS_H
#define _ESM_LOCKS_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * This file covers lockpicks (LOCK), probes (PROB) and armor repair
 * items (REPA). These have nearly identical data structures.
 */

struct Tool : public Record
{
    enum Type
    {
        Type_Pick,
        Type_Probe,
        Type_Repair
    };

    struct Data
    {
        float weight;
        int value;

        float quality; // And when I say nearly identical structure, I
        int uses;      // mean perfectly identical except that these two
                       // variables are swaped for repair items. Don't ask
                       // me why.
    }; // Size = 16

    Data data;
    Type type;
    std::string name, model, icon, script;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName()
    {
        if (type == Type_Probe)
            return REC_PROB;
        else if (type == Type_Repair)
            return REC_REPA;
        else
            return REC_LOCK;
    }
};

struct Probe: Tool
{
    Probe() { type = Type_Probe; }
};

struct Repair: Tool
{
    Repair() { type = Type_Repair; }
};

}
#endif
