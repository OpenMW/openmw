#include "importer.hpp"
#include <boost/shared_ptr.hpp>

#include <OgreRoot.h>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/defs.hpp>

#include <components/esm/savedgame.hpp>
#include <components/esm/player.hpp>

#include <components/esm/loadalch.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadench.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadlevlist.hpp>
#include <components/esm/loadglob.hpp>

#include <components/to_utf8/to_utf8.hpp>

#include "importercontext.hpp"

#include "converter.hpp"

namespace
{

    void writeScreenshot(const ESM::Header& fileHeader, ESM::SavedGame& out)
    {
        Ogre::Image screenshot;
        std::vector<unsigned char> screenshotData = fileHeader.mSCRS; // MemoryDataStream doesn't work with const data :(
        Ogre::DataStreamPtr screenshotStream (new Ogre::MemoryDataStream(&screenshotData[0], screenshotData.size()));
        screenshot.loadRawData(screenshotStream, 128, 128, 1, Ogre::PF_BYTE_BGRA);
        Ogre::DataStreamPtr encoded = screenshot.encode("jpg");
        out.mScreenshot.resize(encoded->size());
        encoded->read(&out.mScreenshot[0], encoded->size());
    }

}

namespace ESSImport
{

    Importer::Importer(const std::string &essfile, const std::string &outfile, const std::string &encoding)
        : mEssFile(essfile)
        , mOutFile(outfile)
        , mEncoding(encoding)
    {

    }

    struct File
    {
        struct Subrecord
        {
            std::string mName;
            size_t mFileOffset;
            std::vector<unsigned char> mData;
        };

        struct Record
        {
            std::string mName;
            size_t mFileOffset;
            std::vector<Subrecord> mSubrecords;
        };

        std::vector<Record> mRecords;
    };

    void read(const std::string& filename, File& file)
    {
        ESM::ESMReader esm;
        esm.open(filename);

        while (esm.hasMoreRecs())
        {
            ESM::NAME n = esm.getRecName();
            esm.getRecHeader();

            File::Record rec;
            rec.mName = n.toString();
            rec.mFileOffset = esm.getFileOffset();
            while (esm.hasMoreSubs())
            {
                File::Subrecord sub;
                esm.getSubName();
                esm.getSubHeader();
                sub.mFileOffset = esm.getFileOffset();
                sub.mName = esm.retSubName().toString();
                sub.mData.resize(esm.getSubSize());
                esm.getExact(&sub.mData[0], sub.mData.size());
                rec.mSubrecords.push_back(sub);
            }
            file.mRecords.push_back(rec);
        }
    }

