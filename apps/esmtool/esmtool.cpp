#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>

#include <boost/program_options.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/records.hpp>

#include "record.hpp"

#define ESMTOOL_VERSION 1.2

// Create a local alias for brevity
namespace bpo = boost::program_options;

struct ESMData
{
    std::string author;
    std::string description;
    int version;
    std::vector<ESM::Header::MasterData> masters;

    std::deque<EsmTool::RecordBase *> mRecords;
    std::map<ESM::Cell *, std::deque<ESM::CellRef> > mCellRefs;
    std::map<int, int> mRecordStats;

    static const std::set<int> sLabeledRec;
};

static const int sLabeledRecIds[] = {
    ESM::REC_GLOB, ESM::REC_CLAS, ESM::REC_FACT, ESM::REC_RACE, ESM::REC_SOUN,
    ESM::REC_REGN, ESM::REC_BSGN, ESM::REC_LTEX, ESM::REC_STAT, ESM::REC_DOOR,
    ESM::REC_MISC, ESM::REC_WEAP, ESM::REC_CONT, ESM::REC_SPEL, ESM::REC_CREA,
    ESM::REC_BODY, ESM::REC_LIGH, ESM::REC_ENCH, ESM::REC_NPC_, ESM::REC_ARMO,
    ESM::REC_CLOT, ESM::REC_REPA, ESM::REC_ACTI, ESM::REC_APPA, ESM::REC_LOCK,
    ESM::REC_PROB, ESM::REC_INGR, ESM::REC_BOOK, ESM::REC_ALCH, ESM::REC_LEVI,
    ESM::REC_LEVC, ESM::REC_SNDG, ESM::REC_CELL, ESM::REC_DIAL
};

const std::set<int> ESMData::sLabeledRec =
    std::set<int>(sLabeledRecIds, sLabeledRecIds + 34);

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

    std::vector<std::string> types;

    ESMData data;
    ESM::ESMReader reader;
    ESM::ESMWriter writer;
};

bool parseOptions (int argc, char** argv, Arguments &info)
{
    bpo::options_description desc("Inspect and extract from Morrowind ES files (ESM, ESP, ESS)\nSyntax: esmtool [options] mode infile [outfile]\nAllowed modes:\n  dump\t Dumps all readable data from the input file.\n  clone\t Clones the input file to the output file.\n  comp\t Compares the given files.\n\nAllowed options");

    desc.add_options()
        ("help,h", "print help message.")
        ("version,v", "print version information and quit.")
        ("raw,r", "Show an unformatted list of all records and subrecords.")
        // The intention is that this option would interact better
        // with other modes including clone, dump, and raw.
        ("type,t", bpo::value< std::vector<std::string> >(),
         "Show only records of this type (four character record code).  May "
         "be specified multiple times.  Only affects dump mode.")
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
        ( "input-file,i", bpo::value< std::vector<std::string> >(), "input file")
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

    if (variables.count("type") > 0)
      info.types = variables["type"].as< std::vector<std::string> >();

    info.mode = variables["mode"].as<std::string>();
    if (!(info.mode == "dump" || info.mode == "clone" || info.mode == "comp"))
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
/*    if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
      {
      std::cout << "\nERROR: more than one ES file specified\n\n";
      std::cout << desc << finalText << std::endl;
      return false;
      }*/

    info.filename = variables["input-file"].as< std::vector<std::string> >()[0];
    if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
        info.outname = variables["input-file"].as< std::vector<std::string> >()[1];

    info.raw_given = variables.count ("raw");
    info.quiet_given = variables.count ("quiet");
    info.loadcells_given = variables.count ("loadcells");

    // Font encoding settings
    info.encoding = variables["encoding"].as<std::string>();
    if(info.encoding != "win1250" && info.encoding != "win1251" && info.encoding != "win1252")
    {
        std::cout << info.encoding << " is not a valid encoding option." << std::endl;
        info.encoding = "win1252";
    }
    std::cout << ToUTF8::encodingUsingMessage(info.encoding) << std::endl;

    return true;
}

void printRaw(ESM::ESMReader &esm);
void loadCell(ESM::Cell &cell, ESM::ESMReader &esm, Arguments& info);

int load(Arguments& info);
int clone(Arguments& info);
int comp(Arguments& info);

int main(int argc, char**argv)
{
    Arguments info;
    if(!parseOptions (argc, argv, info))
        return 1;

    if (info.mode == "dump")
        return load(info);
    else if (info.mode == "clone")
        return clone(info);
    else if (info.mode == "comp")
        return comp(info);
    else
    {
        std::cout << "Invalid or no mode specified, dying horribly. Have a nice day." << std::endl;
        return 1;
    }

    return 0;
}

