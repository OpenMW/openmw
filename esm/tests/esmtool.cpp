#include "../esm_reader.hpp"
#include "../records.hpp"

#include <iostream>

using namespace std;

int main(int argc, char**argv)
{
  if(argc != 2)
    {
      cout << "Specify an ES file\n";
      return 1;
    }

  ESMReader esm;
  esm.open(argv[1]);

  cout << "\nFile: " << argv[1] << endl;
  cout << "Author: " << esm.getAuthor() << endl;
  cout << "Description: " << esm.getDesc() << endl;
  cout << "File format version: " << esm.getFVer() << endl;
  cout << "Special flag: " << esm.getSpecial() << endl;
  cout << "Masters:\n";
  ESMReader::MasterList m = esm.getMasters();
  for(int i=0;i<m.size();i++)
    cout << "  " << m[i].name << ", " << m[i].size << " bytes\n";

  // Loop through all records
  while(esm.hasMoreRecs())
    {
      NAME n = esm.getRecName();
      cout << "\nRecord: " << n.toString();
      esm.getRecHeader();
      cout << " '" << esm.getHNString("NAME") << "'\n";

      switch(n.val)
        {
        case REC_BODY:
          {
            BodyPart bp;
            bp.load(esm);
            cout << "  Name: " << bp.name << endl;
            cout << "  Mesh: " << bp.model << endl;
            break;
          }
        default:
          cout << "  Skipping\n";
          esm.skipRecord();
        }
    }

  return 0;
}
