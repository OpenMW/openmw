#include "usersettings.hpp"

#include <QTextStream>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QMap>
#include <QMessageBox>
#include <QTextCodec>

#include <QFile>

#include <components/files/configurationmanager.hpp>
#include "settingcontainer.hpp"
#include <boost/version.hpp>

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
    {
        stream = new QTextStream(file);
        stream->setCodec(QTextCodec::codecForName("UTF-8"));
    }

    return stream;

}

bool CSMSettings::UserSettings::writeSettings(QMap<QString, CSMSettings::SettingList *> &settings)
{
    QTextStream *stream = openFileStream(mUserFilePath);

    bool success = (stream);

    if (success)
    {
        QList<QString> keyList = settings.keys();

        foreach (QString key, keyList)
        {
            SettingList *sectionSettings = settings[key];

            *stream << "[" << key << "]" << '\n';

            foreach (SettingContainer *item, *sectionSettings)
                *stream << item->objectName() << " = " << item->getValue() << '\n';
        }

        stream->device()->close();
        delete stream;
        stream = 0;
    }
    else
    {
        displayFileErrorMessage(mReadWriteMessage, false);
    }

    return (success);
}


const CSMSettings::SectionMap &CSMSettings::UserSettings::getSettings() const
{
    return mSectionSettings;
}

bool CSMSettings::UserSettings::loadFromFile(const QString &filePath)
{
    if (filePath.isEmpty())
        return false;

    mSectionSettings.clear();

    QTextStream *stream = openFileStream (filePath, true);

    bool success = (stream);

    if (success)
    {
        //looks for a square bracket, "'\\["
        //that has one or more "not nothing" in it, "([^]]+)"
        //and is closed with a square bracket, "\\]"

        QRegExp sectionRe("^\\[([^]]+)\\]");

        //Find any character(s) that is/are not equal sign(s), "[^=]+"
        //followed by an optional whitespace, an equal sign, and another optional whitespace, "\\s*=\\s*"
        //and one or more periods, "(.+)"

        QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

        CSMSettings::SettingMap *settings = 0;
        QString section = "none";

        while (!stream->atEnd())
        {
            QString line = stream->readLine().simplified();

            if (line.isEmpty() || line.startsWith("#"))
                continue;

            //if a section is found, push it onto a new QStringList
            //and push the QStringList onto
            if (sectionRe.exactMatch(line))
            {
                //add the previous section's settings to the member map
                if (settings)
                    mSectionSettings.insert(section, settings);

                //save new section and create a new list
                section = sectionRe.cap(1);
                settings = new SettingMap;
                continue;
            }

            if (keyRe.indexIn(line) != -1)
            {
                SettingContainer *sc  = new SettingContainer (keyRe.cap(2).simplified());
                sc->setObjectName(keyRe.cap(1).simplified());
                (*settings)[keyRe.cap(1).simplified()]  = sc;
            }

        }

        mSectionSettings.insert(section, settings);

        stream->device()->close();
        delete stream;
        stream = 0;
    }

    return success;
}

void CSMSettings::UserSettings::loadSettings (const QString &fileName)
{
    //global
    QString globalFilePath = QString::fromStdString(mCfgMgr.getGlobalPath().string()) + fileName;
    bool globalOk = loadFromFile(globalFilePath);


    //local
    QString localFilePath = QString::fromStdString(mCfgMgr.getLocalPath().string()) + fileName;
    bool localOk = loadFromFile(localFilePath);

    //user
    mUserFilePath = QString::fromStdString(mCfgMgr.getUserPath().string()) + fileName;
    loadFromFile(mUserFilePath);

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

void CSMSettings::UserSettings::updateSettings (const QString &sectionName, const QString &settingName)
{
    if (mSectionSettings.find(sectionName) == mSectionSettings.end())
        return;

    SettingMap *settings = mSectionSettings.value(sectionName);

    if (settingName.isEmpty())
    {
        foreach (const SettingContainer *setting, *settings)
            emit signalUpdateEditorSetting (setting->objectName(), setting->getValue());
    }
    else
    {
        if (settings->find(settingName) != settings->end())
        {
            const SettingContainer *setting = settings->value(settingName);
            emit signalUpdateEditorSetting (setting->objectName(), setting->getValue());
        }
    }
}

QString CSMSettings::UserSettings::getSetting (const QString &section, const QString &setting) const
{
    QString retVal = "";

    if (mSectionSettings.find(section) != mSectionSettings.end())
    {
        CSMSettings::SettingMap *settings = mSectionSettings.value(section);

        if (settings->find(setting) != settings->end())
            retVal = settings->value(setting)->getValue();
    }

    return retVal;
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
