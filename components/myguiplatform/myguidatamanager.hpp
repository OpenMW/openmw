#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H

#include <MyGUI_DataManager.h>

#include <filesystem>
#include <string>

namespace VFS
{
    class Manager;
}

namespace osgMyGUI
{

    class DataManager : public MyGUI::DataManager
    {
    public:
        explicit DataManager(const std::string& path, const VFS::Manager* vfs);

        void setResourcePath(const std::filesystem::path& path);

        /** Get data stream from specified resource name.
            @param _name Resource name (usually file name).
        */
        MyGUI::IDataStream* getData(const std::string& _name) const override;

        /** Free data stream.
            @param _data Data stream.
        */
        void freeData(MyGUI::IDataStream* _data) override;

        /** Is data with specified name exist.
            @param _name Resource name.
        */
        bool isDataExist(const std::string& _name) const override;

        /** Get all data names with names that matches pattern.
            @param _pattern Pattern to match (for example "*.layout").
        */
        const MyGUI::VectorString& getDataListNames(const std::string& _pattern) const override;

        /** Get full path to data.
            @param _name Resource name.
            @return Return full path to specified data.
        */
        std::string getDataPath(const std::string& _name) const override;

    private:
        std::filesystem::path mResourcePath;

        const VFS::Manager* mVfs;
    };

}

#endif
