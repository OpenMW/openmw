#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIDATAMANAGER_H

#include <MyGUI_DataManager.h>

namespace osgMyGUI
{


class DataManager : public MyGUI::DataManager
{
public:
    void initialise() {}
    void shutdown() {}

    void setResourcePath(const std::string& path);

    /** Get data stream from specified resource name.
        @param _name Resource name (usually file name).
    */
    MyGUI::IDataStream* getData(const std::string& _name) override;

    /** Free data stream.
        @param _data Data stream.
    */
    void freeData(MyGUI::IDataStream* _data) override;

    /** Is data with specified name exist.
        @param _name Resource name.
    */
    bool isDataExist(const std::string& _name) override;

    /** Get all data names with names that matches pattern.
        @param _pattern Pattern to match (for example "*.layout").
    */
    const MyGUI::VectorString& getDataListNames(const std::string& _pattern) override;

    /** Get full path to data.
        @param _name Resource name.
        @return Return full path to specified data.
    */
    const std::string& getDataPath(const std::string& _name) override;

private:
    std::string mResourcePath;
};

}

#endif
