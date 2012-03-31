#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <string>
#include <map>
#include <vector>
#include <exception>

class MwIniImporter {
  public:
    typedef std::map<std::string, std::string> strmap;
    typedef std::map<std::string, std::vector<std::string> > multistrmap;

    MwIniImporter();
    void    setVerbose(bool verbose);
    multistrmap  loadIniFile(std::string filename);
    multistrmap  loadCfgFile(std::string filename);
    void    merge(multistrmap &cfg, multistrmap &ini);
    void    importGameFiles(multistrmap &cfg, multistrmap &ini);
    void    writeToFile(boost::iostreams::stream<boost::iostreams::file_sink> &out, multistrmap &cfg);
    
  private:
    bool specialMerge(std::string cfgKey, std::string iniKey, multistrmap &cfg, multistrmap &ini);
    bool mVerbose;
    strmap mMergeMap;
};


#endif
