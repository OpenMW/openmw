#include "importer.hpp"
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>

void MwIniImporter::setVerbose(bool verbose) {
    mVerbose = verbose;
}

strmap MwIniImporter::loadIniFile(std::string filename) {
    std::cout << "load ini file: " << filename << std::endl;
    
    std::string section("");
    std::map<std::string, std::string> map;
    boost::iostreams::stream<boost::iostreams::file_source>file(filename.c_str());

    std::string line;
    while (std::getline(file, line)) {

        // ignore sections for now
        if(line.empty() || line[0] == ';') {
            continue;
        }
        
        if(line[0] == '[') {
            if(line.length() > 2) {
                section = line.substr(1, line.length()-3);
            }
            continue;
        }
        
        int pos = line.find("=");
        if(pos < 1) {
            continue;
        }
        
        map.insert(STRPAIR(section + ":" + line.substr(0,pos), line.substr(pos+1)));
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
            continue;
        }
        
        map.insert(STRPAIR(line.substr(0,pos), line.substr(pos+1)));
    }
    
    return map;
}

void MwIniImporter::merge(strmap &cfg, strmap &ini) {
    strmap::iterator cfgIt;
    strmap::iterator iniIt;
    for(strmap::iterator it=mMergeMap.begin(); it!=mMergeMap.end(); it++) {
        if((iniIt = ini.find(it->second)) != ini.end()) {
            cfg.erase(it->first);
            if(!this->specialMerge(it->first, it->second, cfg, ini)) {
                cfg.insert(STRPAIR(it->first, iniIt->second));
            }
        }
    }
}

bool MwIniImporter::specialMerge(std::string cfgKey, std::string iniKey, strmap &cfg, strmap &ini) {
    return false;
}

void MwIniImporter::writeToFile(std::string file, strmap &cfg) {
    boost::iostreams::stream<boost::iostreams::file_sink> out(file);
    
    for(strmap::iterator it=cfg.begin(); it != cfg.end(); it++) {
        out << (it->first) << "=" << (it->second) << std::endl;
    }
}