    void Importer::compare()
    {
        // data that always changes (and/or is already fully decoded) should be blacklisted
        std::set<std::pair<std::string, std::string> > blacklist;
        blacklist.insert(std::make_pair("GLOB", "FLTV")); // gamehour
        blacklist.insert(std::make_pair("REFR", "DATA")); // player position
        blacklist.insert(std::make_pair("CELL", "NAM8")); // fog of war
        blacklist.insert(std::make_pair("GAME", "GMDT")); // weather data, current time always changes
        blacklist.insert(std::make_pair("CELL", "DELE")); // first 3 bytes are uninitialized

         // this changes way too often, name suggests some renderer internal data?
        blacklist.insert(std::make_pair("CELL", "ND3D"));
        blacklist.insert(std::make_pair("REFR", "ND3D"));

        File file1;
        read(mEssFile, file1);
        File file2;
        read(mOutFile, file2); // todo rename variable

        // FIXME: use max(size1, size2)
        for (unsigned int i=0; i<file1.mRecords.size(); ++i)
        {
            File::Record rec = file1.mRecords[i];

            if (i >= file2.mRecords.size())
            {
                std::cout << "Record in file1 not present in file2: (1) 0x" << std::hex << rec.mFileOffset << std::endl;
                return;
            }

            File::Record rec2 = file2.mRecords[i];

            if (rec.mName != rec2.mName)
            {
                std::cout << "Different record name at (2) 0x" << std::hex << rec2.mFileOffset << std::endl;
                return; // TODO: try to recover
            }

            // FIXME: use max(size1, size2)
            for (unsigned int j=0; j<rec.mSubrecords.size(); ++j)
            {
                File::Subrecord sub = rec.mSubrecords[j];

                if (j >= rec2.mSubrecords.size())
                {
                    std::cout << "Subrecord in file1 not present in file2: (1) 0x" << std::hex << sub.mFileOffset << std::endl;
                    return;
                }

                File::Subrecord sub2 = rec2.mSubrecords[j];

                if (sub.mName != sub2.mName)
                {
                    std::cout << "Different subrecord name (" << rec.mName << "." << sub.mName << " vs. " << sub2.mName << ") at (1) 0x" << std::hex << sub.mFileOffset
                              << " (2) 0x" << sub2.mFileOffset << std::endl;
                    break; // TODO: try to recover
                }

                if (sub.mData != sub2.mData)
                {
                    if (blacklist.find(std::make_pair(rec.mName, sub.mName)) != blacklist.end())
                        continue;

                    std::cout << "Different subrecord data for " << rec.mName << "." << sub.mName << " at (1) 0x" << std::hex << sub.mFileOffset
                              << " (2) 0x" << sub2.mFileOffset << std::endl;

                    std::cout << "Data 1:" << std::endl;
                    for (unsigned int k=0; k<sub.mData.size(); ++k)
                    {
                        bool different = false;
                        if (k >= sub2.mData.size() || sub2.mData[k] != sub.mData[k])
                            different = true;

                        if (different)
                            std::cout << "\033[033m";
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)sub.mData[k] << " ";
                        if (different)
                            std::cout << "\033[0m";
                    }
                    std::cout << std::endl;

                    std::cout << "Data 2:" << std::endl;
                    for (unsigned int k=0; k<sub2.mData.size(); ++k)
                    {
                        bool different = false;
                        if (k >= sub.mData.size() || sub.mData[k] != sub2.mData[k])
                            different = true;

                        if (different)
                            std::cout << "\033[033m";
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)sub2.mData[k] << " ";
                        if (different)
                            std::cout << "\033[0m";
                    }
                    std::cout << std::endl;
                }
            }
        }
    }

    void Importer::run()
    {
        // construct Ogre::Root to gain access to image codecs
        Ogre::LogManager logman;
        Ogre::Root root;

        ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(mEncoding));
        ESM::ESMReader esm;
        esm.open(mEssFile);
        esm.setEncoder(&encoder);

        Context context;

        const ESM::Header& header = esm.getHeader();
        context.mPlayerCellName = header.mGameData.mCurrentCell.toString();

        const unsigned int recREFR = ESM::FourCC<'R','E','F','R'>::value;
        const unsigned int recPCDT = ESM::FourCC<'P','C','D','T'>::value;
        const unsigned int recFMAP = ESM::FourCC<'F','M','A','P'>::value;
        const unsigned int recKLST = ESM::FourCC<'K','L','S','T'>::value;
        const unsigned int recSTLN = ESM::FourCC<'S','T','L','N'>::value;
        const unsigned int recGAME = ESM::FourCC<'G','A','M','E'>::value;
        const unsigned int recJOUR = ESM::FourCC<'J','O','U','R'>::value;

        std::map<unsigned int, boost::shared_ptr<Converter> > converters;
        converters[ESM::REC_GLOB] = boost::shared_ptr<Converter>(new ConvertGlobal());
        converters[ESM::REC_BOOK] = boost::shared_ptr<Converter>(new ConvertBook());
        converters[ESM::REC_NPC_] = boost::shared_ptr<Converter>(new ConvertNPC());
        converters[ESM::REC_CREA] = boost::shared_ptr<Converter>(new ConvertCREA());
        converters[ESM::REC_NPCC] = boost::shared_ptr<Converter>(new ConvertNPCC());
        converters[ESM::REC_CREC] = boost::shared_ptr<Converter>(new ConvertCREC());
        converters[recREFR      ] = boost::shared_ptr<Converter>(new ConvertREFR());
        converters[recPCDT      ] = boost::shared_ptr<Converter>(new ConvertPCDT());
        converters[recFMAP      ] = boost::shared_ptr<Converter>(new ConvertFMAP());
        converters[recKLST      ] = boost::shared_ptr<Converter>(new ConvertKLST());
        converters[recSTLN      ] = boost::shared_ptr<Converter>(new ConvertSTLN());
        converters[recGAME      ] = boost::shared_ptr<Converter>(new ConvertGAME());
        converters[ESM::REC_CELL] = boost::shared_ptr<Converter>(new ConvertCell());
        converters[ESM::REC_ALCH] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Potion>());
        converters[ESM::REC_CLAS] = boost::shared_ptr<Converter>(new ConvertClass());
        converters[ESM::REC_SPEL] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Spell>());
        converters[ESM::REC_ARMO] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Armor>());
        converters[ESM::REC_WEAP] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Weapon>());
        converters[ESM::REC_CLOT] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Clothing>());
        converters[ESM::REC_ENCH] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Enchantment>());
        converters[ESM::REC_WEAP] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::Weapon>());
        converters[ESM::REC_LEVC] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::CreatureLevList>());
        converters[ESM::REC_LEVI] = boost::shared_ptr<Converter>(new DefaultConverter<ESM::ItemLevList>());
        converters[ESM::REC_CNTC] = boost::shared_ptr<Converter>(new ConvertCNTC());
        converters[ESM::REC_FACT] = boost::shared_ptr<Converter>(new ConvertFACT());
        converters[ESM::REC_INFO] = boost::shared_ptr<Converter>(new ConvertINFO());
        converters[ESM::REC_DIAL] = boost::shared_ptr<Converter>(new ConvertDIAL());
        converters[ESM::REC_QUES] = boost::shared_ptr<Converter>(new ConvertQUES());
        converters[recJOUR      ] = boost::shared_ptr<Converter>(new ConvertJOUR());
        converters[ESM::REC_SCPT] = boost::shared_ptr<Converter>(new ConvertSCPT());

        // TODO:
        // - REGN (weather in certain regions?)
        // - VFXM
        // - SPLM (active spell effects)
        // - PROJ (magic projectiles in air)

        std::set<unsigned int> unknownRecords;

        for (std::map<unsigned int, boost::shared_ptr<Converter> >::const_iterator it = converters.begin();
             it != converters.end(); ++it)
        {
            it->second->setContext(context);
        }

        while (esm.hasMoreRecs())
        {
            ESM::NAME n = esm.getRecName();
            esm.getRecHeader();

            std::map<unsigned int, boost::shared_ptr<Converter> >::iterator it = converters.find(n.val);
            if (it != converters.end())
            {
                it->second->read(esm);
            }
            else
            {
                if (unknownRecords.insert(n.val).second)
                    std::cerr << "unknown record " << n.toString() << " (0x" << std::hex << esm.getFileOffset() << ")" << std::endl;

                esm.skipRecord();
            }
        }

        ESM::ESMWriter writer;

        writer.setFormat (ESM::Header::CurrentFormat);

        std::ofstream stream(mOutFile.c_str(), std::ios::binary);
        // all unused
        writer.setVersion(0);
        writer.setType(0);
        writer.setAuthor("");
        writer.setDescription("");
        writer.setRecordCount (0);

        for (std::vector<ESM::Header::MasterData>::const_iterator it = header.mMaster.begin();
             it != header.mMaster.end(); ++it)
            writer.addMaster (it->name, 0); // not using the size information anyway -> use value of 0

        writer.save (stream);

        ESM::SavedGame profile;
        for (std::vector<ESM::Header::MasterData>::const_iterator it = header.mMaster.begin();
             it != header.mMaster.end(); ++it)
        {
            profile.mContentFiles.push_back(it->name);
        }
        profile.mDescription = esm.getDesc();
        profile.mInGameTime.mDay = context.mDay;
        profile.mInGameTime.mGameHour = context.mHour;
        profile.mInGameTime.mMonth = context.mMonth;
        profile.mInGameTime.mYear = context.mYear;
        profile.mPlayerCell = header.mGameData.mCurrentCell.toString();
        if (context.mPlayerBase.mClass == "NEWCLASSID_CHARGEN")
            profile.mPlayerClassName = context.mCustomPlayerClassName;
        else
            profile.mPlayerClassId = context.mPlayerBase.mClass;
        profile.mPlayerLevel = context.mPlayerBase.mNpdt52.mLevel;
        profile.mPlayerName = header.mGameData.mPlayerName.toString();

        writeScreenshot(header, profile);

        writer.startRecord (ESM::REC_SAVE);
        profile.save (writer);
        writer.endRecord (ESM::REC_SAVE);

        // Writing order should be Dynamic Store -> Cells -> Player,
        // so that references to dynamic records can be recognized when loading
        for (std::map<unsigned int, boost::shared_ptr<Converter> >::const_iterator it = converters.begin();
             it != converters.end(); ++it)
        {
            if (it->second->getStage() != 0)
                continue;
            it->second->write(writer);
        }

        writer.startRecord(ESM::REC_NPC_);
        writer.writeHNString("NAME", "player");
        context.mPlayerBase.save(writer);
        writer.endRecord(ESM::REC_NPC_);

        for (std::map<unsigned int, boost::shared_ptr<Converter> >::const_iterator it = converters.begin();
             it != converters.end(); ++it)
        {
            if (it->second->getStage() != 1)
                continue;
            it->second->write(writer);
        }

        writer.startRecord(ESM::REC_PLAY);
        if (context.mPlayer.mCellId.mPaged)
        {
            // exterior cell -> determine cell coordinates based on position
            const int cellSize = 8192;
            int cellX = static_cast<int>(std::floor(context.mPlayer.mObject.mPosition.pos[0]/cellSize));
            int cellY = static_cast<int>(std::floor(context.mPlayer.mObject.mPosition.pos[1] / cellSize));
            context.mPlayer.mCellId.mIndex.mX = cellX;
            context.mPlayer.mCellId.mIndex.mY = cellY;
        }
        context.mPlayer.save(writer);
        writer.endRecord(ESM::REC_PLAY);

        writer.startRecord (ESM::REC_DIAS);
        context.mDialogueState.save(writer);
        writer.endRecord(ESM::REC_DIAS);
    }


}
