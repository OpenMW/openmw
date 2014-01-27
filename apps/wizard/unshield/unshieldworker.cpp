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

#include <qmath.h>

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

void Wizard::UnshieldWorker::setInstallMorrowind(bool install)
{
    QWriteLocker writeLock(&mLock);
    mInstallMorrowind = install;
}

void Wizard::UnshieldWorker::setInstallTribunal(bool install)
{
    QWriteLocker writeLock(&mLock);
    mInstallTribunal = install;
}

void Wizard::UnshieldWorker::setInstallBloodmoon(bool install)
{
    QWriteLocker writeLock(&mLock);
    mInstallBloodmoon = install;
}

bool Wizard::UnshieldWorker::getInstallMorrowind()
{
    QReadLocker readLock(&mLock);
    return mInstallMorrowind;
}

bool Wizard::UnshieldWorker::getInstallTribunal()
{
    QReadLocker readLock(&mLock);
    return mInstallTribunal;
}

bool Wizard::UnshieldWorker::getInstallBloodmoon()
{
    QReadLocker readLock(&mLock);
    return mInstallBloodmoon;
}

void Wizard::UnshieldWorker::setMorrowindPath(const QString &path)
{
    QWriteLocker writeLock(&mLock);
    mMorrowindPath = path;
    mWait.wakeAll();
}

void Wizard::UnshieldWorker::setTribunalPath(const QString &path)
{
    QWriteLocker writeLock(&mLock);
    mTribunalPath = path;
    mWait.wakeAll();

}

void Wizard::UnshieldWorker::setBloodmoonPath(const QString &path)
{
    QWriteLocker writeLock(&mLock);
    mBloodmoonPath = path;
    mWait.wakeAll();

}

QString Wizard::UnshieldWorker::getMorrowindPath()
{
    QReadLocker readLock(&mLock);
    return mMorrowindPath;
}

QString Wizard::UnshieldWorker::getTribunalPath()
{
    QReadLocker readLock(&mLock);
    return mTribunalPath;
}

