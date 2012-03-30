#ifndef MWINIIMPORTER_IMPORTER
#define MWINIIMPORTER_IMPORTER 1

#include <string>
#include <map>
#include <exception>

typedef std::map<std::string, std::string> strmap;

class IniParseException : public std::exception {
    virtual const char* what() const throw() {
        return "unexpected end of line";
    }
};

class MwIniImporter {

  public:
    void    setVerbose(bool verbose);
    strmap  loadIniFile(std::string filename);
    strmap  loadCfgFile(std::string filename);
    void    merge(strmap &cfg, strmap &ini);
    void    writeToFile(std::string file, strmap &cfg);
    
  private:
    bool mVerbose;

};


#endif
