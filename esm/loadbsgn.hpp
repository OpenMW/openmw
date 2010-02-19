#ifndef _ESM_BSGN_H
#define _ESM_BSGN_H

#include "esm_reader.hpp"
//#include "loadspel.hpp"

struct BirthSign
{
  std::string name, description, texture;

  // List of powers and abilities that come with this birth sign.
  //SpellList powers;

  void load(ESMReader &esm)
  {
    name = esm.getHNString("FNAM");
    texture = esm.getHNOString("TNAM");
    description = esm.getHNOString("DESC");

    //powers.load(esm);
  };
};

#endif