QString Wizard::UnshieldWorker::getBloodmoonPath()
{
    QReadLocker readLock(&mLock);
    return mBloodmoonPath;
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

void Wizard::UnshieldWorker::setMorrowindDone(bool done)
{
    QWriteLocker writeLock(&mLock);
    mMorrowindDone = done;
}

void Wizard::UnshieldWorker::setTribunalDone(bool done)
{
    QWriteLocker writeLock(&mLock);
    mTribunalDone = done;
}

void Wizard::UnshieldWorker::setBloodmoonDone(bool done)
{
    QWriteLocker writeLock(&mLock);
    mBloodmoonDone = done;
}

bool Wizard::UnshieldWorker::getMorrowindDone()
{
    QReadLocker readLock(&mLock);
    return mMorrowindDone;
}

bool Wizard::UnshieldWorker::getTribunalDone()
{
    QReadLocker readLock(&mLock);
    return mTribunalDone;
}

bool Wizard::UnshieldWorker::getBloodmoonDone()
{
    QReadLocker readLock(&mLock);
    return mBloodmoonDone;
}

void Wizard::UnshieldWorker::setupSettings()
{
    // Create Morrowind.ini settings map
    if (getIniPath().isEmpty())
        return;

    QFile file(getIniPath());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // TODO: Emit error signal
        qDebug() << "Error opening .ini file!";
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
             qDebug() << "moving: " << info.absoluteFilePath() <<  " to: " << destDir.absolutePath() + relativePath;

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

    if (getInstallMorrowind())
    {
        while (!getMorrowindDone())
        {
            if (getMorrowindPath().isEmpty()) {
                qDebug() << "request file dialog";
                QReadLocker readLock(&mLock);
                emit requestFileDialog(QLatin1String("Morrowind"));
                mWait.wait(&mLock);
            }

            if (!getMorrowindPath().isEmpty()) {
                disk.setPath(getMorrowindPath());

                if (!findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Morrowind.bsa"))
                        | findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Tribunal.bsa"))
                        | findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Bloodmoon.bsa")))
                {
                    QReadLocker readLock(&mLock);
                    emit requestFileDialog(QLatin1String("Morrowind"));
                    mWait.wait(&mLock);
                } else {
                    if (installMorrowind()) {
                        setMorrowindDone(true);
                    } else {
                        qDebug() << "Erorr installing Morrowind";
                        return;
                    }
                }
            }
        }
    }

    if (getInstallTribunal())
    {
        while (!getTribunalDone())
        {
            if (getTribunalPath().isEmpty()) {
                qDebug() << "request file dialog";
                QReadLocker locker(&mLock);
                emit requestFileDialog(QLatin1String("Tribunal"));
                mWait.wait(&mLock);
            }

            if (!getTribunalPath().isEmpty()) {
                disk.setPath(getTribunalPath());

                if (!findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Tribunal.bsa")))
                {
                    qDebug() << "found";
                    QReadLocker locker(&mLock);
                    emit requestFileDialog(QLatin1String("Tribunal"));
                    mWait.wait(&mLock);
                } else {
                    if (installTribunal()) {
                        setTribunalDone(true);
                    } else {
                        qDebug() << "Erorr installing Tribunal";
                        return;
                    }
                }
            }
        }
    }

    if (getInstallBloodmoon())
    {
        while (!getBloodmoonDone())
        {
            if (getBloodmoonPath().isEmpty()) {
                qDebug() << "request file dialog";
                QReadLocker locker(&mLock);
                emit requestFileDialog(QLatin1String("Bloodmoon"));
                mWait.wait(&mLock);
            }

            if (!getBloodmoonPath().isEmpty()) {
                disk.setPath(getBloodmoonPath());

                if (!findFile(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Bloodmoon.bsa")))
                {
                    QReadLocker locker(&mLock);
                    emit requestFileDialog(QLatin1String("Bloodmoon"));
                    mWait.wait(&mLock);
                } else {
                    if (installBloodmoon()) {
                        setBloodmoonDone(true);
                    } else {
                        qDebug() << "Erorr installing Bloodmoon";
                        return;
                    }
                }
            }
        }
    }

    int total = 0;

    if (mInstallMorrowind)
        total = 100;

    if (mInstallTribunal)
        total = total + 100;

    if (mInstallBloodmoon)
        total = total + 100;

    emit textChanged(tr("Installation finished!"));
    emit progressChanged(total);
    emit finished();

    qDebug() << "installation finished!";
}

bool Wizard::UnshieldWorker::installMorrowind()
{
    qDebug() << "install morrowind!";
    emit textChanged(QLatin1String("Installing Morrowind"));

    QDir disk(getMorrowindPath());

    if (!disk.exists()) {
        qDebug() << "getMorrowindPath: " << getMorrowindPath();
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
        return false;
    }

    temp.setPath(tempPath);

    if (!temp.mkdir(QLatin1String("morrowind"))) {
        qDebug() << "Can't make dir";
        return false;
    }

    if (!temp.cd(QLatin1String("morrowind"))) {
        qDebug() << "Can't cd to dir";
        return false;
    }

    // Extract the installation files
    extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath());

    // TODO: Throw error;
    // Move the files from the temporary path to the destination folder
    emit textChanged(tr("Moving installation files"));
    if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), getPath())) {
        qDebug() << "failed to move files!";
        return false;
    }

    // Install files outside of cab archives
    installDirectories(disk.absolutePath());

    // Copy Morrowind configuration file
    QString iniPath(temp.absoluteFilePath(QLatin1String("App Executables")));
    iniPath.append(QDir::separator() + QLatin1String("Morrowind.ini"));

    QFileInfo info(iniPath);

    qDebug() << info.absoluteFilePath() << getPath();

    if (info.exists()) {
        emit textChanged(tr("Extracting: Morrowind.ini"));
        moveFile(info.absoluteFilePath(), getPath() + QDir::separator() + QLatin1String("Morrowind.ini"));
    } else {
        qDebug() << "Could not find ini file!";
        return false;
    }

    emit textChanged(tr("Morrowind installation finished!"));
    return true;
}

bool Wizard::UnshieldWorker::installTribunal()
{
    emit textChanged(QLatin1String("Installing Tribunal"));

    QDir disk(getTribunalPath());

    if (!disk.exists()) {
        qDebug() << "disk does not exist! " << disk.absolutePath() << getTribunalPath();
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
        return false;
    }

    temp.setPath(tempPath);

    if (!temp.mkdir(QLatin1String("tribunal"))) {
        qDebug() << "Can't make dir";
        return false;
    }

    if (!temp.cd(QLatin1String("tribunal"))) {
        qDebug() << "Can't cd to dir";
        return false;
    }

    // Extract the installation files
    extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath());

    // TODO: Throw error;
    // Move the files from the temporary path to the destination folder
    emit textChanged(tr("Moving installation files"));
    if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), getPath())) {
        qDebug() << "failed to move files!";
        return false;
    }

    // Install files outside of cab archives
    installDirectories(disk.absolutePath());

    emit textChanged(tr("Tribunal installation finished!"));
    return true;
}

