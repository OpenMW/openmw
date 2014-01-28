#include "unshieldworker.hpp"

#include <QDebug>

#include <QReadLocker>
#include <QWriteLocker>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoListIterator>
#include <QStringList>
#include <QTextStream>
#include <QTextCodec>
#include <QFile>
#include <QDir>
#include <QDirIterator>

Wizard::UnshieldWorker::UnshieldWorker(QObject *parent) :
    QObject(parent),
    mIniSettings()
{
    unshield_set_log_level(0);

    mMorrowindPath = QString();
    mTribunalPath = QString();
    mBloodmoonPath = QString();

    mPath = QString();
    mIniPath = QString();

    // Default to Latin encoding
    mIniCodec = QTextCodec::codecForName("windows-1252");

    mInstallMorrowind = false;
    mInstallTribunal = false;
    mInstallBloodmoon = false;

    mMorrowindDone = false;
    mTribunalDone = false;
    mBloodmoonDone = false;
}

Wizard::UnshieldWorker::~UnshieldWorker()
{

}

void Wizard::UnshieldWorker::setInstallComponent(Wizard::Component component, bool install)
{
    QWriteLocker writeLock(&mLock);
    switch (component) {

    case Wizard::Component_Morrowind:
        mInstallMorrowind = install;
        break;
    case Wizard::Component_Tribunal:
        mInstallTribunal = install;
        break;
    case Wizard::Component_Bloodmoon:
        mInstallBloodmoon = install;
        break;
    }
}

bool Wizard::UnshieldWorker::getInstallComponent(Component component)
{
    QReadLocker readLock(&mLock);
    switch (component) {

    case Wizard::Component_Morrowind:
        return mInstallMorrowind;
    case Wizard::Component_Tribunal:
        return mInstallTribunal;
    case Wizard::Component_Bloodmoon:
        return mInstallBloodmoon;
    }

    return false;
}

void Wizard::UnshieldWorker::setComponentPath(Wizard::Component component, const QString &path)
{
     QWriteLocker writeLock(&mLock);
     switch (component) {

     case Wizard::Component_Morrowind:
         mMorrowindPath = path;
         break;
     case Wizard::Component_Tribunal:
         mTribunalPath = path;
         break;
     case Wizard::Component_Bloodmoon:
         mBloodmoonPath = path;
         break;
     }

     mWait.wakeAll();
}

QString Wizard::UnshieldWorker::getComponentPath(Component component)
{
    QReadLocker readLock(&mLock);
    switch (component) {

    case Wizard::Component_Morrowind:
        return mMorrowindPath;
    case Wizard::Component_Tribunal:
        return mTribunalPath;
    case Wizard::Component_Bloodmoon:
        return mBloodmoonPath;
    }

    return QString();
}

void Wizard::UnshieldWorker::setComponentDone(Component component, bool done)
{
    QWriteLocker writeLock(&mLock);
    switch (component) {

    case Wizard::Component_Morrowind:
        mMorrowindDone = done;
        break;
    case Wizard::Component_Tribunal:
        mTribunalDone = done;
        break;
    case Wizard::Component_Bloodmoon:
        mBloodmoonDone = done;
        break;
    }
}

bool Wizard::UnshieldWorker::getComponentDone(Component component)
{
    QReadLocker readLock(&mLock);
    switch (component)
    {

    case Wizard::Component_Morrowind:
        return mMorrowindDone;
    case Wizard::Component_Tribunal:
        return mTribunalDone;
    case Wizard::Component_Bloodmoon:
        return mBloodmoonDone;
    }

    return false;
}

void Wizard::UnshieldWorker::setPath(const QString &path)
{
    QWriteLocker writeLock(&mLock);
    mPath = path;
}

void Wizard::UnshieldWorker::setIniPath(const QString &path)
{
    QWriteLocker writeLock(&mLock);
    mIniPath = path;
}

QString Wizard::UnshieldWorker::getPath()
{
    QReadLocker readLock(&mLock);
    return mPath;
}

QString Wizard::UnshieldWorker::getIniPath()
{
    QReadLocker readLock(&mLock);
    return mIniPath;
}

void Wizard::UnshieldWorker::setIniCodec(QTextCodec *codec)
{
    QWriteLocker writeLock(&mLock);
    mIniCodec = codec;
}

void Wizard::UnshieldWorker::setupSettings()
{
    // Create Morrowind.ini settings map
    if (getIniPath().isEmpty())
        return;

    QFile file(getIniPath());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening .ini file!";
        emit error(tr("Failed to open Morrowind configuration file!"), tr("Opening %1 failed: %2.").arg(getIniPath(), file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec(mIniCodec);

    mIniSettings.readFile(stream);
}

bool Wizard::UnshieldWorker::removeDirectory(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName))
    {
        QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                               QDir::System | QDir::Hidden |
                                               QDir::AllDirs | QDir::Files, QDir::DirsFirst));
        foreach(QFileInfo info, list) {
            if (info.isDir()) {
                result = removeDirectory(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result)
                return result;
        }

        result = dir.rmdir(dirName);
    }
    return result;
}

