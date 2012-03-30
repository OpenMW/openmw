#include "importer.hpp"
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>

void MwIniImporter::setVerbose(bool verbose) {
    mVerbose = verbose;
}

strmap MwIniImporter::loadIniFile(std::string filename) {
    std::cout << "load ini file: " << filename << std::endl;
    
    std::map<std::string, std::string> map;
    boost::iostreams::stream<boost::iostreams::file_source>file(filename.c_str());

    std::string line;
    while (std::getline(file, line)) {

        // ignore sections for now
        if(line.empty() || line[0] == ';' || line[0] == '[') {
            continue;
        }
        
        int pos = line.find("=");
        if(pos < 1) {
            throw IniParseException();
        }
        
        map.insert(std::pair<std::string,std::string>(
            line.substr(0,pos), line.substr(pos+1)
        ));
    }
    
    return map;
}

strmap MwIniImporter::loadCfgFile(std::string filename) {
    std::cout << "load cfg file: " << filename << std::endl;
    
    std::map<std::string, std::string> map;
    boost::iostreams::stream<boost::iostreams::file_source>file(filename.c_str());

    std::string line;
    while (std::getline(file, line)) {
        
        if(line[0] == '[') { // section
            continue; // ignore for now
        }
        
        // we cant say comment by only looking at first char anymore
        int comment_pos = line.find("#");
        if(comment_pos > 0) {
            line = line.substr(0,comment_pos);
        }
        
        if(line.empty()) {
            continue;
        }
        
        int pos = line.find("=");
        if(pos < 1) {
            throw IniParseException();
        }
        
        map.insert(std::pair<std::string,std::string>(
            line.substr(0,pos), line.substr(pos+1)
        ));
    }
    
    return map;
}

void MwIniImporter::merge(strmap &cfg, strmap &ini) {
    strmap::iterator ini_it;
    for(strmap::iterator it=cfg.begin(); it != cfg.end(); it++) {
        ini_it = ini.find(it->first);
        
        // found a key in both files
        if(ini_it != ini.end()) {
            cfg.erase(it);
            cfg.insert(std::pair<std::string,std::string>(
                ini_it->first, ini_it->second
            ));
        }
    }
}

void MwIniImporter::writeToFile(std::string file, strmap &cfg) {
    boost::iostreams::stream<boost::iostreams::file_sink> out(file);
    
    for(strmap::iterator it=cfg.begin(); it != cfg.end(); it++) {
        out << (it->first) << "=" << (it->second) << std::endl;
    }
}


