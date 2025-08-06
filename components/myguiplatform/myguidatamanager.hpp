#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H

#include <MyGUI_DataManager.h>

#include <filesystem>
#include <string>

namespace VFS
{
    class Manager;
}

namespace MyGUIPlatform
{

    class DataManager : public MyGUI::DataManager
    {
    public:
        explicit DataManager(const std::string& path, const VFS::Manager* vfs);

        void setResourcePath(const std::filesystem::path& path);

        /** Get data stream from specified resource name.
            @param name Resource name (usually file name).
        */
        MyGUI::IDataStream* getData(const std::string& name) const override;

        /** Free data stream.
            @param data Data stream.
        */
        void freeData(MyGUI::IDataStream* data) override;

        /** Is data with specified name exist.
            @param name Resource name.
        */
        bool isDataExist(const std::string& name) const override;

        /** Get all data names with names that matches pattern.
            @param pattern Pattern to match (for example "*.layout").
        */
        const MyGUI::VectorString& getDataListNames(const std::string& pattern) const override;

        /** Get full path to data.
            @param name Resource name.
            @return Return full path to specified data.
        */
        std::string getDataPath(const std::string& name) const override;

    private:
        std::filesystem::path mResourcePath;

        const VFS::Manager* mVfs;
    };

}

#endif
