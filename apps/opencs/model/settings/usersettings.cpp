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

#include "settingmodel.hpp"
#include "setting.hpp"

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

    mReadWriteMessage = QObject::tr("<br><b>Could not open or create file for writing</b><br><br> \
            Please make sure you have the right permissions and try again.<br>");

    mReadOnlyMessage = QObject::tr("<br><b>Could not open file for reading</b><br><br> \
            Please make sure you have the right permissions and try again.<br>");

    mSettingModel = new SettingModel (this);

    buildSettingModelDefaults();
}

void CSMSettings::UserSettings::buildSettingModelDefaults()
{
    QString section = "Window Size";
    {
        Setting *width = mSettingModel->createSetting ("Width", section, "1024");
        Setting *height = mSettingModel->createSetting ("Height", section, "768");
        Setting *preDefined = mSettingModel->createSetting ("Pre-Defined", section, "1024 x 768");

        preDefined->valueList() << "640 x 480" << "800 x 600" << "1024 x 768" << "1440 x 900";
        preDefined->proxyList() << "Width" << "Height";

        // value lists for width / height are used to validate interaction with pre-defined setting
        // lists are ignored for independent changes to width / height settings (line edit widget is not list-style)
        width->valueList() << "640" << "800" << "1024" << "1440";
        height->valueList() << "480" << "600" << "768" << "900";
    }

    section = "Display Format";
    {
        QString defaultValue = "Icon and Text";
        Setting *recordStatusDisplay = mSettingModel->createSetting("Record Status Display", section, defaultValue);
        Setting *refIdDisplay = mSettingModel->createSetting("Referenceable ID Type Display", section, defaultValue);

        QStringList values;
        values << defaultValue << "Icon Only" << "Text Only";

        recordStatusDisplay->setValueList(values);
        refIdDisplay->setValueList(values);
    }
}

CSMSettings::UserSettings::~UserSettings()
{
    mUserSettingsInstance = 0;
}

QTextStream *CSMSettings::UserSettings::openFileStream (const QString &filePath, bool isReadOnly) const
{
    QIODevice::OpenMode openFlags = QIODevice::Text;

    if (isReadOnly)
        openFlags = QIODevice::ReadOnly | openFlags;
    else
        openFlags = QIODevice::ReadWrite | QIODevice::Truncate | openFlags;

    QFile *file = new QFile(filePath);
    QTextStream *stream = 0;

    if (file->open(openFlags))
        stream = new QTextStream(file);

    if (stream)
        stream->setCodec(QTextCodec::codecForName("UTF-8"));
    else
        destroyStream (stream);

    return stream;
}

bool CSMSettings::UserSettings::writeSettings()
{
    QTextStream *stream = openFileStream(mUserFilePath);

    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return false;
    }

    mSettingModel->sort(1);
    QString section = "";

    for (int i =0; i < mSettingModel->rowCount(); ++i)
    {
        const Setting *setting = mSettingModel->getSetting(i);

        if (section != setting->sectionName())
        {
            section = setting->sectionName();
            *stream << "[" << section << "]" << "\n";
        }

        foreach (const QString &settingValue, setting->values())
            *stream << setting->name() << " = " << settingValue << "\n";
    }

    destroyStream (stream);
    return true;
}

void CSMSettings::UserSettings::destroyStream(QTextStream *stream) const
{
    stream->device()->close();
    delete stream;
    stream = 0;
}

QSortFilterProxyModel *CSMSettings::UserSettings::createProxyFilter (int column, QAbstractItemModel *model)
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);

    if (!model)
        proxy->setSourceModel (mSettingModel);
    else
        proxy->setSourceModel (model);

    proxy->setFilterKeyColumn(column);
    proxy->setFilterFixedString ("*");
    proxy->setDynamicSortFilter (true);
    return proxy;
}

bool CSMSettings::UserSettings::loadSettingsFromFile (const QString &filePath)
{
    if (filePath.isEmpty())
        return false;

    QTextStream *stream = openFileStream (filePath, true);

    if (!stream)
        return false;

    QSortFilterProxyModel *sectionFilter = createProxyFilter (1);
    QSortFilterProxyModel *settingFilter = createProxyFilter (0, sectionFilter);

    //regExp for section names
    QRegExp sectionRe("^\\[([^]]+)\\]");

    //regExp for setting definitions (name / value pair separated by '=')
    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    QString section = "none";

    while (!stream->atEnd())
    {
        QString line = stream->readLine().simplified();

        //skip if no data or comment found
        if (line.isEmpty() || line.startsWith("#"))
            continue;

        // parse setting section name and filter model
        if (sectionRe.exactMatch(line))
        {
            //save section name and set filter proxy
            section = sectionRe.cap(1).simplified();
            sectionFilter->setFilterFixedString (section);
            continue;
        }

        // parse setting definition, assuming valid section
        if ( (keyRe.indexIn(line) != -1) && (section != "none"))
        {
            qDebug() << "parsing line: " << line;
            QString settingName = keyRe.cap(1).simplified();
            QString settingValue = keyRe.cap(2).simplified();

            settingFilter->setFilterFixedString(settingName);

            { // test for missing setting...
                bool success = (settingFilter->rowCount() != 0);

                if (success)
                    success = mSettingModel->setDataByName(section, settingName, settingValue);

                if (!success)
                {
                    qDebug ("\nUndefined setting found.\n    File: %s\n    Section: %s\n    Definition: %s = %s",
                            filePath.toStdString().c_str(), section.toStdString().c_str(),
                            settingName.toStdString().c_str(), settingValue.toStdString().c_str());
                }
                else
                    qDebug ("\nSetting %s value set to %s\n", settingName.toStdString().c_str(), settingValue.toStdString().c_str());
            }
         }
    }
    return true;
}

void CSMSettings::UserSettings::loadSettings (const QString &fileName)
{
    //global
    QString globalFilePath = QString::fromStdString(mCfgMgr.getGlobalPath().string()) + fileName;
    bool globalOk = loadSettingsFromFile(globalFilePath);


    //local
    QString localFilePath = QString::fromStdString(mCfgMgr.getLocalPath().string()) + fileName;
    bool localOk = loadSettingsFromFile(localFilePath);

    //user
    mUserFilePath = QString::fromStdString(mCfgMgr.getUserPath().string()) + fileName;
    loadSettingsFromFile(mUserFilePath);

    if (!(localOk || globalOk))
    {
        QString message = QObject::tr("<br><b>Could not open user settings files for reading</b><br><br> \
                Global and local settings files could not be read.\
                You may have incorrect file permissions or the OpenCS installation may be corrupted.<br>");

        message += QObject::tr("<br>Global filepath: ") + globalFilePath;
        message += QObject::tr("<br>Local filepath: ") + localFilePath;

        displayFileErrorMessage ( message, true);
    }
}

CSMSettings::UserSettings& CSMSettings::UserSettings::instance()
{
    assert(mUserSettingsInstance);
    return *mUserSettingsInstance;
}

void CSMSettings::UserSettings::displayFileErrorMessage(const QString &message, bool isReadOnly)
{
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(QObject::tr("OpenCS configuration file I/O error"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);

        if (!isReadOnly)
            msgBox.setText (mReadWriteMessage + message);
        else
            msgBox.setText (message);

        msgBox.exec();
}
