#include <gtest/gtest.h>

#include <boost/filesystem/fstream.hpp>

#include <components/files/configurationmanager.hpp>
#include <components/esm/esmreader.hpp>
#include <components/loadinglistener/loadinglistener.hpp>

#include "apps/openmw/mwworld/esmstore.hpp"

/// Base class for tests of ESMStore that rely on external content files to produce the test data
struct ContentFileTest : public ::testing::Test
{
  protected:

    virtual void SetUp()
    {
        readContentFiles();

        // load the content files
        std::vector<ESM::ESMReader> readerList;
        readerList.resize(mContentFiles.size());

        int index=0;
        for (std::vector<boost::filesystem::path>::const_iterator it = mContentFiles.begin(); it != mContentFiles.end(); ++it)
        {
            ESM::ESMReader lEsm;
            lEsm.setEncoder(NULL);
            lEsm.setIndex(index);
            lEsm.setGlobalReaderList(&readerList);
            lEsm.open(it->string());
            readerList[index] = lEsm;
            std::auto_ptr<Loading::Listener> listener;
            listener.reset(new Loading::Listener);
            mEsmStore.load(readerList[index], listener.get());

            ++index;
        }

        mEsmStore.setUp();
    }

    virtual void TearDown()
    {
    }

    // read absolute path to content files from openmw.cfg
    void readContentFiles()
    {
        boost::program_options::variables_map variables;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
        ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken()->composing())
        ("content", boost::program_options::value<std::vector<std::string> >()->default_value(std::vector<std::string>(), "")
            ->multitoken(), "content file(s): esm/esp, or omwgame/omwaddon")
        ("data-local", boost::program_options::value<std::string>()->default_value(""));

        boost::program_options::notify(variables);

        mConfigurationManager.readConfiguration(variables, desc, true);

        Files::PathContainer dataDirs, dataLocal;
        if (!variables["data"].empty()) {
            dataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
        }

        std::string local = variables["data-local"].as<std::string>();
        if (!local.empty()) {
            dataLocal.push_back(Files::PathContainer::value_type(local));
        }

        mConfigurationManager.processPaths (dataDirs);
        mConfigurationManager.processPaths (dataLocal, true);

        if (!dataLocal.empty())
            dataDirs.insert (dataDirs.end(), dataLocal.begin(), dataLocal.end());

        Files::Collections collections (dataDirs, true);

        std::vector<std::string> contentFiles = variables["content"].as<std::vector<std::string> >();
        for (std::vector<std::string>::iterator it = contentFiles.begin(); it != contentFiles.end(); ++it)
            mContentFiles.push_back(collections.getPath(*it));
    }

protected:
    Files::ConfigurationManager mConfigurationManager;
    MWWorld::ESMStore mEsmStore;
    std::vector<boost::filesystem::path> mContentFiles;
};

/// Print results of the dialogue merging process, i.e. the resulting linked list.
TEST_F(ContentFileTest, dialogue_merging_test)
{
    if (mContentFiles.empty())
    {
        std::cout << "No content files found, skipping test" << std::endl;
        return;
    }

    const std::string file = "test_dialogue_merging.txt";

    boost::filesystem::ofstream stream;
    stream.open(file);

    const MWWorld::Store<ESM::Dialogue>& dialStore = mEsmStore.get<ESM::Dialogue>();
    for (MWWorld::Store<ESM::Dialogue>::iterator it = dialStore.begin(); it != dialStore.end(); ++it)
    {
        const ESM::Dialogue& dial = *it;
        stream << "Dialogue: " << dial.mId << std::endl;

        for (ESM::Dialogue::InfoContainer::const_iterator infoIt = dial.mInfo.begin(); infoIt != dial.mInfo.end(); ++infoIt)
        {
            const ESM::DialInfo& info = *infoIt;
            stream << info.mId << std::endl;
        }
        stream << std::endl;
    }

    std::cout << "dialogue_merging_test successful, results printed to " << file << std::endl;
}

