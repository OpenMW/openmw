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
        Setting *width = createSetting ("Width", section, "1024");
        Setting *height = createSetting ("Height", section, "768");
        Setting *preDefined = createSetting ("Pre-Defined", section, "1024 x 768");

        preDefined->valueList() << "640 x 480" << "800 x 600" << "1024 x 768" << "1440 x 900";

        // value lists for width / height are used to validate interaction with pre-defined setting
        // lists are ignored for independent changes to width / height settings (line edit widget is not list-style)
        QStringList widthValues;
        QStringList heightValues;

        widthValues << "640" << "800" << "1024" << "1440";
        heightValues << "480" << "600" << "768" << "900";

        preDefined->proxyMap().insert("Width", widthValues);
        preDefined->proxyMap().insert("Height", heightValues);

        preDefined->setWidgetType (CSVSettings::Widget_ComboBox);

        width->setWidgetType (CSVSettings::Widget_LineEdit);
        height->setWidgetType (CSVSettings::Widget_LineEdit);
    }

    section = "Display Format";
    {
        QString defaultValue = "Icon and Text";
        Setting *recordStatusDisplay = createSetting("Record Status Display", section, defaultValue);
        Setting *refIdDisplay = createSetting("Referenceable ID Type Display", section, defaultValue);

        QStringList values;
        values << defaultValue << "Icon Only" << "Text Only";

        recordStatusDisplay->setValueList(values);
        refIdDisplay->setValueList(values);

        recordStatusDisplay->setWidgetType (CSVSettings::Widget_RadioButton);
        refIdDisplay->setWidgetType (CSVSettings::Widget_RadioButton);
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
        const CSMSettings::SettingData *setting = mSettingModel->getSetting(i);

        if (section != setting->section())
        {
            section = setting->section();
            *stream << "[" << section << "]" << "\n";
        }

        *stream << setting->name() << " = " << setting->value() << "\n";
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

bool CSMSettings::UserSettings::loadSettingsFromFile (const QString &filePath)
{
    if (filePath.isEmpty())
        return false;

    QTextStream *stream = openFileStream (filePath, true);

    if (!stream)
        return false;

    //regExp for section names
    QRegExp sectionRe("^\\[([^]]+)\\]");

    //regExp for setting definitions (name / value pair separated by '=')
    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    QString sectionName = "none";

    QMap <QString, QStringList> settings;

    while (!stream->atEnd())
    {
        QString line = stream->readLine().simplified();

        //skip if no data or comment found
        if (line.isEmpty() || line.startsWith("#"))
            continue;

        // save the section name and skip
        if (sectionRe.exactMatch(line))
        {
            if (settings.count()>0)
            {
                //merge (overwrite) settings from the file to
                //the class member.  This ensures duplicate setting
                //definitions in later cfg files will overwrite earlier
                //entries.
                foreach (const QString &key, settings.keys())
                    mSettingDefinitions.insert(key, settings.value(key));

                settings.clear();
            }
            sectionName = sectionRe.cap(1).simplified();
            continue;
        }

        // parse setting definition, assuming valid section
        if ( (keyRe.indexIn(line) != -1) && (sectionName != "none"))
        {
            QString settingName = keyRe.cap(1).simplified();
            QString settingValue = keyRe.cap(2).simplified();
            QString mapKey = sectionName + "." + settingName;

            // test for missing setting...
            if (!(mSettingDeclarations.contains(mapKey)))
            {
                qDebug ("\nUndefined setting found.\n    File: %s\n    Section: %s\n    Definition: %s = %s",
                        filePath.toStdString().c_str(),
                        sectionName.toStdString().c_str(),
                        settingName.toStdString().c_str(),
                        settingValue.toStdString().c_str());
                continue;
            }

            settings[mapKey].append(settingValue);

            qDebug ("\nSetting %s value set to %s\n",
                    settingName.toStdString().c_str(),
                    settingValue.toStdString().c_str());
         }
    }
    if (settings.count()>0)
    {
        //merge (overwrite) settings from the file to
        //the class member.  This ensures duplicate setting
        //definitions in later cfg files will overwrite earlier
        //entries.
        foreach (const QString &key, settings.keys())
            mSettingDefinitions.insert(key, settings.value(key));

        settings.clear();
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

    qDebug() << "filepaths: " << "\n\tglobal: " << globalFilePath;
    qDebug() << "\n\tlocal: " << localFilePath;
    qDebug() << "\n\tuser: " << mUserFilePath;

    if (!(localOk || globalOk))
    {
        QString message = QObject::tr("<br><b>Could not open user settings files for reading</b><br><br> \
                Global and local settings files could not be read.\
                You may have incorrect file permissions or the OpenCS installation may be corrupted.<br>");

        message += QObject::tr("<br>Global filepath: ") + globalFilePath;
        message += QObject::tr("<br>Local filepath: ") + localFilePath;

        displayFileErrorMessage ( message, true);

        return;
    }

    //iterate the settings that were defined in the config files
    //and add them to the model by section and setting name
    //multiple values for a setting are added as separate data elements
    //in the final model
    foreach (const QString &key, mSettingDefinitions.keys())
    {
        int delimiter = key.indexOf(".");

        QString sectionName = key.left(delimiter);
        QString settingName = key.mid(delimiter + 1);

        foreach (const QString &value, mSettingDefinitions.value(key))
        {
            const QStringList &valueList = mSettingDeclarations.value(key)->valueList();
            mSettingModel->createSetting (settingName, sectionName, value, valueList);
        }
    }
}

CSMSettings::Setting *CSMSettings::UserSettings::createSetting(const QString &name,
                                                               const QString &section,
                                                               const QString &defaultValue)
{
    Setting *setting = new Setting (name, section, defaultValue);
    mSettingDeclarations[section + "." + name] = setting;

    return setting;
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
