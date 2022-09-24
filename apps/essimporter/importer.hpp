#ifndef OPENMW_ESSIMPORTER_IMPORTER_H
#define OPENMW_ESSIMPORTER_IMPORTER_H

#include <filesystem>

namespace ESSImport
{

    class Importer
    {
    public:
        Importer(
            const std::filesystem::path& essfile, const std::filesystem::path& outfile, const std::string& encoding);

        void run();

        void compare();

    private:
        std::filesystem::path mEssFile;
        std::filesystem::path mOutFile;
        std::string mEncoding;
    };

}

#endif
