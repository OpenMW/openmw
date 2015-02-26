#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iosfwd>
#include <boost/filesystem/path.hpp>

#include <components/to_utf8/to_utf8.hpp>

class MwIniImporter {
  public:
    typedef std::map<std::string, std::string> strmap;
    typedef std::map<std::string, std::vector<std::string> > multistrmap;

    MwIniImporter();
    void    setInputEncoding(const ToUTF8::FromType& encoding);
    void    setVerbose(bool verbose);
    multistrmap  loadIniFile(const boost::filesystem::path& filename) const;
    static multistrmap  loadCfgFile(const boost::filesystem::path& filename);
    void    merge(multistrmap &cfg, const multistrmap &ini) const;
    void    mergeFallback(multistrmap &cfg, const multistrmap &ini) const;
    void    importGameFiles(multistrmap &cfg, const multistrmap &ini, 
        const boost::filesystem::path& iniFilename) const;
    void    importArchives(multistrmap &cfg, const multistrmap &ini) const;
    static void    writeToFile(std::ostream &out, const multistrmap &cfg);

  private:
    static void insertMultistrmap(multistrmap &cfg, const std::string& key, const std::string& value);
    static std::string numberToString(int n);

    /// \return file's "last modified time", used in original MW to determine plug-in load order
    static std::time_t lastWriteTime(const boost::filesystem::path& filename, std::time_t defaultTime);

    bool mVerbose;
    strmap mMergeMap;
    std::vector<std::string> mMergeFallback;
    ToUTF8::FromType mEncoding;
};


#endif
