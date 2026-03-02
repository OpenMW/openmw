#include "myguidatamanager.hpp"

#include <stdexcept>
#include <string>

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
        {
        }

    private:
        std::unique_ptr<std::istream> mOwnedStream;
    };
}

namespace MyGUIPlatform
{

    void DataManager::setResourcePath(VFS::Path::NormalizedView path)
    {
        mResourcePath = path;
    }

    VFS::Path::NormalizedView DataManager::getResourcePath() const
    {
        return mResourcePath;
    }

    DataManager::DataManager(VFS::Path::NormalizedView resourcePath, const VFS::Manager* vfs)
        : mResourcePath(resourcePath)
        , mVfs(vfs)
    {
    }

    MyGUI::IDataStream* DataManager::getData(const std::string& name) const
    {
        VFS::Path::Normalized path(mResourcePath);
        path /= name;
        return new DataStream(mVfs->get(path));
    }

    void DataManager::freeData(MyGUI::IDataStream* data)
    {
        delete data;
    }

    bool DataManager::isDataExist(const std::string& name) const
    {
        VFS::Path::Normalized path(mResourcePath);
        path /= name;
        return mVfs->exists(path);
    }

    const MyGUI::VectorString& DataManager::getDataListNames(const std::string& /*pattern*/) const
    {
        throw std::runtime_error("DataManager::getDataListNames is not implemented - VFS is used");
    }

    std::string DataManager::getDataPath(const std::string& name) const
    {
        VFS::Path::Normalized path(mResourcePath);
        path /= name;
        if (!mVfs->exists(path))
            return {};

        return path;
    }

}
