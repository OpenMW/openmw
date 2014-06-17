#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <string>
#include <map>
#include <vector>
#include <exception>
#include <iosfwd>

#include <components/to_utf8/to_utf8.hpp>

class MwIniImporter {
  public:
    typedef std::map<std::string, std::string> strmap;
    typedef std::map<std::string, std::vector<std::string> > multistrmap;

    MwIniImporter();
    void    setInputEncoding(const ToUTF8::FromType& encoding);
    void    setVerbose(bool verbose);
    multistrmap  loadIniFile(const std::string& filename) const;
    static multistrmap  loadCfgFile(const std::string& filename);
    void    merge(multistrmap &cfg, const multistrmap &ini) const;
    void    mergeFallback(multistrmap &cfg, const multistrmap &ini) const;
    void    importGameFiles(multistrmap &cfg, const multistrmap &ini) const;
    void    importArchives(multistrmap &cfg, const multistrmap &ini) const;
    static void    writeToFile(std::ostream &out, const multistrmap &cfg);

  private:
    static void insertMultistrmap(multistrmap &cfg, const std::string& key, const std::string& value);
    static std::string numberToString(int n);
    bool mVerbose;
    strmap mMergeMap;
    std::vector<std::string> mMergeFallback;
    ToUTF8::FromType mEncoding;
};


#endif
