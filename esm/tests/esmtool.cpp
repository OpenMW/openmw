#include "../esm_reader.hpp"
#include "../records.hpp"

#include "esmtool_cmd.h"

#include <iostream>

using namespace std;
using namespace ESM;

void printRaw(ESMReader &esm);

int main(int argc, char**argv)
{
  gengetopt_args_info info;

  if(cmdline_parser(argc, argv, &info) != 0)
    return 1;

  if(info.inputs_num != 1)
    {
      if(info.inputs_num == 0)
        cout << "ERROR: missing ES file\n\n";
      else
        cout << "ERROR: more than one ES file specified\n\n";
      cmdline_parser_print_help();
      return 1;
    }

  ESMReader esm;
  const char* filename = info.inputs[0];
  cout << "\nFile: " << filename << endl;

  if(info.raw_given)
    {
      cout << "RAW file listing:\n";

      esm.openRaw(filename);

      printRaw(esm);

      return 0;
    }

  esm.open(filename);

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
      string id = esm.getHNOString("NAME");
      cout << " '" << id << "'\n";

      switch(n.val)
        {
        case REC_ARMO:
          {
            Armor am;
            am.load(esm);
            cout << "  Name: " << am.name << endl;
            cout << "  Mesh: " << am.model << endl;
            cout << "  Icon: " << am.icon << endl;
            cout << "  Script: " << am.script << endl;
            cout << "  Enchantment: " << am.enchant << endl;
            cout << "  Type: " << am.data.type << endl;
            cout << "  Weight: " << am.data.weight << endl;
            break;
          }
        case REC_BODY:
          {
            BodyPart bp;
            bp.load(esm);
            cout << "  Name: " << bp.name << endl;
            cout << "  Mesh: " << bp.model << endl;
            break;
          }
        case REC_BSGN:
          {
            BirthSign bs;
            bs.load(esm);
            cout << "  Name: " << bs.name << endl;
            cout << "  Texture: " << bs.texture << endl;
            cout << "  Description: " << bs.description << endl;
            break;
          }
        case REC_DOOR:
          {
            Door d;
            d.load(esm);
            cout << "  Name: " << d.name << endl;
            cout << "  Mesh: " << d.model << endl;
            cout << "  Script: " << d.script << endl;
            cout << "  OpenSound: " << d.openSound << endl;
            cout << "  CloseSound: " << d.closeSound << endl;
            break;
          }
        case REC_SOUN:
          {
            Sound d;
            d.load(esm);
            cout << "  Sound: " << d.sound << endl;
            cout << "  Volume: " << (int)d.data.volume << endl;
            break;
          }
        default:
          cout << "  Skipping\n";
          esm.skipRecord();
        }
    }

  return 0;
}

void printRaw(ESMReader &esm)
{
  while(esm.hasMoreRecs())
    {
      NAME n = esm.getRecName();
      cout << "Record: " << n.toString() << endl;
      esm.getRecHeader();
      while(esm.hasMoreSubs())
        {
          uint64_t offs = esm.getOffset();
          esm.getSubName();
          esm.skipHSub();
          n = esm.retSubName();
          cout << "    " << n.toString() << " - " << esm.getSubSize()
               << " bytes @ 0x" << hex << offs << "\n";
        }
    }
}
