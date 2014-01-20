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

#include "definitionmodel.hpp"
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
    {/*
        Setting *width = declarationModel().singleText
                                                    (section, "Widget", "1024");

        Setting *height = declarationModel().singleText
                                                    (section, "Height", "768");

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
//      width->setViewType (CSVSettings::ViewType_Text);
//      height->setWidgetType (CSVSettings::ViewType_Text);
    }

    section = "Display Format";
    {
        QString defaultValue = "Icon and Text";

        QStringList dfValues;
        dfValues << defaultValue << "Icon Only" << "Text Only";

        declarationModel().singleBool (section, "Record Status Display",
                                       dfValues, defaultValue);

        declarationModel().singleBool (section, "Referenceable ID Type Display",
                                       dfValues, defaultValue);
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
            pageMap = SettingModel::readFilestream(stream);
        else
           {
            success = success && false;
            qDebug() << "could not open filepath: " << filepath;
        }

        mergeSettings (totalMap, pageMap);
    }

    validate (totalMap);

    qDebug () << "Loaded Definitions: ";

    foreach (const QString &pageKey, totalMap.keys())
    {
        SettingMap *settingMap = totalMap[pageKey];
        qDebug () << "\t[" << pageKey << "]";
        foreach (const QString &settingKey, settingMap->keys())
        {
            foreach (const QString &value, *(settingMap->value(settingKey)))
                qDebug () << "\t\t" << settingKey << " = " << value;
        }
    }

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
                                (mCfgMgr.getUserPath().string()) + fileName;

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