bool Wizard::UnshieldWorker::installBloodmoon()
{
    emit textChanged(QLatin1String("Installing Bloodmoon"));

    QDir disk(getBloodmoonPath());

    if (!disk.exists()) {
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
        return false;
    }

    temp.setPath(tempPath);

    if (!temp.mkdir(QLatin1String("bloodmoon"))) {
        qDebug() << "Can't make dir";
        return false;
    }

    if (!temp.cd(QLatin1String("bloodmoon"))) {
        qDebug() << "Can't cd to dir";
        return false;
    }

    // Extract the installation files
    extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath());

    // TODO: Throw error;
    // Move the files from the temporary path to the destination folder
    emit textChanged(tr("Moving installation files"));
    if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), getPath())) {
        qDebug() << "failed to move files!";
        return false;
    }

    // Install files outside of cab archives
    installDirectories(disk.absolutePath());

    QFileInfo patch(temp.absoluteFilePath(QLatin1String("Tribunal Patch") + QDir::separator() + QLatin1String("Tribunal.esm")));
    QFileInfo original(getPath() + QDir::separator() + QLatin1String("Tribunal.esm"));

    if (original.exists() && patch.exists()) {
        emit textChanged(tr("Extracting: Tribunal patch"));
        copyFile(patch.absoluteFilePath(), original.absoluteFilePath());
    }

    emit textChanged(tr("Bloodmoon installation finished!"));
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

    if (getMorrowindDone())
        progress = progress + 100;

    if (getTribunalDone())
        progress = progress + 100;

    qDebug() << progress << counter << unshield_file_count(unshield);

    emit textChanged(tr("Extracting: %1").arg(QString::fromLatin1(unshield_file_name(unshield, index))));
    emit progressChanged(progress);

    success = unshield_file_save(unshield, index, fileName.toLatin1().constData());

    if (!success) {
        emit error(tr("Failed to extract %1").arg(fileName));
        dir.remove(fileName);
    }

    return success;
}

bool Wizard::UnshieldWorker::findFile(const QString &cabFile, const QString &fileName)
{
    Unshield *unshield;
    unshield = unshield_open(cabFile.toLatin1().constData());

    // TODO: Proper error
    if (!unshield) {
        emit error(tr("Failed to open %1").arg(cabFile));
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

void Wizard::UnshieldWorker::extractCab(const QString &cabFile, const QString &outputDir)
{
    Unshield *unshield;
    unshield = unshield_open(cabFile.toLatin1().constData());

    // TODO: Proper error
    if (!unshield) {
        emit error(tr("Failed to open %1").arg(cabFile));
        return;
    }

    int counter = 0;

    for (int i=0; i<unshield_file_group_count(unshield); ++i)
    {
        UnshieldFileGroup *group = unshield_file_group_get(unshield, i);

        for (size_t j=group->first_file; j<=group->last_file; ++j)
        {
            if (unshield_file_is_valid(unshield, j)) {
                extractFile(unshield, outputDir, group->name, j, counter);
                ++counter;
            }
        }
    }

    unshield_close(unshield);
}
