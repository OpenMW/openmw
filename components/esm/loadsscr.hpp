#ifndef _ESM_SSCR_H
#define _ESM_SSCR_H

#include "esm_reader.hpp"

namespace ESM {

/*
  Startup script. I think this is simply a 'main' script that is run
  from the begining. The SSCR records contain a DATA identifier which
  is totally useless (TODO: don't remember what it contains exactly,
  document it below later.), and a NAME which is simply a script
  reference.
 */

struct StartScript
{
  std::string script;

  // Load a record and add it to the list
  void load(ESMReader &esm)
  {
    esm.getSubNameIs("DATA");
    esm.skipHSub();
    script = esm.getHNString("NAME");
  }
};

}
#endif
