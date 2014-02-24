#include "../bsa_file.hpp"

/*
  Test of the BSAFile class

  This test requires that data/Morrowind.bsa exists in the root
  directory of OpenMW.

 */

#include <iostream>

using namespace std;
using namespace Bsa;

BSAFile bsa;

void find(const char* file)
{
  cout << "Does file '" << file << "' exist?\n  ";
  if(bsa.exists(file))
    {
      cout << "Yes.\n  ";
      cout << bsa.getFile(file)->size() << " bytes\n";
    }
  else cout << "No.\n";
}

int main()
{
  cout << "Reading Morrowind.bsa\n";
  bsa.open("../../data/Morrowind.bsa");

  const BSAFile::FileList &files = bsa.getList();

  cout << "First 10 files in archive:\n";
  for(int i=0; i<10; i++)
    cout << "  " << files[i].name
         << " (" << files[i].fileSize << " bytes @"
         << files[i].offset << ")\n";

  find("meshes\\r\\xnetch_betty.nif");
  find("humdrum");
}