void loadCell(ESM::Cell &cell, ESM::ESMReader &esm, Arguments& info)
{
    bool quiet = (info.quiet_given || info.mode == "clone");
    bool save = (info.mode == "clone");

    // Skip back to the beginning of the reference list
    // FIXME: Changes to the references backend required to support multiple plugins have
    //  almost certainly broken this following line. I'll leave it as is for now, so that
    //  the compiler does not complain.
    cell.restore(esm, 0);

    // Loop through all the references
    ESM::CellRef ref;
    if(!quiet) std::cout << "  References:\n";
    while(cell.getNextRef(esm, ref))
    {
        if (save) {
            info.data.mCellRefs[&cell].push_back(ref);
        }

        if(quiet) continue;

        std::cout << "    Refnum: " << ref.mRefnum << std::endl;
        std::cout << "    ID: '" << ref.mRefID << "'\n";
        std::cout << "    Owner: '" << ref.mOwner << "'\n";
        std::cout << "    Uses/health: " << ref.mCharge << "   NAM9: " << ref.mNam9 << std::endl;
    }
}

void printRaw(ESM::ESMReader &esm)
{
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        std::cout << "Record: " << n.toString() << std::endl;
        esm.getRecHeader();
        while(esm.hasMoreSubs())
        {
            uint64_t offs = esm.getOffset();
            esm.getSubName();
            esm.skipHSub();
            n = esm.retSubName();
            std::cout << "    " << n.toString() << " - " << esm.getSubSize()
                 << " bytes @ 0x" << std::hex << offs << "\n";
        }
    }
}

int load(Arguments& info)
{
    ESM::ESMReader& esm = info.reader;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);

    std::string filename = info.filename;
    std::cout << "Loading file: " << filename << std::endl;

    std::list<int> skipped;

    try {

        if(info.raw_given && info.mode == "dump")
        {
            std::cout << "RAW file listing:\n";

            esm.openRaw(filename);

            printRaw(esm);

            return 0;
        }

        bool quiet = (info.quiet_given || info.mode == "clone");
        bool loadCells = (info.loadcells_given || info.mode == "clone");
        bool save = (info.mode == "clone");

        esm.open(filename);

        info.data.author = esm.getAuthor();
        info.data.description = esm.getDesc();
        info.data.masters = esm.getMasters();

        if (!quiet)
        {
            std::cout << "Author: " << esm.getAuthor() << std::endl
                 << "Description: " << esm.getDesc() << std::endl
                 << "File format version: " << esm.getFVer() << std::endl;
            std::vector<ESM::Header::MasterData> m = esm.getMasters();
            if (!m.empty())
            {
                std::cout << "Masters:" << std::endl;
                for(unsigned int i=0;i<m.size();i++)
                    std::cout << "  " << m[i].name << ", " << m[i].size << " bytes" << std::endl;
            }
        }

        // Loop through all records
        while(esm.hasMoreRecs())
        {
            ESM::NAME n = esm.getRecName();
            uint32_t flags;
            esm.getRecHeader(flags);

            // Is the user interested in this record type?
            bool interested = true;
            if (info.types.size() > 0)
            {
                std::vector<std::string>::iterator match;
                match = std::find(info.types.begin(), info.types.end(),
                                  n.toString());
                if (match == info.types.end()) interested = false;
            }

            std::string id = esm.getHNOString("NAME");

            if(!quiet && interested)
                std::cout << "\nRecord: " << n.toString()
                     << " '" << id << "'\n";

            EsmTool::RecordBase *record = EsmTool::RecordBase::create(n);

            if (record == 0) {
                if (std::find(skipped.begin(), skipped.end(), n.val) == skipped.end())
                {
                    std::cout << "Skipping " << n.toString() << " records." << std::endl;
                    skipped.push_back(n.val);
                }

                esm.skipRecord();
                if (quiet) break;
                std::cout << "  Skipping\n";
            } else {
                if (record->getType().val == ESM::REC_GMST) {
                    // preset id for GameSetting record
                    record->cast<ESM::GameSetting>()->get().mId = id;
                }
                record->setId(id);
                record->setFlags((int) flags);
                record->load(esm);
                if (!quiet && interested) record->print();

                if (record->getType().val == ESM::REC_CELL && loadCells) {
                    loadCell(record->cast<ESM::Cell>()->get(), esm, info);
                }

                if (save) {
                    info.data.mRecords.push_back(record);
                } else {
                    delete record;
                }
                ++info.data.mRecordStats[n.val];
            }
        }

    } catch(std::exception &e) {
        std::cout << "\nERROR:\n\n  " << e.what() << std::endl;

        typedef std::deque<EsmTool::RecordBase *> RecStore;
        RecStore &store = info.data.mRecords;
        for (RecStore::iterator it = store.begin(); it != store.end(); ++it)
        {
            delete *it;
        }
        store.clear();
        return 1;
    }

    return 0;
}

