#include "myguidatamanager.hpp"

#include <MyGUI_DataStream.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/debug/debuglog.hpp>

namespace osgMyGUI
{

void DataManager::setResourcePath(const std::string &path)
{
    mResourcePath = path;
}

void DataManager::setVfs(const VFS::Manager* vfs)
{
    mVfs = vfs;
}

MyGUI::IDataStream *DataManager::getData(const std::string &name)
{
    // Note: MyGUI is supposed to read/free input steam itself,
    // so copy data from VFS stream to the string stream and pass it to MyGUI.
    Files::IStreamPtr streamPtr = mVfs->get("mygui\\"+name);
    std::istream* fileStream = streamPtr.get();
    std::unique_ptr<std::stringstream> dataStream;
    dataStream.reset(new std::stringstream);
    *dataStream << fileStream->rdbuf();
    return new MyGUI::DataStream(dataStream.release());
}

void DataManager::freeData(MyGUI::IDataStream *data)
{
    delete data;
}

bool DataManager::isDataExist(const std::string &name)
{
    return mVfs->exists("mygui\\"+name);
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
    // FIXME: in theory, we should use the VFS here too, but it does not provide the real path to data files.
    // In some cases there is no real path at all (when the requested MyGUI file is in BSA archive, for example).
    // Currently it should not matter since we use this virtual function only to setup fonts for profilers.
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
