#include "usersettings.hpp"

#include <QTextStream>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QMap>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>

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


CSMSettings::UserSettings::UserSettings()
{
    mUserSettingsInstance = this;
}

CSMSettings::UserSettings::~UserSettings()
{
}

QFile *CSMSettings::UserSettings::openFile (const QString &filename)
{
    QFile *file = new QFile(filename);

    bool success = (file->open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) ;

    if (!success)
    {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(QObject::tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(QObject::tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file->fileName()));
        msgBox.exec();
        delete file;
        file = 0;
    }

    return file;
}

bool CSMSettings::UserSettings::writeFile(QFile *file, QMap<QString, CSMSettings::SettingList *> &settings)
{
    if (!file)
        return false;

    QTextStream stream(file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    QList<QString> keyList = settings.keys();

    foreach (QString key, keyList)
    {
        SettingList *sectionSettings = settings[key];

        stream << "[" << key << "]" << '\n';

        foreach (SettingContainer *item, *sectionSettings)
            stream << item->getName() << " = " << item->getValue() << '\n';
    }

    file->close();

    return true;
}

void CSMSettings::UserSettings::getSettings(QTextStream &stream, SectionMap &sections)
{
    //looks for a square bracket, "'\\["
    //that has one or more "not nothing" in it, "([^]]+)"
    //and is closed with a square bracket, "\\]"

    QRegExp sectionRe("^\\[([^]]+)\\]");

    //Find any character(s) that is/are not equal sign(s), "[^=]+"
    //followed by an optional whitespace, an equal sign, and another optional whirespace, "\\s*=\\s*"
    //and one or more periods, "(.+)"

    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    CSMSettings::SettingMap *settings = 0;
    QString section = "none";

    while (!stream.atEnd())
    {
        QString line = stream.readLine().simplified();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        //if a section is found, push it onto a new QStringList
        //and push the QStringList onto
        if (sectionRe.exactMatch(line))
        {
            //add the previous section's settings to the member map
            if (settings)
                sections.insert(section, settings);

            //save new section and create a new list
            section = sectionRe.cap(1);
            settings = new SettingMap;
            continue;
        }

        if (keyRe.indexIn(line) != -1)
        {
            SettingContainer *sc  = new SettingContainer (keyRe.cap(2).simplified());
            (*settings)[keyRe.cap(1).simplified()]  = sc;
        }

    }
    sections.insert(section, settings);
}

QString CSMSettings::UserSettings::getSettingValue(QString section, QString setting)
{
    Files::ConfigurationManager configMgr;
    QString userPath = QString::fromStdString(configMgr.getUserPath().string());
    QStringList list;

    list.append(QString("opencs.cfg"));
    list.append(userPath + QString("opencs.cfg"));


    CSMSettings::SectionMap sectionMap;

    foreach (const QString &path, list)
    {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);

        if (file.exists())
        {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenCS configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return QString();
            }

            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            getSettings(stream, sectionMap);
        }

        file.close();
    }

    if(sectionMap.find(section) == sectionMap.end())
        return QString();

    CSMSettings::SettingMap *settings = sectionMap.value(section);

    if(settings->find(setting) == settings->end())
        return QString();

    CSMSettings::SettingContainer *settingContainer = settings->value(setting);

    return settingContainer->getValue();
}

