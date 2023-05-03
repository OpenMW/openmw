#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

std::filesystem::path getExecutablePath()
{
#ifdef _WIN32
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exe_path(buffer);
    return std::filesystem::path(exe_path).parent_path();
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t bufsize = sizeof(buffer);
    _NSGetExecutablePath(buffer, &bufsize);
    return std::filesystem::path(buffer).parent_path();
#else
    char buffer[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1)
    {
        throw std::runtime_error("Could not retrieve executable path");
    }
    buffer[len] = '\0';
    std::string exe_path(buffer);
    return std::filesystem::path(exe_path).parent_path();
#endif
}

int runBinary(
    const std::filesystem::path& binaryPath, const std::filesystem::path& iniPath, const std::filesystem::path& cfgPath)
{
#ifdef _WIN32
    std::wstringstream cmd;
    cmd << L'"' << binaryPath.native() << L'"' << L" -i " << '"' << iniPath.native() << '"' << L" -c " << '"'
        << cfgPath.native() << '"';
    return _wsystem(cmd.str().c_str());
#else
    std::stringstream cmd;
    cmd << binaryPath << " -i "
        << "'" << iniPath << "'"
        << " -c "
        << "'" << cfgPath << "'";
    return std::system(cmd.str().c_str());
#endif
}

struct TestParam
{
    std::string name;
    std::string fileName;
};

const std::vector<TestParam> testParams
    = { { "ascii", "ascii" }, { "space", "spaaaaa ce" }, { "unicode", "(╯°□°)╯︵ ┻━┻" }, { "emoji", "🖕" } };

class IniImporterTest : public ::testing::TestWithParam<TestParam>
{
};

TEST_P(IniImporterTest, TestIniImport)
{
    auto const& param = IniImporterTest::GetParam();

    // Create temporary file
    std::string iniData = R"([Archives]
Archive 0=game1.bsa
Archive 1=game2.bsa
)";
    std::filesystem::path tempIniFile = std::filesystem::temp_directory_path() / (param.fileName + ".ini");
    std::ofstream tempFile(tempIniFile);
    tempFile << iniData;
    tempFile.close();
    std::filesystem::path tempCfgFile = std::filesystem::temp_directory_path() / (param.fileName + ".cfg");
    std::filesystem::path binaryPath = getExecutablePath() / "openmw-iniimporter";

    int ret = runBinary(binaryPath, tempIniFile, tempCfgFile);
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