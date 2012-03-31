#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <string>
#include <map>
#include <exception>


typedef std::map<std::string, std::string> strmap;
#define STRPAIR std::make_pair<std::string, std::string>

class MwIniImporter {

  public:
    MwIniImporter() {
        mMergeMap.insert(STRPAIR("fps", "General:Show FPS"));
    };
    void    setVerbose(bool verbose);
    strmap  loadIniFile(std::string filename);
    strmap  loadCfgFile(std::string filename);
    void    merge(strmap &cfg, strmap &ini);
    void    writeToFile(std::string file, strmap &cfg);
    
  private:
    bool   specialMerge(std::string cfgKey, std::string iniKey, strmap cfg, strmap ini);
    bool mVerbose;
    strmap mMergeMap;
};


#endif