#include <iomanip>

int clone(Arguments& info)
{
    if (info.outname.empty())
    {
        std::cout << "You need to specify an output name" << std::endl;
        return 1;
    }

    if (load(info) != 0)
    {
        std::cout << "Failed to load, aborting." << std::endl;
        return 1;
    }

    int recordCount = info.data.mRecords.size();

    int digitCount = 1; // For a nicer output
    if (recordCount > 9) ++digitCount;
    if (recordCount > 99) ++digitCount;
    if (recordCount > 999) ++digitCount;
    if (recordCount > 9999) ++digitCount;
    if (recordCount > 99999) ++digitCount;
    if (recordCount > 999999) ++digitCount;

    std::cout << "Loaded " << recordCount << " records:" << std::endl << std::endl;

    ESM::NAME name;

    int i = 0;
    typedef std::map<int, int> Stats;
    Stats &stats = info.data.mRecordStats;
    for (Stats::iterator it = stats.begin(); it != stats.end(); ++it)
    {
        name.val = it->first;
        float amount = it->second;
        std::cout << std::setw(digitCount) << amount << " " << name.toString() << "  ";

        if (++i % 3 == 0)
            std::cout << std::endl;
    }

    if (i % 3 != 0)
        std::cout << std::endl;

    std::cout << std::endl << "Saving records to: " << info.outname << "..." << std::endl;

    ESM::ESMWriter& esm = info.writer;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);
    esm.setAuthor(info.data.author);
    esm.setDescription(info.data.description);
    esm.setVersion(info.data.version);
    esm.setRecordCount (recordCount);

    for (std::vector<ESM::Header::MasterData>::iterator it = info.data.masters.begin(); it != info.data.masters.end(); ++it)
        esm.addMaster(it->name, it->size);

    std::fstream save(info.outname.c_str(), std::fstream::out | std::fstream::binary);
    esm.save(save);

    int saved = 0;
    typedef std::deque<EsmTool::RecordBase *> Records;
    Records &records = info.data.mRecords;
    for (Records::iterator it = records.begin(); it != records.end() && i > 0; ++it)
    {
        EsmTool::RecordBase *record = *it;

        name.val = record->getType().val;

        esm.startRecord(name.toString(), record->getFlags());

        // TODO wrap this with std::set
        if (ESMData::sLabeledRec.count(name.val) > 0) {
            esm.writeHNCString("NAME", record->getId());
        } else {
            esm.writeHNOString("NAME", record->getId());
        }

        record->save(esm);

        if (name.val == ESM::REC_CELL) {
            ESM::Cell *ptr = &record->cast<ESM::Cell>()->get();
            if (!info.data.mCellRefs[ptr].empty()) {
                typedef std::deque<ESM::CellRef> RefList;
                RefList &refs = info.data.mCellRefs[ptr];
                for (RefList::iterator it = refs.begin(); it != refs.end(); ++it)
                {
                    it->save(esm);
                }
            }
        }

        esm.endRecord(name.toString());

        saved++;
        int perc = (saved / (float)recordCount)*100;
        if (perc % 10 == 0)
        {
            std::cerr << "\r" << perc << "%";
        }
    }

    std::cout << "\rDone!" << std::endl;

    esm.close();
    save.close();

    return 0;
}

int comp(Arguments& info)
{
    if (info.filename.empty() || info.outname.empty())
    {
        std::cout << "You need to specify two input files" << std::endl;
        return 1;
    }

    Arguments fileOne;
    Arguments fileTwo;

    fileOne.raw_given = 0;
    fileTwo.raw_given = 0;

    fileOne.mode = "clone";
    fileTwo.mode = "clone";

    fileOne.encoding = info.encoding;
    fileTwo.encoding = info.encoding;

    fileOne.filename = info.filename;
    fileTwo.filename = info.outname;

    if (load(fileOne) != 0)
    {
        std::cout << "Failed to load " << info.filename << ", aborting comparison." << std::endl;
        return 1;
    }

    if (load(fileTwo) != 0)
    {
        std::cout << "Failed to load " << info.outname << ", aborting comparison." << std::endl;
        return 1;
    }

    if (fileOne.data.mRecords.size() != fileTwo.data.mRecords.size())
    {
        std::cout << "Not equal, different amount of records." << std::endl;
        return 1;
    }




    return 0;
}
