#ifndef TESTING_UTIL_H
#define TESTING_UTIL_H

#include <filesystem>
#include <sstream>

#include <components/misc/strings/conversion.hpp>
#include <components/vfs/archive.hpp>
#include <components/vfs/file.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

namespace TestingOpenMW
{

    inline std::filesystem::path outputFilePath(const std::string name)
    {
        std::filesystem::path dir("tests_output");
        std::filesystem::create_directory(dir);
        return dir / Misc::StringUtils::stringToU8String(name);
    }

    inline std::filesystem::path outputFilePathWithSubDir(const std::filesystem::path& subpath)
    {
        std::filesystem::path path("tests_output");
        path /= subpath;
        std::filesystem::create_directories(path.parent_path());
        return path;
    }

    inline std::filesystem::path temporaryFilePath(const std::string name)
    {
        return std::filesystem::temp_directory_path() / name;
    }

    class VFSTestFile : public VFS::File
    {
    public:
        explicit VFSTestFile(std::string content)
            : mContent(std::move(content))
        {
        }

        Files::IStreamPtr open() override { return std::make_unique<std::stringstream>(mContent, std::ios_base::in); }

        std::filesystem::path getPath() override { return "TestFile"; }

    private:
        const std::string mContent;
    };

    struct VFSTestData : public VFS::Archive
    {
        std::map<std::string, VFS::File*, VFS::Path::PathLess> mFiles;

        VFSTestData(std::map<std::string, VFS::File*, VFS::Path::PathLess> files)
            : mFiles(std::move(files))
        {
        }

        void listResources(VFS::FileMap& out) override
        {
            for (const auto& [key, value] : mFiles)
                out.emplace(key, value);
        }

        bool contains(std::string_view file) const override { return mFiles.contains(file); }

        std::string getDescription() const override { return "TestData"; }
    };

    inline std::unique_ptr<VFS::Manager> createTestVFS(std::map<std::string, VFS::File*, VFS::Path::PathLess> files)
    {
        auto vfs = std::make_unique<VFS::Manager>();
        vfs->addArchive(std::make_unique<VFSTestData>(std::move(files)));
        vfs->buildIndex();
        return vfs;
    }

#define EXPECT_ERROR(X, ERR_SUBSTR)                                                                                    \
    try                                                                                                                \
    {                                                                                                                  \
        X;                                                                                                             \
        FAIL() << "Expected error";                                                                                    \
    }                                                                                                                  \
    catch (std::exception & e)                                                                                         \
    {                                                                                                                  \
        EXPECT_THAT(e.what(), ::testing::HasSubstr(ERR_SUBSTR));                                                       \
    }

}

#endif // TESTING_UTIL_H