bool Wizard::UnshieldWorker::copyFile(const QString &source, const QString &destination, bool keepSource)
{
    QDir dir;
    QFile file;

    QFileInfo info(destination);

    if (info.exists())
        dir.remove(info.absoluteFilePath());

    if (file.copy(source, destination)) {
        if (!keepSource) {
            return file.remove(source);
        } else {
            return true;
        }
    } else {
        qDebug() << "copy failed! " << file.errorString();
        emit error(tr("Failed to copy file!"), tr("Copying %1 to %2 failed: %3.").arg(source, destination, file.errorString()));
    }

    return false;
}

bool Wizard::UnshieldWorker::copyDirectory(const QString &source, const QString &destination, bool keepSource)
{
    QDir sourceDir(source);
    QDir destDir(destination);

    if (!destDir.exists()) {
        sourceDir.mkpath(destination);
        destDir.refresh();
    }

    if (!destDir.exists())
        return false;

    bool result = true;

    QFileInfoList list(sourceDir.entryInfoList(QDir::NoDotAndDotDot |
                                                 QDir::System | QDir::Hidden |
                                                 QDir::AllDirs | QDir::Files, QDir::DirsFirst));

    foreach (const QFileInfo &info, list) {
        QString relativePath(info.absoluteFilePath());
        relativePath.remove(source);

        if (info.isSymLink())
            continue;

        if (info.isDir()) {
            result = moveDirectory(info.absoluteFilePath(), destDir.absolutePath() + relativePath);
        } else {
            result = moveFile(info.absoluteFilePath(), destDir.absolutePath() + relativePath);
        }
    }

    if (!keepSource)
        return result && removeDirectory(sourceDir.absolutePath());

    return result;
}

bool Wizard::UnshieldWorker::moveFile(const QString &source, const QString &destination)
{
    return copyFile(source, destination, false);
}

bool Wizard::UnshieldWorker::moveDirectory(const QString &source, const QString &destination)
{
    return copyDirectory(source, destination, false);
}

void Wizard::UnshieldWorker::installDirectories(const QString &source)
{
    QDir dir(source);

    if (!dir.exists())
        return;

    QStringList directories;
    directories << QLatin1String("Fonts")
                << QLatin1String("Music")
                << QLatin1String("Sound")
                << QLatin1String("Splash")
                << QLatin1String("Video");

    QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                          QDir::System | QDir::Hidden |
                                          QDir::AllDirs));
    foreach(QFileInfo info, list) {
        if (info.isSymLink())
            continue;

        if (directories.contains(info.fileName())) {
            qDebug() << "found " << info.fileName();
            emit textChanged(tr("Extracting: %1 directory").arg(info.fileName()));
            copyDirectory(info.absoluteFilePath(), getPath() + QDir::separator() + info.fileName());
        }
    }

    // Copy the Data Files dir too, but only the subdirectories
    QFileInfo info(dir.absoluteFilePath("Data Files"));
    if (info.exists()) {
        emit textChanged(tr("Extracting: Data Files directory"));
        copyDirectory(info.absoluteFilePath(), getPath());
    }

}

