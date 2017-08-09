#include "myguidatamanager.hpp"

#include <MyGUI_DataFileStream.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <iostream>

namespace osgMyGUI
{

void DataManager::setResourcePath(const std::string &path)
{
    mResourcePath = path;
}

MyGUI::IDataStream *DataManager::getData(const std::string &name)
{
    std::string fullpath = getDataPath(name);
    std::unique_ptr<boost::filesystem::ifstream> stream;
    stream.reset(new boost::filesystem::ifstream);
    stream->open(fullpath, std::ios::binary);
    if (stream->fail())
    {
        std::cerr << "DataManager::getData: Failed to open '" << name << "'" << std::endl;
        return NULL;
    }
    return new MyGUI::DataFileStream(stream.release());
}

void DataManager::freeData(MyGUI::IDataStream *data)
{
    delete data;
}

bool DataManager::isDataExist(const std::string &name)
{
    std::string fullpath = mResourcePath + "/" + name;
    return boost::filesystem::exists(fullpath);
}

const MyGUI::VectorString &DataManager::getDataListNames(const std::string &pattern)
{
    // TODO: pattern matching (unused?)
    static MyGUI::VectorString strings;
    strings.clear();
    strings.push_back(getDataPath(pattern));
    return strings;
}

const std::string &DataManager::getDataPath(const std::string &name)
{
    static std::string result;
    result.clear();
    if (!isDataExist(name))
    {
        return result;
    }
    result = mResourcePath + "/" + name;
    return result;
}

}
