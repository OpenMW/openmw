/*
  Runs NIFFile through all the NIFs in Morrowind.bsa.
 */

#include "../nif_file.hpp"
#include "../../bsa/bsa_file.hpp"
#include "../../tools/stringops.hpp"
#include <iostream>

using namespace Mangle::Stream;
using namespace std;
using namespace Nif;

int main(int argc, char **args)
{
  BSAFile bsa;
  cout << "Reading Morrowind.bsa\n";
  bsa.open("../../data/Morrowind.bsa");

  const BSAFile::FileList &files = bsa.getList();

  for(int i=0; i<files.size(); i++)
    {
      const char *n = files[i].name;
      if(!ends(n, ".nif")) continue;

      cout << "Decoding " << n << endl;
      NIFFile nif(bsa.getFile(n), n);
    }
}
