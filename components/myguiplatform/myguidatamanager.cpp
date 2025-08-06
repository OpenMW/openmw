#include "myguidatamanager.hpp"

#include <stdexcept>
#include <string>

#include <MyGUI_DataFileStream.h>

#include <components/files/conversion.hpp>
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

    void DataManager::setResourcePath(const std::filesystem::path& path)
    {
        mResourcePath = path;
    }

    DataManager::DataManager(const std::string& resourcePath, const VFS::Manager* vfs)
        : mResourcePath(resourcePath)
        , mVfs(vfs)
    {
    }

    MyGUI::IDataStream* DataManager::getData(const std::string& name) const
    {
        return new DataStream(mVfs->get(Files::pathToUnicodeString(mResourcePath / name)));
    }

    void DataManager::freeData(MyGUI::IDataStream* data)
    {
        delete data;
    }

    bool DataManager::isDataExist(const std::string& name) const
    {
        return mVfs->exists(Files::pathToUnicodeString(mResourcePath / name));
    }

    const MyGUI::VectorString& DataManager::getDataListNames(const std::string& /*pattern*/) const
    {
        throw std::runtime_error("DataManager::getDataListNames is not implemented - VFS is used");
    }

    std::string DataManager::getDataPath(const std::string& name) const
    {
        if (name.empty())
        {
            return Files::pathToUnicodeString(mResourcePath);
        }

        if (!isDataExist(name))
            return {};

        return Files::pathToUnicodeString(mResourcePath / name);
    }

}
