#include "myguidatamanager.hpp"

#include <MyGUI_DataFileStream.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/debug/debuglog.hpp>

namespace osgMyGUI
{

void DataManager::setResourcePath(const std::string &path)
{
    mResourcePath = path;
}

MyGUI::IDataStream *DataManager::getData(const std::string &name) OPENMW_MYGUI_CONST_GETTER_3_4_1
{
    std::string fullpath = getDataPath(name);
    std::unique_ptr<boost::filesystem::ifstream> stream;
    stream.reset(new boost::filesystem::ifstream);
    stream->open(fullpath, std::ios::binary);
    if (stream->fail())
    {
        Log(Debug::Error) << "DataManager::getData: Failed to open '" << name << "'";
        return nullptr;
    }
    return new MyGUI::DataFileStream(stream.release());
}

void DataManager::freeData(MyGUI::IDataStream *data)
{
    delete data;
}

bool DataManager::isDataExist(const std::string &name) OPENMW_MYGUI_CONST_GETTER_3_4_1
{
    std::string fullpath = mResourcePath + "/" + name;
    return boost::filesystem::exists(fullpath);
}

const MyGUI::VectorString &DataManager::getDataListNames(const std::string &pattern) OPENMW_MYGUI_CONST_GETTER_3_4_1
{
    // TODO: pattern matching (unused?)
    static MyGUI::VectorString strings;
    strings.clear();
    strings.push_back(getDataPath(pattern));
    return strings;
}

const std::string &DataManager::getDataPath(const std::string &name) OPENMW_MYGUI_CONST_GETTER_3_4_1
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
