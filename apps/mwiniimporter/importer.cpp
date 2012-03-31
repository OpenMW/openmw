#include "importer.hpp"
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

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
    std::map<std::string, std::string> map;
    boost::iostreams::stream<boost::iostreams::file_source>file(filename.c_str());

    std::string line;
    while (std::getline(file, line)) {

        if(line[0] == '[') {
            if(line.length() > 2) {
                section = line.substr(1, line.length()-3);
            }
            continue;
        }
        
        int comment_pos = line.find(";");
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
        
        map.insert(std::make_pair<std::string, std::string>(section + ":" + line.substr(0,pos), line.substr(pos+1)));
    }
    
    return map;
}

strmap MwIniImporter::loadCfgFile(std::string filename) {
    std::cout << "load cfg file: " << filename << std::endl;
    
    std::map<std::string, std::string> map;
    boost::iostreams::stream<boost::iostreams::file_source>file(filename.c_str());

    std::string line;
    while (std::getline(file, line)) {
        
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

bool MwIniImporter::specialMerge(std::string cfgKey, std::string iniKey, strmap &cfg, strmap &ini) {
    return false;
}

void MwIniImporter::importGameFiles(strmap &cfg, strmap &ini, std::vector<std::string> &esmFiles, std::vector<std::string> &espFiles) {
    std::string baseGameFile("Game Files:GameFile");
    std::string gameFile("");

    strmap::iterator it = ini.begin();
    for(int i=0; it != ini.end(); i++) {
        gameFile = baseGameFile;
        gameFile.append(1,i+'0');
        
        it = ini.find(gameFile);
        if(it == ini.end()) {
            break;
        }
        
        std::string filetype(it->second.substr(it->second.length()-4, 3));
        std::transform(filetype.begin(), filetype.end(), filetype.begin(), ::tolower);
        
        if(filetype.compare("esm") == 0) {
            esmFiles.push_back(it->second);
        }
        else if(filetype.compare("esp") == 0) {
            espFiles.push_back(it->second);
        }
        
        gameFile = "";
    }
}

void MwIniImporter::writeGameFiles(boost::iostreams::stream<boost::iostreams::file_sink> &out, std::vector<std::string> &esmFiles, std::vector<std::string> &espFiles) {
    for(std::vector<std::string>::iterator it=esmFiles.begin(); it != esmFiles.end(); it++) {
        out << "master=" << *it << std::endl;
    }
    for(std::vector<std::string>::iterator it=espFiles.begin(); it != espFiles.end(); it++) {
        out << "plugin=" << *it << std::endl;
    }
}

void MwIniImporter::writeToFile(boost::iostreams::stream<boost::iostreams::file_sink> &out, strmap &cfg) {
    cfg.erase("master");
    cfg.erase("plugin");
    
    for(strmap::iterator it=cfg.begin(); it != cfg.end(); it++) {
        out << (it->first) << "=" << (it->second) << std::endl;
    }
}


