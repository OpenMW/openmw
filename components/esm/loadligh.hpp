#ifndef _ESM_LIGH_H
#define _ESM_LIGH_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * Lights. Includes static light sources and also carryable candles
 * and torches.
 */

struct Light : public Record
{
    enum Flags
    {
        Dynamic = 0x001,
        Carry = 0x002, // Can be carried
        Negative = 0x004, // Negative light?
        Flicker = 0x008,
        Fire = 0x010,
        OffDefault = 0x020, // Off by default
        FlickerSlow = 0x040,
        Pulse = 0x080,
        PulseSlow = 0x100
    };

    struct LHDTstruct
    {
        float weight;
        int value;
        int time; // Duration
        int radius;
        int color; // 4-byte rgba value
        int flags;
    }; // Size = 24 bytes

    LHDTstruct data;

    std::string sound, script, model, icon, name;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_LIGH; }
};
}
#endif
