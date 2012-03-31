#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <string>
#include <map>
#include <vector>
#include <exception>


typedef std::map<std::string, std::string> strmap;

class MwIniImporter {

  public:
    MwIniImporter();
    void    setVerbose(bool verbose);
    strmap  loadIniFile(std::string filename);
    strmap  loadCfgFile(std::string filename);
    void    merge(strmap &cfg, strmap &ini);
    void    importGameFiles(strmap &cfg, strmap &ini, std::vector<std::string> &esmFiles, std::vector<std::string> &espFiles);
    void    writeGameFiles(boost::iostreams::stream<boost::iostreams::file_sink> &out, std::vector<std::string> &esmFiles, std::vector<std::string> &espFiles);
    void    writeToFile(boost::iostreams::stream<boost::iostreams::file_sink> &out, strmap &cfg);
    
  private:
    bool   specialMerge(std::string cfgKey, std::string iniKey, strmap &cfg, strmap &ini);
    bool mVerbose;
    strmap mMergeMap;
};


#endif
