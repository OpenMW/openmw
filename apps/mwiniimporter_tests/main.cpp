#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>


struct TestParam
{
    std::string name;
    std::string fileName;
};

const std::vector<TestParam> testParams = {
    { "ascii", "ascii" },
    { "unicode", "(╯°□°)╯︵ ┻━┻"},
    { "emoji", "💩"}
};

class IniImporterTest : public ::testing::TestWithParam<TestParam>
{
};


TEST_P(IniImporterTest, TestIniImport)
{
    auto param = IniImporterTest::GetParam();

    // Create temporary file
    std::string iniData = R"([Archives]
Archive 0=game1.bsa
Archive 1=game2.bsa
)";
    std::filesystem::path tempIniFile = std::filesystem::temp_directory_path() / param.fileName += "morrowind.ini";
    std::ofstream tempFile(tempIniFile);
    tempFile << iniData;
    tempFile.close();
    std::filesystem::path tempCfgFile = std::filesystem::temp_directory_path() / param.fileName += "openmw.cfg";

    std::stringstream cmd;
    cmd << "./openmw-iniimporter -i " << tempIniFile << " -c " << tempCfgFile;

    int ret = std::system(cmd.str().c_str());
    ASSERT_EQ(ret, 0);

    // Verify the cfg file was created and has the expected contents
    std::ifstream ifs(tempCfgFile);
    ASSERT_TRUE(ifs.good());

    std::string cfgData = R"(fallback-archive=Morrowind.bsa
fallback-archive=game1.bsa
fallback-archive=game2.bsa
)";

    std::stringstream actual;
    actual << ifs.rdbuf();

    ASSERT_EQ(cfgData, actual.str());

    // Clean up temporary file
    std::filesystem::remove(tempCfgFile);
    std::filesystem::remove(tempIniFile);
}

INSTANTIATE_TEST_SUITE_P(IniImporterTests, IniImporterTest, ::testing::ValuesIn(testParams));


int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}