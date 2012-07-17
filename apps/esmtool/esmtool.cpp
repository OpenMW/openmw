#include <iostream>

#include <boost/program_options.hpp>

#include <components/esm/esm_reader.hpp>
#include <components/esm/records.hpp>

#define ESMTOOL_VERSION 1.1

using namespace std;
using namespace ESM;

// Create a local alias for brevity
namespace bpo = boost::program_options;

void printRaw(ESMReader &esm);
void loadCell(Cell &cell, ESMReader &esm, bool quiet);

// Based on the legacy struct
struct Arguments
{
    unsigned int raw_given;
    unsigned int quiet_given;
    unsigned int loadcells_given;
    std::string encoding;
    std::string filename;
};

bool parseOptions (int argc, char** argv, Arguments &info)
{
    bpo::options_description desc("Inspect and extract from Morrowind ES files (ESM, ESP, ESS)\nSyntax: esmtool [options] file \nAllowed options");

    desc.add_options()
        ("help,h", "print help message.")
        ("version,v", "print version information and quit.")
        ("raw,r", "Show an unformattet list of all records and subrecords.")
        ("quiet,q", "Supress all record information. Useful for speed tests.")
        ("loadcells,C", "Browse through contents of all cells.")

        ( "encoding,e", bpo::value<std::string>(&(info.encoding))->
            default_value("win1252"),
            "Character encoding used in ESMTool:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default")
        ;

    std::string finalText = "\nIf no option is given, the default action is to parse all records in the archive\nand display diagnostic information.";

    // input-file is hidden and used as a positional argument
    bpo::options_description hidden("Hidden Options");

    hidden.add_options()
        ( "input-file,i", bpo::value< vector<std::string> >(), "input file")
        ;

    bpo::positional_options_description p;
    p.add("input-file", -1);

    // there might be a better way to do this
    bpo::options_description all;
    all.add(desc).add(hidden);
    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
        .options(all).positional(p).run();

    bpo::variables_map variables;
    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    if (variables.count ("help"))
    {
        std::cout << desc << finalText << std::endl;
        return false;
    }
    if (variables.count ("version"))
    {
        std::cout << "ESMTool version " << ESMTOOL_VERSION << std::endl;
        return false;
    }

    if ( !variables.count("input-file") )
    {
        std::cout << "\nERROR: missing ES file\n\n";
        std::cout << desc << finalText << std::endl;
        return false;
    }

    // handling gracefully the user adding multiple files
    if (variables["input-file"].as< vector<std::string> >().size() > 1)
    {
        std::cout << "\nERROR: more than one ES file specified\n\n";
        std::cout << desc << finalText << std::endl;
        return false;
    }

    info.filename = variables["input-file"].as< vector<std::string> >()[0];

    info.raw_given = variables.count ("raw");
    info.quiet_given = variables.count ("quiet");
    info.loadcells_given = variables.count ("loadcells");

    // Font encoding settings
    info.encoding = variables["encoding"].as<std::string>();
    if (info.encoding == "win1250")
    {
        std::cout << "Using Central and Eastern European font encoding." << std::endl;
    }
    else if (info.encoding == "win1251")
    {
        std::cout << "Using Cyrillic font encoding." << std::endl;
    }
    else
    {
        if(info.encoding != "win1252")
        {
            std::cout << info.encoding << " is not a valid encoding option." << std::endl;
            info.encoding = "win1252";
        }
        std::cout << "Using default (English) font encoding." << std::endl;
    }

    return true;
}


int main(int argc, char**argv)
{
  Arguments info;
  if(!parseOptions (argc, argv, info))
    return 1;

  ESMReader esm;
  esm.setEncoding(info.encoding);

  string filename = info.filename;
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
  for(unsigned int i=0;i<m.size();i++)
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
            p.load(esm, id);
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
            b.load(esm, id);
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