// Note: here we don't test records that don't use string names (e.g. Land, Pathgrid, Cell)
#define RUN_TEST_FOR_TYPES(func, arg1, arg2) \
    func<ESM::Activator>(arg1, arg2); \
    func<ESM::Apparatus>(arg1, arg2); \
    func<ESM::Armor>(arg1, arg2); \
    func<ESM::BirthSign>(arg1, arg2); \
    func<ESM::BodyPart>(arg1, arg2); \
    func<ESM::Book>(arg1, arg2); \
    func<ESM::Class>(arg1, arg2); \
    func<ESM::Clothing>(arg1, arg2); \
    func<ESM::Container>(arg1, arg2); \
    func<ESM::Creature>(arg1, arg2); \
    func<ESM::CreatureLevList>(arg1, arg2); \
    func<ESM::Dialogue>(arg1, arg2); \
    func<ESM::Door>(arg1, arg2); \
    func<ESM::Enchantment>(arg1, arg2); \
    func<ESM::Faction>(arg1, arg2); \
    func<ESM::GameSetting>(arg1, arg2); \
    func<ESM::Global>(arg1, arg2); \
    func<ESM::Ingredient>(arg1, arg2); \
    func<ESM::ItemLevList>(arg1, arg2); \
    func<ESM::Light>(arg1, arg2); \
    func<ESM::Lockpick>(arg1, arg2); \
    func<ESM::Miscellaneous>(arg1, arg2); \
    func<ESM::NPC>(arg1, arg2); \
    func<ESM::Potion>(arg1, arg2); \
    func<ESM::Probe>(arg1, arg2); \
    func<ESM::Race>(arg1, arg2); \
    func<ESM::Region>(arg1, arg2); \
    func<ESM::Repair>(arg1, arg2); \
    func<ESM::Script>(arg1, arg2); \
    func<ESM::Sound>(arg1, arg2); \
    func<ESM::SoundGenerator>(arg1, arg2); \
    func<ESM::Spell>(arg1, arg2); \
    func<ESM::StartScript>(arg1, arg2); \
    func<ESM::Weapon>(arg1, arg2);

template <typename T>
void printRecords(MWWorld::ESMStore& esmStore, std::ostream& outStream)
{
    const MWWorld::Store<T>& store = esmStore.get<T>();
    outStream << store.getSize() << " " << T::getRecordType() << " records" << std::endl;

    for (typename MWWorld::Store<T>::iterator it = store.begin(); it != store.end(); ++it)
    {
        const T& record = *it;
        outStream << record.mId << std::endl;
    }

    outStream << std::endl;
}

/// Print some basic diagnostics about the loaded content files, e.g. number of records and names of those records
/// Also used to test the iteration order of records
TEST_F(ContentFileTest, content_diagnostics_test)
{
    if (mContentFiles.empty())
    {
        std::cout << "No content files found, skipping test" << std::endl;
        return;
    }

    const std::string file = "test_content_diagnostics.txt";

    boost::filesystem::ofstream stream;
    stream.open(file);

    RUN_TEST_FOR_TYPES(printRecords, mEsmStore, stream);

    std::cout << "diagnostics_test successful, results printed to " << file << std::endl;
}

// TODO:
/// Print results of autocalculated NPC spell lists. Also serves as test for attribute/skill autocalculation which the spell autocalculation heavily relies on
/// - even rounding errors can completely change the resulting spell lists.
/*
TEST_F(ContentFileTest, autocalc_test)
{
    if (mContentFiles.empty())
    {
        std::cout << "No content files found, skipping test" << std::endl;
        return;
    }


}
*/

/*
/// Base class for tests of ESMStore that do not rely on external content files
struct StoreTest : public ::testing::Test
{
protected:
    MWWorld::ESMStore mEsmStore;
};

/// Tests deletion of records.
TEST_F(StoreTest, delete_test)
{


}

/// Tests overwriting of records.
TEST_F(StoreTest, overwrite_test)
{

}
*/
