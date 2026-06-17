#ifndef COMPONENTS_FILES_MACOSPATH_H
#define COMPONENTS_FILES_MACOSPATH_H

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <filesystem>
#include <vector>

/**
 * \namespace Files
 */
namespace Files
{

    /**
     * \struct MacOsPath
     */
    struct MacOsPath
    {
        MacOsPath(const std::string& application_name);

        /**
         * \brief Return path to the local directory.
         *
         * \return std::filesystem::path
         */
        std::filesystem::path getUserConfigPath() const;

        std::filesystem::path getUserDataPath() const;

        /**
         * \brief Return path to the global (system) directory.
         *
         * \return std::filesystem::path
         */
        std::filesystem::path getGlobalConfigPath() const;

        /**
         * \brief Return path to the runtime directory which is the
         * place where an application was started.
         *
         * \return std::filesystem::path
         */
        std::filesystem::path getLocalPath() const;

        /**
         * \brief
         *
         * \return std::filesystem::path
         */
        std::filesystem::path getCachePath() const;

        /**
         * \brief
         *
         * \return std::filesystem::path
         */
        std::filesystem::path getGlobalDataPath() const;

        std::vector<std::filesystem::path> getInstallPaths() const;

        std::string mName;
    };

} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */

#endif /* COMPONENTS_FILES_MACOSPATH_H */
