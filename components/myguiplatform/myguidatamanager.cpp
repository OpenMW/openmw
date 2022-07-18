#include "myguidatamanager.hpp"

#include <memory>
#include <string>
#include <stdexcept>

#include <MyGUI_DataFileStream.h>

#include <components/vfs/manager.hpp>

namespace
{
    class DataStream final : public MyGUI::DataStream
    {
    public:
        explicit DataStream(std::unique_ptr<std::istream>&& stream)
            : MyGUI::DataStream(stream.get())
            , mOwnedStream(std::move(stream))
        {}

    private:
        std::unique_ptr<std::istream> mOwnedStream;
    };
}

namespace osgMyGUI
{

void DataManager::setResourcePath(const std::string &path)
{
    mResourcePath = path;
}

DataManager::DataManager(const std::string& resourcePath, const VFS::Manager* vfs)
    : mResourcePath(resourcePath)
    , mVfs(vfs)
{
}

MyGUI::IDataStream *DataManager::getData(const std::string &name) const
{
    return new DataStream(mVfs->get(mResourcePath + "/" + name));
}

void DataManager::freeData(MyGUI::IDataStream *data)
{
    delete data;
}

bool DataManager::isDataExist(const std::string &name) const
{
    return mVfs->exists(mResourcePath + "/" + name);
}

const MyGUI::VectorString &DataManager::getDataListNames(const std::string &pattern) const
{
    throw std::runtime_error("DataManager::getDataListNames is not implemented - VFS is used");
}

const std::string &DataManager::getDataPath(const std::string &name) const
{
    static std::string result;
    result.clear();

    if (name.empty())
        return mResourcePath;

    if (!isDataExist(name))
        return result;

    result = mResourcePath + "/" + name;
    return result;
}

}
