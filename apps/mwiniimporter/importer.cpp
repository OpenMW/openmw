#include "importer.hpp"
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>

MwIniImporter::MwIniImporter() {
    const char *map[][2] =
    {
        { "fps", "General:Show FPS" },
        { 0, 0 }
    };
    
    for(int i=0; map[i][0]; i++) {
        mMergeMap.insert(std::make_pair<std::string, std::string>(map[i][0], map[i][1]));
    }
}

void MwIniImporter::setVerbose(bool verbose) {
    mVerbose = verbose;
}

strmap MwIniImporter::loadIniFile(std::string filename) {
    std::cout << "load ini file: " << filename << std::endl;
    
    std::string section("");
    std::multimap<std::string, std::string> map;
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
        
        map.insert(std::make_pair<std::string, std::string>(section + ":" + line.substr(0,pos), line.substr(pos+1)));
    }
    
    return map;
}

strmap MwIniImporter::loadCfgFile(std::string filename) {
    std::cout << "load cfg file: " << filename << std::endl;
    
    std::multimap<std::string, std::string> map;
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
        
        map.insert(std::make_pair<std::string, std::string>(line.substr(0,pos), line.substr(pos+1)));
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
                cfg.insert(std::make_pair<std::string, std::string>(it->first, iniIt->second));
            }
        }
    }
}

void MwIniImporter::importGameFiles(strmap &cfg, strmap &ini) {
    std::vector<std::string> esmFiles;
    std::string baseEsm("Game Files:GameFile");
    std::string esmFile("");

    strmap::iterator it = ini.begin();
    for(int i=0; it != ini.end(); i++) {
        esmFile = baseEsm;
        esmFile.append(1,i+'0');
        
        it = ini.find(esmFile);
        if(it == ini.end()) {
            break;
        }
        
        std::cout << "found EMS file: " << it->second << std::endl;
        esmFiles.push_back(it->second);
        esmFile = "";
    }


    std::vector<std::string> bsaFiles;
    std::string baseBsa("Archives:Archive ");
    std::string bsaFile("");
    
    it = ini.begin();
    for(int i=0; it != ini.end(); i++) {
        bsaFile = baseBsa;
        bsaFile.append(1,i+'0');
        
        it = ini.find(bsaFile);
        if(it == ini.end()) {
            break;
        }
        
        std::cout << "found BSA file: " << it->second << std::endl;
        bsaFiles.push_back(it->second);
        bsaFile = "";
    }
    
    if(!esmFiles.empty()) {
        cfg.erase("master");
        for(std::vector<std::string>::iterator it = esmFiles.begin(); it != esmFiles.end(); it++) {
            cfg.insert(std::make_pair<std::string, std::string>("master", *it));
        }
    }
    
    if(!bsaFile.empty()) {
        cfg.erase("plugin");
        for(std::vector<std::string>::iterator it = bsaFiles.begin(); it != bsaFiles.end(); it++) {
            cfg.insert(std::make_pair<std::string, std::string>("plugin", *it));
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


