#include <iostream>
#include <list>

#include <boost/program_options.hpp>

#include <components/esm/esm_reader.hpp>
#include <components/esm/esm_writer.hpp>
#include <components/esm/records.hpp>

#define ESMTOOL_VERSION 1.1

using namespace std;
using namespace ESM;

// Create a local alias for brevity
namespace bpo = boost::program_options;

void printRaw(ESMReader &esm);
void loadCell(Cell &cell, ESMReader &esm, bool quiet);

struct ESMData
{
    std::string author;
    std::string description;
    int version;
    int type;
    ESMReader::MasterList masters;

    std::list<Record*> records;
};

// Based on the legacy struct
struct Arguments
{
    unsigned int raw_given;
    unsigned int quiet_given;
    unsigned int loadcells_given;

    std::string mode;
    std::string encoding;
    std::string filename;
    std::string outname;
  
    ESMData data;
};

bool parseOptions (int argc, char** argv, Arguments &info)
{
    bpo::options_description desc("Inspect and extract from Morrowind ES files (ESM, ESP, ESS)\nSyntax: esmtool [options] mode infile [outfile]\nAllowed modes:\n  dump\t Dumps all readable data from the input file\n  clone\t Clones the input file to the output file.\n\nAllowed options");

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
        ( "mode,m", bpo::value<std::string>(), "esmtool mode")
        ( "input-file,i", bpo::value< vector<std::string> >(), "input file")
        ;

    bpo::positional_options_description p;
    p.add("mode", 1).add("input-file", 2);

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
    if (!variables.count("mode"))
    {
        std::cout << "No mode specified!" << std::endl << std::endl
                  << desc << finalText << std::endl;
        return false;
    }

    info.mode = variables["mode"].as<std::string>();
    if (!(info.mode == "dump" || info.mode == "clone"))
    {
        std::cout << std::endl << "ERROR: invalid mode \"" << info.mode << "\"" << std::endl << std::endl
                  << desc << finalText << std::endl;
        return false;
    }

    if ( !variables.count("input-file") )
    {
        std::cout << "\nERROR: missing ES file\n\n";
        std::cout << desc << finalText << std::endl;
        return false;
    }

    // handling gracefully the user adding multiple files
/*    if (variables["input-file"].as< vector<std::string> >().size() > 1)
      {
      std::cout << "\nERROR: more than one ES file specified\n\n";
      std::cout << desc << finalText << std::endl;
      return false;
      }*/

    info.filename = variables["input-file"].as< vector<std::string> >()[0];
    if (variables["input-file"].as< vector<std::string> >().size() > 1)
        info.outname = variables["input-file"].as< vector<std::string> >()[1];

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

int load(Arguments& info);
int clone(Arguments& info);

int main(int argc, char**argv)
{
    Arguments info;
    if(!parseOptions (argc, argv, info))
        return 1;

    if (info.mode == "dump")
        return load(info);
    else if (info.mode == "clone")
        return clone(info);
    else
    {
        cout << "Invalid or no mode specified, dying horribly. Have a nice day." << endl;
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

int load(Arguments& info)
{
    ESMReader esm;
    esm.setEncoding(info.encoding);

    string filename = info.filename;
    cout << "\nFile: " << filename << endl;

    std::list<int> skipped;

    try {

        if(info.raw_given && info.mode == "dump")
        {
            cout << "RAW file listing:\n";

            esm.openRaw(filename);

            printRaw(esm);

            return 0;
        }

        bool quiet = (info.quiet_given || info.mode == "clone");
        bool loadCells = (info.loadcells_given);// || info.mode == "clone");
        bool save = (info.mode == "clone");

        esm.open(filename);

        info.data.author = esm.getAuthor();
        info.data.description = esm.getDesc();
        info.data.masters = esm.getMasters();
        info.data.version = esm.getVer();
        info.data.type = esm.getType();

        if (!quiet)
        {
            cout << "Author: " << esm.getAuthor() << endl
                 << "Description: " << esm.getDesc() << endl
                 << "File format version: " << esm.getFVer() << endl
                 << "Special flag: " << esm.getSpecial() << endl;
            ESMReader::MasterList m = esm.getMasters();
            if (!m.empty())
            {
                cout << "Masters:" << endl;
                for(unsigned int i=0;i<m.size();i++)
                    cout << "  " << m[i].name << ", " << m[i].size << " bytes" << endl;
            }
        }

        // Loop through all records
        while(esm.hasMoreRecs())
        {
            NAME n = esm.getRecName();
            esm.getRecHeader();
            string id = esm.getHNOString("NAME");
            if(!quiet)
                cout << "\nRecord: " << n.toString()
                     << " '" << id << "'\n";

            Record* rec = NULL;

            switch(n.val)
            {
            case REC_ACTI:
            {
                rec = new Activator();
                Activator& ac = *(Activator*)rec;
                ac.load(esm);
                if(quiet) break;
                cout << "  Name: " << ac.name << endl;
                cout << "  Mesh: " << ac.model << endl;
                cout << "  Script: " << ac.script << endl;
                break;
            }
            case REC_ALCH:
            {
                rec = new Potion();
                Potion& p = *(Potion*)rec;
                p.load(esm);
                if(quiet) break;
                cout << "  Name: " << p.name << endl;
                break;
            }
            case REC_APPA:
            {
                rec = new Apparatus();
                Apparatus& p = *(Apparatus*)rec;
                p.load(esm);
                if(quiet) break;
                cout << "  Name: " << p.name << endl;
                break;
            }
            case REC_ARMO:
            {
                rec = new Armor();
                Armor& am = *(Armor*)rec;
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
                rec = new BodyPart();
                BodyPart& bp = *(BodyPart*)rec;
                bp.load(esm);
                if(quiet) break;
                cout << "  Name: " << bp.name << endl;
                cout << "  Mesh: " << bp.model << endl;
                break;
            }
            case REC_BOOK:
            {
                rec = new Book();
                Book& b = *(Book*)rec;
                b.load(esm);
                if(quiet) break;
                cout << "  Name: " << b.name << endl;
                cout << "  Mesh: " << b.model << endl;
                break;
            }
            case REC_BSGN:
            {
                rec = new BirthSign();
                BirthSign& bs = *(BirthSign*)rec;
                bs.load(esm);
                if(quiet) break;
                cout << "  Name: " << bs.name << endl;
                cout << "  Texture: " << bs.texture << endl;
                cout << "  Description: " << bs.description << endl;
                break;
            }
            case REC_CELL:
            {
                rec = new Cell();
                Cell& b = *(Cell*)rec;
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
                rec = new Class();
                Class& b = *(Class*)rec;
                b.load(esm);
                if(quiet) break;
                cout << "  Name: " << b.name << endl;
                cout << "  Description: " << b.description << endl;
                break;
            }
            case REC_CLOT:
            {
                rec = new Clothing();
                Clothing& b = *(Clothing*)rec;
                b.load(esm);
                if(quiet) break;
                cout << "  Name: " << b.name << endl;
                break;
            }
            case REC_CONT:
            {
                rec = new Container();
                Container& b = *(Container*)rec;
                b.load(esm);
                if(quiet) break;
                cout << "  Name: " << b.name << endl;
                break;
            }
            case REC_CREA:
            {
                rec = new Creature();
                Creature& b = *(Creature*)rec;
                b.setID(id);
                b.load(esm);
                if(quiet) break;
                cout << "  Name: " << b.name << endl;
                break;
            }
            case REC_DIAL:
            {
                rec = new Dialogue();
                Dialogue& b = *(Dialogue*)rec;
                b.load(esm);
                break;
            }
            case REC_DOOR:
            {
                rec = new Door();
                Door& d = *(Door*)rec;
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
                rec = new Enchantment();
                Enchantment& b = *(Enchantment*)rec;
                b.load(esm);
                break;
            }
            case REC_FACT:
            {
                rec = new Faction();
                Faction& f = *(Faction*)rec;
                f.load(esm);
                if (quiet) break;
                cout << "  Name: " << f.name << endl
                     << "  Attr1: " << f.data.attribute1 << endl
                     << "  Attr2: " << f.data.attribute2 << endl
                     << "  Hidden: " << f.data.isHidden << endl;
                break;
            }
            case REC_GLOB:
            {
                rec = new Global();
                Global& g = *(Global*)rec;
                g.load(esm);
                break;
            }
            case REC_GMST:
            {
                rec = new GameSetting();
                GameSetting& b = *(GameSetting*)rec;
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
                rec = new DialInfo();
                DialInfo& p = *(DialInfo*)rec;
                p.load(esm);
                if(quiet) break;
                cout << "  Id: " << p.id << endl;
                cout << "  Text: " << p.response << endl;
                break;
            }
            case REC_INGR:
            {
                rec = new Ingredient();
                Ingredient& i = *(Ingredient*)rec;
                i.load(esm);
                if (quiet) break;
                cout << "  Name: " << i.name << endl
                     << "  Weight: " << i.data.weight << endl
                     << "  Value: " << i.data.value << endl;
                break;
            }
            case REC_LAND:
            {
                rec = new Land();
                Land& l = *(Land*)rec;
                l.load(esm);
                if (quiet) break;
                cout << "  Coords: [" << l.X << "," << l.Y << "]" << endl;
                break;
            }
            case REC_LEVI:
            {
                rec = new ItemLevList();
                ItemLevList& l = *(ItemLevList*)rec;
                l.load(esm);
                if (quiet) break;
                cout << "  Number of items: " << l.list.size() << endl;
                break;
            }
            case REC_LEVC:
            {
                rec = new CreatureLevList();
                CreatureLevList& l = *(CreatureLevList*)rec;
                l.load(esm);
                if (quiet) break;
                cout << "  Number of items: " << l.list.size() << endl;
                break;
            }
            case REC_LIGH:
            {
                rec = new Light();
                Light& l = *(Light*)rec;
                l.load(esm);
                if (quiet) break;
                cout << "  Name: " << l.name << endl
                     << "  Weight: " << l.data.weight << endl
                     << "  Value: " << l.data.value << endl;
                break;
            }
            case REC_LOCK:
            {
                rec = new Tool();
                Tool& l = *(Tool*)rec;
                l.load(esm);
                if (quiet) break;
                cout << "  Name: " << l.name << endl
                     << "  Quality: " << l.data.quality << endl;
                break;
            }
            case REC_LTEX:
            {
                rec = new LandTexture();
                LandTexture& t = *(LandTexture*)rec;
                t.load(esm);
                if (quiet) break;
                cout << "  Id: " << t.id << endl
                     << "  Texture: " << t.texture << endl;
                break;
            }
            case REC_MISC:
            {
                rec = new Miscellaneous();
                Miscellaneous& m = *(Miscellaneous*)rec;
                m.load(esm);
                if (quiet) break;
                cout << "  Name: " << m.name << endl
                     << "  Value: " << m.data.value << endl;
                break;
            }
            case REC_MGEF:
            {
                rec = new MagicEffect();
                MagicEffect& m = *(MagicEffect*)rec;
                m.load(esm);
                if (quiet) break;
                cout << "  Index: " << m.index << endl
                     << "  " << (m.data.flags & MagicEffect::Negative ? "Negative" : "Positive") << endl;
                break;
            }
            case REC_NPC_:
            {
                rec = new NPC();
                NPC& n = *(NPC*)rec;
                n.load(esm);
                if (quiet) break;
                cout << "  Name: " << n.name << endl
                     << "  Race: " << n.race << endl;
                break;
            }
            case REC_PGRD:
            {
                rec = new Pathgrid();
                Pathgrid& p = *(Pathgrid*)rec;
                p.load(esm);
                if (quiet) break;
                cout << "  Cell: " << p.cell << endl
                     << "  Point count: " << p.points.size() << endl
                     << "  Edge count: " << p.edges.size() << endl;
                break;
            }
            case REC_PROB:
            {
                rec = new Probe();
                Probe& r = *(Probe*)rec;
                r.load(esm);
                if (quiet) break;
                cout << "  Name: " << r.name << endl
                     << "  Quality: " << r.data.quality << endl;
                break;
            }
            case REC_RACE:
            {
                rec = new Race();
                Race& r = *(Race*)rec;
                r.load(esm);
                if (quiet) break;
                cout << "  Name: " << r.name << endl
                     << "  Length: " << r.data.height.male << "m " << r.data.height.female << "f" << endl;
                break;
            }
            case REC_REGN:
            {
                rec = new Region();
                Region& r = *(Region*)rec;
                r.load(esm);
                if (quiet) break;
                cout << "  Name: " << r.name << endl;
                break;
            }
            case REC_REPA:
            {
                rec = new Repair();
                Repair& r = *(Repair*)rec;
                r.load(esm);
                if (quiet) break;
                cout << "  Name: " << r.name << endl
                     << "  Quality: " << r.data.quality << endl;
                break;
            }
            case REC_SCPT:
            {
                rec = new Script();
                Script& s = *(Script*)rec;
                s.load(esm);
                if (quiet) break;
                cout << "  Name: " << s.data.name.toString() << endl;
                break;
            }
            case REC_SKIL:
            {
                rec = new Skill();
                Skill& s = *(Skill*)rec;
                s.load(esm);
                if (quiet) break;
                cout << "  ID: " << s.index << endl
                     << "  Type: " << (s.data.specialization == 0 ? "Combat" : (s.data.specialization == 1 ? "Magic" : "Stealth")) << endl;
                break;
            }
            case REC_SNDG:
            {
                rec = new SoundGenerator();
                SoundGenerator& s = *(SoundGenerator*)rec;
                s.load(esm);
                if (quiet) break;
                cout << "  Creature: " << s.creature << endl
                     << "  Sound: " << s.sound << endl;
                break;
            }
            case REC_SOUN:
            {
                rec = new Sound();
                Sound& d = *(Sound*)rec;
                d.load(esm);
                if(quiet) break;
                cout << "  Sound: " << d.sound << endl;
                cout << "  Volume: " << (int)d.data.volume << endl;
                break;
            }
            case REC_SPEL:
            {
                rec = new Spell();
                Spell& s = *(Spell*)rec;
                s.load(esm);
                if(quiet) break;
                cout << "  Name: " << s.name << endl;
                break;
            }
            case REC_STAT:
            {
                rec = new Static();
                Static& s = *(Static*)rec;
                s.load(esm);
                if (quiet) break;
                cout << "  Model: " << s.model << endl;
                break;
            }
            case REC_WEAP:
            {
                rec = new Weapon();
                Weapon& w = *(Weapon*)rec;
                w.load(esm);
                if (quiet) break;
                cout << "  Name: " << w.name << endl
                     << "  Chop: " << w.data.chop[0] << "-" << w.data.chop[1] << endl
                     << "  Slash: " << w.data.slash[0] << "-" << w.data.slash[1] << endl
                     << "  Thrust: " << w.data.thrust[0] << "-" << w.data.thrust[1] << endl
                     << "  Value: " << w.data.value << endl;
                break;
            }
            default:
                if (find(skipped.begin(), skipped.end(), n.val) == skipped.end())
                {
                    cout << "Skipping " << n.toString() << " records." << endl;
                    skipped.push_back(n.val);
                }

                esm.skipRecord();
                if(quiet) break;
                cout << "  Skipping\n";
            }

            if (rec != NULL)
            {
                rec->setId(id);

                if (save)
                    info.data.records.push_back(rec);
                else
                    delete rec;
            }
        }

    } catch(exception &e)
    {
        cout << "\nERROR:\n\n  " << e.what() << endl;

        for (std::list<Record*>::iterator it = info.data.records.begin(); it != info.data.records.end();)
        {
            delete *it;
            info.data.records.erase(it++);
        }
        return 1;
    }

    return 0;
}

#include <map>
#include <iomanip>

int clone(Arguments& info)
{
    if (info.outname.empty())
    {
        cout << "You need to specify an output name" << endl;
        return 1;
    }

    if (load(info) != 0)
    {
        cout << "Failed to load, aborting." << endl;
        return 1;
    }

    int recordCount = info.data.records.size();

    int digitCount = 1; // For a nicer output
    if (recordCount > 9) ++digitCount;
    if (recordCount > 99) ++digitCount;
    if (recordCount > 999) ++digitCount;
    if (recordCount > 9999) ++digitCount;
    if (recordCount > 99999) ++digitCount;
    if (recordCount > 999999) ++digitCount;

    cout << "Loaded " << recordCount << " records:" << endl << endl;

    std::map<std::string, int> records;

    for (std::list<Record*>::iterator it = info.data.records.begin(); it != info.data.records.end(); ++it)
    {
        Record* rec = *it;
        NAME n;
        n.val = rec->getName();
        records[n.toString()]++;
    }

    int i = 0;
    for (std::map<std::string,int>::iterator it = records.begin(); it != records.end(); ++it)
    {
        std::string n = it->first;
        float amount = it->second;
        cout << setw(digitCount) << amount << " " << n << "  ";

        if (++i % 3 == 0)
            cout << endl;
    }
    
    if (i % 3 != 0)
        cout << endl;

    cout << endl << "Saving records to: " << info.outname << "..." << endl;

    ESMWriter esm;
    esm.setAuthor(info.data.author);
    esm.setDescription(info.data.description);
    esm.setVersion(info.data.version);
    esm.setType(info.data.type);

    for (ESMReader::MasterList::iterator it = info.data.masters.begin(); it != info.data.masters.end(); ++it)
        esm.addMaster(it->name, it->size);

    std::fstream save(info.outname.c_str(), std::fstream::out | std::fstream::binary);
    esm.save(save);

    int saved = 0;
    for (std::list<Record*>::iterator it = info.data.records.begin(); it != info.data.records.end() && i > 0; ++it)
    {
        Record* rec = *it;
        
        NAME n;
        n.val = rec->getName();

        esm.startRecord(n.toString(), 0);
        std::string id = rec->getId();
        esm.writeHNOString("NAME", id);
        rec->save(esm);
        esm.endRecord(n.toString());

        saved++;
        int perc = (saved / (float)recordCount)*100;
        if (perc % 10 == 0)
        {
            cerr << "\r" << perc << "%";
        }
    }
    
    cout << "\rDone!" << endl;

    esm.close();
    save.close();

    return 0;
}
