#include "myguidatamanager.hpp"

#include <memory>
#include <string>

#include <MyGUI_DataFileStream.h>

#include <filesystem>
#include <fstream>

#include <components/debug/debuglog.hpp>

namespace osgMyGUI
{

void DataManager::setResourcePath(const std::string &path)
{
    mResourcePath = path;
}

MyGUI::IDataStream *DataManager::getData(const std::string &name) const
{
    // Note: MyGUI is supposed to read/free input steam itself,
    // so copy data from VFS stream to the string stream and pass it to MyGUI.
    Files::IStreamPtr streamPtr = mVfs->get(name);
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

bool DataManager::isDataExist(const std::string &name) const
{
    return mVfs->exists(name);
}

void DataManager::setVfs(const VFS::Manager* vfs)
{
    mVfs = vfs;
}

const MyGUI::VectorString &DataManager::getDataListNames(const std::string &pattern) const
{
    throw std::runtime_error("DataManager::getDataListNames is not implemented - VFS is used");
}

const std::string &DataManager::getDataPath(const std::string &name) const
{
    throw std::runtime_error("DataManager::getDataPath is not implemented - VFS is used");
}

}
