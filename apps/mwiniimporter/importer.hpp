#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <exception>
#include <filesystem>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include <components/toutf8/toutf8.hpp>

class MwIniImporter
{
public:
    typedef std::map<std::string, std::string> strmap;
    typedef std::map<std::string, std::vector<std::string>> multistrmap;
    typedef std::vector<std::pair<std::string, std::vector<std::string>>> dependencyList;

    MwIniImporter();
    void setInputEncoding(const ToUTF8::FromType& encoding);
    void setVerbose(bool verbose);
    multistrmap loadIniFile(const std::filesystem::path& filename) const;
    static multistrmap loadCfgFile(const std::filesystem::path& filename);
    void merge(multistrmap& cfg, const multistrmap& ini) const;
    void mergeFallback(multistrmap& cfg, const multistrmap& ini) const;
    void importGameFiles(multistrmap& cfg, const multistrmap& ini, const std::filesystem::path& iniFilename) const;
    void importArchives(multistrmap& cfg, const multistrmap& ini) const;
    static void writeToFile(std::ostream& out, const multistrmap& cfg);

    static std::vector<std::string> dependencySort(MwIniImporter::dependencyList source);

private:
    static void dependencySortStep(
        std::string& element, MwIniImporter::dependencyList& source, std::vector<std::string>& result);
    static std::vector<std::string>::iterator findString(std::vector<std::string>& source, const std::string& string);

    static void insertMultistrmap(multistrmap& cfg, const std::string& key, const std::string& value);
    static void addPaths(std::vector<std::filesystem::path>& output, std::vector<std::string> input);

    /// \return file's "last modified time", used in original MW to determine plug-in load order
    static std::time_t lastWriteTime(const std::filesystem::path& filename, std::time_t defaultTime);

    bool mVerbose;
    strmap mMergeMap;
    std::vector<std::string> mMergeFallback;
    ToUTF8::FromType mEncoding;
};

#endif
