#include <components/esm/esm_reader.hpp>
#include <components/esm/records.hpp>

#include "esmtool_cmd.h"

#include <iostream>

using namespace std;
using namespace ESM;

void printRaw(ESMReader &esm);
void loadCell(Cell &cell, ESMReader &esm, bool quiet);

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

  try {

  if(info.raw_given)
    {
      cout << "RAW file listing:\n";

      esm.openRaw(filename);

      printRaw(esm);

      return 0;
    }

  bool quiet = info.quiet_given;
  bool loadCells = info.loadcells_given;

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
      esm.getRecHeader();
      string id = esm.getHNOString("NAME");
      if(!quiet)
        cout << "\nRecord: " << n.toString()
             << " '" << id << "'\n";

      switch(n.val)
        {
        case REC_ACTI:
          {
            Activator ac;
            ac.load(esm);
            if(quiet) break;
            cout << "  Name: " << ac.name << endl;
            cout << "  Mesh: " << ac.model << endl;
            cout << "  Script: " << ac.script << endl;
            break;
          }
        case REC_ALCH:
          {
            Potion p;
            p.load(esm);
            if(quiet) break;
            cout << "  Name: " << p.name << endl;
            break;
          }
        case REC_APPA:
          {
            Apparatus p;
            p.load(esm);
            if(quiet) break;
            cout << "  Name: " << p.name << endl;
            break;
          }
        case REC_ARMO:
          {
            Armor am;
            am.load(esm);
            if(quiet) break;
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
            if(quiet) break;
            cout << "  Name: " << bp.name << endl;
            cout << "  Mesh: " << bp.model << endl;
            break;
          }
        case REC_BOOK:
          {
            Book b;
            b.load(esm);
            if(quiet) break;
            cout << "  Name: " << b.name << endl;
            cout << "  Mesh: " << b.model << endl;
            break;
          }
        case REC_BSGN:
          {
            BirthSign bs;
            bs.load(esm);
            if(quiet) break;
            cout << "  Name: " << bs.name << endl;
            cout << "  Texture: " << bs.texture << endl;
            cout << "  Description: " << bs.description << endl;
            break;
          }
        case REC_CELL:
          {
            Cell b;
            b.load(esm);
            if(!quiet)
              {
                cout << "  Name: " << b.name << endl;
                cout << "  Region: " << b.region << endl;
              }
            if(loadCells)
              loadCell(b, esm, quiet);
            break;
          }
        case REC_CLAS:
          {
            Class b;
            b.load(esm);
            if(quiet) break;
            cout << "  Name: " << b.name << endl;
            cout << "  Description: " << b.description << endl;
            break;
          }
        case REC_CLOT:
          {
            Clothing b;
            b.load(esm);
            if(quiet) break;
            cout << "  Name: " << b.name << endl;
            break;
          }
        case REC_CONT:
          {
            Container b;
            b.load(esm);
            if(quiet) break;
            cout << "  Name: " << b.name << endl;
            break;
          }
        case REC_CREA:
          {
            Creature b;
            b.load(esm);
            if(quiet) break;
            cout << "  Name: " << b.name << endl;
            break;
          }
        case REC_DIAL:
          {
            Dialogue b;
            b.load(esm);
            break;
          }
        case REC_DOOR:
          {
            Door d;
            d.load(esm);
            if(quiet) break;
            cout << "  Name: " << d.name << endl;
            cout << "  Mesh: " << d.model << endl;
            cout << "  Script: " << d.script << endl;
            cout << "  OpenSound: " << d.openSound << endl;
            cout << "  CloseSound: " << d.closeSound << endl;
            break;
          }
        case REC_ENCH:
          {
            Enchantment b;
            b.load(esm);
            break;
          }
        case REC_GMST:
          {
            GameSetting b;
            b.id = id;
            b.load(esm);
            if(quiet) break;
            cout << "  Value: ";
            if(b.type == VT_String)
              cout << "'" << b.str << "' (string)";
            else if(b.type == VT_Float)
              cout << b.f << " (float)";
            else if(b.type == VT_Int)
              cout << b.i << " (int)";
            cout << "\n  Dirty: " << b.dirty << endl;
            break;
          }
        case REC_INFO:
          {
            DialInfo p;
            p.load(esm);
            if(quiet) break;
            cout << "  Id: " << p.id << endl;
            cout << "  Text: " << p.response << endl;
            break;
          }
        case REC_SOUN:
          {
            Sound d;
            d.load(esm);
            if(quiet) break;
            cout << "  Sound: " << d.sound << endl;
            cout << "  Volume: " << (int)d.data.volume << endl;
            break;
          }
        case REC_SPEL:
          {
            Spell s;
            s.load(esm);
            if(quiet) break;
            cout << "  Name: " << s.name << endl;
            break;
          }
        default:
          esm.skipRecord();
          if(quiet) break;
          cout << "  Skipping\n";
        }
    }

  } catch(exception &e)
    {
      cout << "\nERROR:\n\n  " << e.what() << endl;
      return 1;
    }

  return 0;
}

void loadCell(Cell &cell, ESMReader &esm, bool quiet)
{
  // Skip back to the beginning of the reference list
  cell.restore(esm);

  // Loop through all the references
  CellRef ref;
  if(!quiet) cout << "  References:\n";
  while(cell.getNextRef(esm, ref))
    {
      if(quiet) continue;

      cout << "    Refnum: " << ref.refnum << endl;
      cout << "    ID: '" << ref.refID << "'\n";
      cout << "    Owner: '" << ref.owner << "'\n";
      cout << "    INTV: " << ref.intv << "   NAM9: " << ref.intv << endl;
    }
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