void Wizard::UnshieldWorker::extract()
{
    qDebug() << "extract!";
    QDir disk;

    if (getInstallComponent(Wizard::Component_Morrowind))
    {
        while (!getComponentDone(Wizard::Component_Morrowind))
        {
            if (getComponentPath(Wizard::Component_Morrowind).isEmpty()) {
                qDebug() << "request file dialog";
                QReadLocker readLock(&mLock);
                emit requestFileDialog(Wizard::Component_Morrowind);
                mWait.wait(&mLock);
            }

            if (!getComponentPath(Wizard::Component_Morrowind).isEmpty()) {
                disk.setPath(getComponentPath(Wizard::Component_Morrowind));

                if (!findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Morrowind.bsa"))
                        | findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Tribunal.bsa"))
                        | findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Bloodmoon.bsa")))
                {
                    QReadLocker readLock(&mLock);
                    emit requestFileDialog(Wizard::Component_Morrowind);
                    mWait.wait(&mLock);
                } else {
                    if (installComponent(Wizard::Component_Morrowind)) {
                        setComponentDone(Wizard::Component_Morrowind, true);
                    } else {
                        qDebug() << "Erorr installing Morrowind";

                        return;
                    }
                }
            }
        }
    }

    if (getInstallComponent(Wizard::Component_Tribunal))
    {
        setupAddon(Wizard::Component_Tribunal);
    }

    if (getInstallComponent(Wizard::Component_Bloodmoon))
    {
        setupAddon(Wizard::Component_Bloodmoon);
    }

    // Remove the temporary directory
    removeDirectory(getPath() + QDir::separator() + QLatin1String("extract-temp"));

    // Fill the progress bar
    int total = 0;

    if (getInstallComponent(Wizard::Component_Morrowind))
        total = 100;

    if (getInstallComponent(Wizard::Component_Tribunal))
        total = total + 100;

    if (getInstallComponent(Wizard::Component_Bloodmoon))
        total = total + 100;

    emit textChanged(tr("Installation finished!"));
    emit progressChanged(total);
    emit finished();

    qDebug() << "installation finished!";
}

void Wizard::UnshieldWorker::setupAddon(Component component)
{
    while (!getComponentDone(component))
    {
        QDir disk(getComponentPath(Wizard::Component_Morrowind));
        QString name;
        if (component == Wizard::Component_Tribunal)
            name = QLatin1String("Tribunal");

        if (component == Wizard::Component_Bloodmoon)
            name = QLatin1String("Bloodmoon");

        if (name.isEmpty()) {
            emit error(tr("Component parameter is invalid!"), tr("An invalid component parameter was supplied."));
            return;
        }

        if (!disk.cd(name)) {
            qDebug() << "not found on cd!";
            QReadLocker locker(&mLock);
            emit requestFileDialog(component);
            mWait.wait(&mLock);

        } else if (disk.exists(QLatin1String("data1.hdr"))) {
            qDebug() << "Exists! " << disk.absolutePath();
            setComponentPath(component, disk.absolutePath());
        }

        if (getComponentPath(component).isEmpty()) {
            qDebug() << "request file dialog";
            QReadLocker locker(&mLock);
            emit requestFileDialog(Wizard::Component_Tribunal);
            mWait.wait(&mLock);
        }

        // Make sure the dir is up-to-date
        disk.setPath(getComponentPath(component));

        if (!getComponentPath(component).isEmpty()) {

            if (!findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), name + QLatin1String(".bsa")))
            {
                QReadLocker locker(&mLock);
                emit requestFileDialog(component);
                mWait.wait(&mLock);
            } else {
                // Now do the actual installing
                if (installComponent(component)) {
                    setComponentDone(component, true);
                } else {
                    qDebug() << "Error installing " << name;
                    return;
                }
            }
        }
    }
}

bool Wizard::UnshieldWorker::installComponent(Component component)
{
    QString name;
    switch (component) {

    case Wizard::Component_Morrowind:
        name = QLatin1String("Morrowind");
        break;
    case Wizard::Component_Tribunal:
        name = QLatin1String("Tribunal");
        break;
    case Wizard::Component_Bloodmoon:
        name = QLatin1String("Bloodmoon");
        break;
    }

    if (name.isEmpty()) {
        emit error(tr("Component parameter is invalid!"), tr("An invalid component parameter was supplied."));
        return false;
    }

    emit textChanged(tr("Installing %0").arg(name));

    QDir disk(getComponentPath(component));

    if (!disk.exists()) {
        qDebug() << "Component path not set: " << getComponentPath(Wizard::Component_Morrowind);
        emit error(tr("Component path not set!"), tr("The source path for %0 was not set.").arg(name));
        return false;
    }

    // Create temporary extract directory
    // TODO: Use QTemporaryDir in Qt 5.0
    QString tempPath(getPath() + QDir::separator() + QLatin1String("extract-temp"));
    QDir temp;

    // Make sure the temporary folder is empty
    removeDirectory(tempPath);

    if (!temp.mkpath(tempPath)) {
        qDebug() << "Can't make path";
        emit error(tr("Cannot create temporary directory!"), tr("Failed to create %0.").arg(tempPath));
        return false;
    }

    temp.setPath(tempPath);

    if (!temp.mkdir(name)) {
        emit error(tr("Cannot create temporary directory!"), tr("Failed to create %0.").arg(temp.absoluteFilePath(name)));
        return false;
    }

    if (!temp.cd(name)) {
        qDebug() << "Can't cd to dir";
        emit error(tr("Cannot move into temporary directory!"), tr("Failed to move into %0.").arg(temp.absoluteFilePath(name)));
        return false;
    }

    // Extract the installation files
    if (!extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath()))
        return false;

    // Move the files from the temporary path to the destination folder
    emit textChanged(tr("Moving installation files"));
    if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), getPath())) {
        qDebug() << "failed to move files!";
        emit error(tr("Moving extracted files failed!"),
                   tr("Failed to move files from %0 to %1.").arg(temp.absoluteFilePath(QLatin1String("Data Files")),
                                                                getPath()));
        return false;
    }

    // Install files outside of cab archives
    installDirectories(disk.absolutePath());

    if (component == Wizard::Component_Morrowind)
    {
        // Copy Morrowind configuration file
        QString iniPath(temp.absoluteFilePath(QLatin1String("App Executables")));
        iniPath.append(QDir::separator() + QLatin1String("Morrowind.ini"));

        QFileInfo info(iniPath);

        if (info.exists()) {
            emit textChanged(tr("Extracting: Morrowind.ini"));
            moveFile(info.absoluteFilePath(), getPath() + QDir::separator() + QLatin1String("Morrowind.ini"));
        } else {
            qDebug() << "Could not find ini file!";
            emit error(tr("Could not find Morrowind configuration file!"), tr("Failed to find %0.").arg(iniPath));
            return false;
        }
    }

    if (component == Wizard::Component_Bloodmoon)
    {
        QFileInfo patch(temp.absoluteFilePath(QLatin1String("Tribunal Patch") + QDir::separator() + QLatin1String("Tribunal.esm")));
        QFileInfo original(getPath() + QDir::separator() + QLatin1String("Tribunal.esm"));

        if (original.exists() && patch.exists()) {
            emit textChanged(tr("Extracting: Tribunal patch"));
            copyFile(patch.absoluteFilePath(), original.absoluteFilePath());
        }

    }

    emit textChanged(tr("%0 installation finished!").arg(name));
    return true;

}

