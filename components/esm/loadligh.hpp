#ifndef _ESM_LIGH_H
#define _ESM_LIGH_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Lights. Includes static light sources and also carryable candles
 * and torches.
 */

struct Light
{
  enum Flags
    {
      Dynamic       = 0x001,
      Carry     = 0x002, // Can be carried
      Negative      = 0x004, // Negative light?
      Flicker       = 0x008,
      Fire      = 0x010,
      OffDefault    = 0x020, // Off by default
      FlickerSlow   = 0x040,
      Pulse     = 0x080,
      PulseSlow     = 0x100
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

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    icon = esm.getHNOString("ITEX");
    assert(sizeof(data) == 24);
    esm.getHNT(data, "LHDT", 24);
    script = esm.getHNOString("SCRI");
    sound = esm.getHNOString("SNAM");
  }
};
}
#endif
