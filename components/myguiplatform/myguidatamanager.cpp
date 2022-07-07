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

void DataManager::setUseVfs(bool useVfs)
{
    mUseVfs = useVfs;
}

MyGUI::IDataStream *DataManager::getData(const std::string &name) const
{
    if (mUseVfs)
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

    std::string fullpath = getDataPath(name);
    auto stream = std::make_unique<std::ifstream>();
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

bool DataManager::isDataExist(const std::string &name) const
{
    if (mUseVfs) return mVfs->exists(name);

    std::string fullpath = mResourcePath + "/" + name;
    return std::filesystem::exists(fullpath);
}

void DataManager::setVfs(const VFS::Manager* vfs)
{
    mVfs = vfs;
}

const MyGUI::VectorString &DataManager::getDataListNames(const std::string &pattern) const
{
    // TODO: pattern matching (unused?)
    static MyGUI::VectorString strings;
    strings.clear();
    strings.push_back(getDataPath(pattern));
    return strings;
}

const std::string &DataManager::getDataPath(const std::string &name) const
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
