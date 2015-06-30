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
    virtual MyGUI::IDataStream* getData(const std::string& _name);

    /** Free data stream.
        @param _data Data stream.
    */
    virtual void freeData(MyGUI::IDataStream* _data);

    /** Is data with specified name exist.
        @param _name Resource name.
    */
    virtual bool isDataExist(const std::string& _name);

    /** Get all data names with names that matches pattern.
        @param _pattern Pattern to match (for example "*.layout").
    */
    virtual const MyGUI::VectorString& getDataListNames(const std::string& _pattern);

    /** Get full path to data.
        @param _name Resource name.
        @return Return full path to specified data.
    */
    virtual const std::string& getDataPath(const std::string& _name);

private:
    std::string mResourcePath;
};

}

#endif