bool Wizard::UnshieldWorker::extractFile(Unshield *unshield, const QString &outputDir, const QString &prefix, int index, int counter)
{
    bool success;
    QString path(outputDir);
    path.append(QDir::separator());

    int directory = unshield_file_directory(unshield, index);

    if (!prefix.isEmpty())
        path.append(prefix + QDir::separator());

    if (directory >= 0)
        path.append(QString::fromLatin1(unshield_directory_name(unshield, directory)) + QDir::separator());

    // Ensure the path has the right separators
    path.replace(QLatin1Char('\\'), QDir::separator());
    path = QDir::toNativeSeparators(path);

    // Ensure the target path exists
    QDir dir;
    dir.mkpath(path);

    QString fileName(path);
    fileName.append(QString::fromLatin1(unshield_file_name(unshield, index)));

    // Calculate the percentage done
    int progress = (((float) counter / (float) unshield_file_count(unshield)) * 100);

    if (getComponentDone(Wizard::Component_Morrowind))
        progress = progress + 100;

    if (getComponentDone(Wizard::Component_Tribunal))
        progress = progress + 100;

    emit textChanged(tr("Extracting: %1").arg(QString::fromLatin1(unshield_file_name(unshield, index))));
    emit progressChanged(progress);

    success = unshield_file_save(unshield, index, fileName.toLatin1().constData());

    if (!success) {
        emit error(tr("Failed to extract %1.").arg(QString::fromLatin1(unshield_file_name(unshield, index))), tr("Complete path: %1.").arg(fileName));
        dir.remove(fileName);
    }

    return success;
}

bool Wizard::UnshieldWorker::findFile(const QString &cabFile, const QString &fileName)
{
    Unshield *unshield;
    unshield = unshield_open(cabFile.toLatin1().constData());

    if (!unshield) {
        emit error(tr("Failed to open InstallShield Cabinet File."), tr("Opening %1 failed.").arg(cabFile));
        return false;
    }

    for (int i=0; i<unshield_file_group_count(unshield); ++i)
    {
        UnshieldFileGroup *group = unshield_file_group_get(unshield, i);

        for (size_t j=group->first_file; j<=group->last_file; ++j)
        {
            QString current(QString::fromLatin1(unshield_file_name(unshield, j)));

            qDebug() << "File is: " << unshield_file_name(unshield, j);
            if (current == fileName)
                return true; // File is found!
        }
    }

    unshield_close(unshield);
    return false;
}

bool Wizard::UnshieldWorker::extractCab(const QString &cabFile, const QString &outputDir)
{
    bool success;

    Unshield *unshield;
    unshield = unshield_open(cabFile.toLatin1().constData());

    if (!unshield) {
        emit error(tr("Failed to open InstallShield Cabinet File."), tr("Opening %1 failed.").arg(cabFile));
        return false;
    }

    int counter = 0;

    for (int i=0; i<unshield_file_group_count(unshield); ++i)
    {
        UnshieldFileGroup *group = unshield_file_group_get(unshield, i);

        for (size_t j=group->first_file; j<=group->last_file; ++j)
        {
            if (unshield_file_is_valid(unshield, j)) {
                success = extractFile(unshield, outputDir, group->name, j, counter);
                ++counter;
            }
        }
    }

    unshield_close(unshield);
    return success;
}
