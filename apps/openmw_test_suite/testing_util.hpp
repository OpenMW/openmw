#ifndef TESTING_UTIL_H
#define TESTING_UTIL_H

#include <filesystem>
#include <sstream>

#include <components/vfs/archive.hpp>
#include <components/vfs/manager.hpp>

namespace TestingOpenMW
{

    inline std::string outputFilePath(const std::string name)
    {
        std::filesystem::path dir("tests_output");
        std::filesystem::create_directory(dir);
        return (dir / name).string();
    }

    inline std::string temporaryFilePath(const std::string name)
    {
        return (std::filesystem::temp_directory_path() / name).string();
    }

    class VFSTestFile : public VFS::File
    {
    public:
        explicit VFSTestFile(std::string content) : mContent(std::move(content)) {}

        Files::IStreamPtr open() override
        {
            return std::make_unique<std::stringstream>(mContent, std::ios_base::in);
        }

        std::string getPath() override
        {
            return "TestFile";
        }

    private:
        const std::string mContent;
    };

    struct VFSTestData : public VFS::Archive
    {
        std::map<std::string, VFS::File*> mFiles;

        VFSTestData(std::map<std::string, VFS::File*> files) : mFiles(std::move(files)) {}

        void listResources(std::map<std::string, VFS::File*>& out, char (*normalize_function) (char)) override
        {
            out = mFiles;
        }

        bool contains(const std::string& file, char (*normalize_function) (char)) const override
        {
            return mFiles.count(file) != 0;
        }

        std::string getDescription() const override { return "TestData"; }

    };

    inline std::unique_ptr<VFS::Manager> createTestVFS(std::map<std::string, VFS::File*> files)
    {
        auto vfs = std::make_unique<VFS::Manager>(true);
        vfs->addArchive(std::make_unique<VFSTestData>(std::move(files)));
        vfs->buildIndex();
        return vfs;
    }

    #define EXPECT_ERROR(X, ERR_SUBSTR) try { X; FAIL() << "Expected error"; } \
        catch (std::exception& e) { EXPECT_THAT(e.what(), ::testing::HasSubstr(ERR_SUBSTR)); }

}

#endif // TESTING_UTIL_H
