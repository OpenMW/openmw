#include "importer.hpp"
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

MwIniImporter::MwIniImporter()
    : mVerbose(false)
{
    const char *map[][2] =
    {
        { "fps", "General:Show FPS" },
        { "nosound", "General:Disable Audio" },
        { 0, 0 }
    };
    const char *fallback[] = {
        "Weather:Sunrise Time",
        "Weather:Sunset Time",
        0
    };

    for(int i=0; map[i][0]; i++) {
        mMergeMap.insert(std::make_pair<std::string, std::string>(map[i][0], map[i][1]));
    }

    for(int i=0; fallback[i]; i++) {
        mMergeFallback.push_back(fallback[i]);
    }
}

void MwIniImporter::setVerbose(bool verbose) {
    mVerbose = verbose;
}

std::string MwIniImporter::numberToString(int n) {
    std::stringstream str;
    str << n;
    return str.str();
}

MwIniImporter::multistrmap MwIniImporter::loadIniFile(std::string filename) {
    std::cout << "load ini file: " << filename << std::endl;

    std::string section("");
    MwIniImporter::multistrmap map;
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

        std::string key(section + ":" + line.substr(0,pos));
        std::string value(line.substr(pos+1));

        multistrmap::iterator it;
        if((it = map.find(key)) == map.end()) {
            map.insert( std::make_pair (key, std::vector<std::string>() ) );
        }
        map[key].push_back(value);
    }

    return map;
}

MwIniImporter::multistrmap MwIniImporter::loadCfgFile(std::string filename) {
    std::cout << "load cfg file: " << filename << std::endl;

    MwIniImporter::multistrmap map;
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

        std::string key(line.substr(0,pos));
        std::string value(line.substr(pos+1));

        multistrmap::iterator it;
        if((it = map.find(key)) == map.end()) {
            map.insert( std::make_pair (key, std::vector<std::string>() ) );
        }
        map[key].push_back(value);
    }

    return map;
}

void MwIniImporter::merge(multistrmap &cfg, multistrmap &ini) {
    multistrmap::iterator cfgIt;
    multistrmap::iterator iniIt;
    for(strmap::iterator it=mMergeMap.begin(); it!=mMergeMap.end(); ++it) {
        if((iniIt = ini.find(it->second)) != ini.end()) {
            for(std::vector<std::string>::iterator vc = iniIt->second.begin(); vc != iniIt->second.end(); ++vc) {
                cfg.erase(it->first);
                insertMultistrmap(cfg, it->first, *vc);
            }
        }
    }
}

void MwIniImporter::mergeFallback(multistrmap &cfg, multistrmap &ini) {
    cfg.erase("fallback");

    multistrmap::iterator cfgIt;
    multistrmap::iterator iniIt;
    for(std::vector<std::string>::iterator it=mMergeFallback.begin(); it!=mMergeFallback.end(); ++it) {
        if((iniIt = ini.find(*it)) != ini.end()) {
            for(std::vector<std::string>::iterator vc = iniIt->second.begin(); vc != iniIt->second.end(); ++vc) {
                std::string value(*it);
                std::replace( value.begin(), value.end(), ' ', '_' );
                std::replace( value.begin(), value.end(), ':', '_' );
                value.append(",").append(vc->substr(0,vc->length()-1));
                insertMultistrmap(cfg, "fallback", value);
            }
        }
    }
}

void MwIniImporter::insertMultistrmap(multistrmap &cfg, std::string key, std::string value) {
    multistrmap::iterator it = cfg.find(key);
    if(it == cfg.end()) {
        cfg.insert(std::make_pair (key, std::vector<std::string>() ));
    }
    cfg[key].push_back(value);
}

void MwIniImporter::importGameFiles(multistrmap &cfg, multistrmap &ini) {
    std::vector<std::string> esmFiles;
    std::vector<std::string> espFiles;
    std::string baseGameFile("Game Files:GameFile");
    std::string gameFile("");

    multistrmap::iterator it = ini.begin();
    for(int i=0; it != ini.end(); i++) {
        gameFile = baseGameFile;
        gameFile.append(this->numberToString(i));

        it = ini.find(gameFile);
        if(it == ini.end()) {
            break;
        }

        for(std::vector<std::string>::iterator entry = it->second.begin(); entry!=it->second.end(); ++entry) {
            std::string filetype(entry->substr(entry->length()-4, 3));
            std::transform(filetype.begin(), filetype.end(), filetype.begin(), ::tolower);

            if(filetype.compare("esm") == 0) {
                esmFiles.push_back(*entry);
            }
            else if(filetype.compare("esp") == 0) {
                espFiles.push_back(*entry);
            }
        }

        gameFile = "";
    }

    cfg.erase("master");
    cfg.insert( std::make_pair<std::string, std::vector<std::string> > ("master", std::vector<std::string>() ) );

    for(std::vector<std::string>::iterator it=esmFiles.begin(); it!=esmFiles.end(); ++it) {
        cfg["master"].push_back(*it);
    }

    cfg.erase("plugin");
    cfg.insert( std::make_pair<std::string, std::vector<std::string> > ("plugin", std::vector<std::string>() ) );

    for(std::vector<std::string>::iterator it=espFiles.begin(); it!=espFiles.end(); ++it) {
        cfg["plugin"].push_back(*it);
    }
}

void MwIniImporter::writeToFile(boost::iostreams::stream<boost::iostreams::file_sink> &out, multistrmap &cfg) {

    for(multistrmap::iterator it=cfg.begin(); it != cfg.end(); ++it) {
        for(std::vector<std::string>::iterator entry=it->second.begin(); entry != it->second.end(); ++entry) {
            out << (it->first) << "=" << (*entry) << std::endl;
        }
    }
}
