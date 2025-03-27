#ifndef OPENMW_COMPONENTS_TESTING_UTIL_H
#define OPENMW_COMPONENTS_TESTING_UTIL_H

#include <chrono>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <sstream>

#include <components/misc/strings/conversion.hpp>
#include <components/vfs/archive.hpp>
#include <components/vfs/file.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

namespace TestingOpenMW
{
    inline std::filesystem::path outputDir()
    {
        static const std::string run
            = std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        std::filesystem::path dir = std::filesystem::temp_directory_path() / "openmw" / "tests" / run;
        std::filesystem::create_directories(dir);
        return dir;
    }

    inline std::filesystem::path outputFilePath(std::string_view name)
    {
        std::filesystem::path dir = outputDir();
        return dir / Misc::StringUtils::stringToU8String(name);
    }

    inline std::filesystem::path outputDirPath(const std::filesystem::path& subpath)
    {
        std::filesystem::path path = outputDir();
        path /= subpath;
        std::filesystem::create_directories(path);
        return path;
    }

    inline std::filesystem::path outputFilePathWithSubDir(const std::filesystem::path& subpath)
    {
        std::filesystem::path path = outputDir();
        path /= subpath;
        std::filesystem::create_directories(path.parent_path());
        return path;
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
        VFS::FileMap mFiles;

        explicit VFSTestData(VFS::FileMap&& files)
            : mFiles(std::move(files))
        {
        }

        void listResources(VFS::FileMap& out) override { out = mFiles; }

        bool contains(VFS::Path::NormalizedView file) const override { return mFiles.contains(file); }

        std::string getDescription() const override { return "TestData"; }
    };

    inline std::unique_ptr<VFS::Manager> createTestVFS(VFS::FileMap&& files)
    {
        auto vfs = std::make_unique<VFS::Manager>();
        vfs->addArchive(std::make_unique<VFSTestData>(std::move(files)));
        vfs->buildIndex();
        return vfs;
    }

    inline std::unique_ptr<VFS::Manager> createTestVFS(
        std::initializer_list<std::pair<VFS::Path::NormalizedView, VFS::File*>> files)
    {
        return createTestVFS(VFS::FileMap(files.begin(), files.end()));
    }
}

#endif
