#include "usersettings.hpp"

#include <QTextStream>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QMap>
#include <QMessageBox>
#include <QTextCodec>

#include <QFile>
#include <QSortFilterProxyModel>

#include <components/files/configurationmanager.hpp>
#include <boost/version.hpp>

#include "setting.hpp"
#include "../../view/settings/support.hpp"

#include <QDebug>


/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

    template<>
    inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
    {
        return boost::filesystem::path(arg);
    }

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */

CSMSettings::UserSettings *CSMSettings::UserSettings::mUserSettingsInstance = 0;

CSMSettings::UserSettings::UserSettings()
{
    assert(!mUserSettingsInstance);
    mUserSettingsInstance = this;

    buildSettingModelDefaults();
}

void CSMSettings::UserSettings::buildSettingModelDefaults()
{
    QString section = "Window Size";
    {
        Setting *width = new Setting
                                    (Type_SingleText, "Width", section, "1024");
        Setting *height = new Setting
                                    (Type_SingleText, "Height", section, "768");

        width->setProperty(Property_WidgetWidth, 5);

        height->setProperty (Property_WidgetWidth, 5);
        height->setProperty (Property_ViewColumn, 1);

        addDeclaration (width);
        addDeclaration (height);
/*
        QStringList predefValues;

        predefValues    << "640 x 480"
                        << "800 x 600"
                        << "1024 x 768"
                        << "1440 x 900";

        Setting *preDefined = declarationModel().singleList
                        (section, "Pre-Defined", predefValues, "1024 x 768");

*/
        //QStringList widthValues;
        //QStringList heightValues;

//        widthValues << "640" << "800" << "1024" << "1440";
//        heightValues << "480" << "600" << "768" << "900";

//        preDefined->proxyMap().insert("Width", widthValues);
//        preDefined->proxyMap().insert("Height", heightValues);

//      preDefined->setViewType(CSVSettings::ViewType_List);

    }

    section = "Display Format";
    {
        QString defaultValue = "Icon and Text";

        Setting *rsd = new Setting (Type_SingleBool, "Record Status Display",
                                                        section, defaultValue);

        Setting *ritd = new Setting (Type_SingleBool,
                                     "Referenceable ID Type Display",
                                                        section, defaultValue);


        QStringList dfValues;

        dfValues << defaultValue << "Icon Only" << "Text Only";

        rsd->setPropertyList (PropertyList_DeclaredValues, dfValues);
        ritd->setPropertyList (PropertyList_DeclaredValues, dfValues);

        addDeclaration (rsd);
        addDeclaration (ritd);
    }
}

CSMSettings::UserSettings::~UserSettings()
{
    mUserSettingsInstance = 0;
}

bool CSMSettings::UserSettings::loadSettingsFromFile
                                                (const QStringList &filepaths)
{
    if (filepaths.isEmpty())
        return false;

    bool success = true;

    PageMap totalMap;

    foreach (const QString &filepath, filepaths)
    {
        QTextStream *stream = openFilestream (filepath, true);

        PageMap pageMap;

        if (stream)
            pageMap = readFilestream(stream);
        else
            success = success && false;

        mergeSettings (totalMap, pageMap, Merge_Overwrite);
    }
   // validate (totalMap);
    buildModel (totalMap);

    return success;
}

void CSMSettings::UserSettings::loadSettings (const QString &fileName)
{
    QStringList filepaths;

    //global
    filepaths << QString::fromStdString
                                (mCfgMgr.getGlobalPath().string()) + fileName;

    //local
    filepaths << QString::fromStdString
                                (mCfgMgr.getLocalPath().string()) + fileName;
    //user
    filepaths << QString::fromStdString
                            (mCfgMgr.getUserConfigPath().string()) + fileName;

    bool success = loadSettingsFromFile (filepaths);

    if (!success)
    {
        QString message = QObject::tr("<br><b>Could not open user settings \
                files for reading</b><br><br> Global and local settings files \
                could not be read.  You may have incorrect file permissions or \
                the OpenCS installation may be corrupted.<br>");

        message += QObject::tr("<br>Global filepath: ") + filepaths.at(0);
        message += QObject::tr("<br>Local filepath: ") + filepaths.at(1);

        displayFileErrorMessage ( message, true);
    }

    mUserFilePath = filepaths.at (2);
}

void CSMSettings::UserSettings::saveSettings ()
{
   writeFilestream (openFilestream (mUserFilePath, false));
}

CSMSettings::UserSettings& CSMSettings::UserSettings::instance()
{
    assert(mUserSettingsInstance);
    return *mUserSettingsInstance;
}
