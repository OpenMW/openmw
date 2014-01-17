#include "unshieldworker.hpp"

#include <QDebug>

#include <QFileInfo>
#include <QFileInfoListIterator>
#include <QMessageBox>
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
    mInstallMorrowind = install;
}

void Wizard::UnshieldWorker::setInstallTribunal(bool install)
{
    mInstallTribunal = install;
}

void Wizard::UnshieldWorker::setInstallBloodmoon(bool install)
{
    mInstallBloodmoon = install;
}

void Wizard::UnshieldWorker::setPath(const QString &path)
{
    mPath = path;
}

void Wizard::UnshieldWorker::setIniPath(const QString &path)
{
    mIniPath = path;
}

void Wizard::UnshieldWorker::setIniCodec(QTextCodec *codec)
{
    mIniCodec = codec;
}

void Wizard::UnshieldWorker::setupSettings()
{
    // Create Morrowind.ini settings map
    if (mIniPath.isEmpty())
        return;

    QFile file(mIniPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // TODO: Emit error signal
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error opening Morrowind configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                   Please make sure you have the right permissions \
                                   and try again.<br>").arg(file.fileName()));
                                   msgBox.exec();
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
            copyDirectory(info.absoluteFilePath(), mPath + QDir::separator() + info.fileName());
        }
    }
}

void Wizard::UnshieldWorker::extract()
{
    emit textChanged(QLatin1String("Starting installation"));
    emit textChanged(QLatin1String("Installation target: ") + mPath);

    QString diskPath("/mnt/cdrom/");
    QDir disk(diskPath);

    // Create temporary extract directory
    // TODO: Use QTemporaryDir in Qt 5.0
    QString tempPath(mPath + QDir::separator() + QLatin1String("extract-temp"));
    QDir temp;

    // Make sure the temporary folder is empty
    removeDirectory(tempPath);

    if (!temp.mkpath(tempPath)) {
        qDebug() << "Can't make path";
        return;
    }

    temp.setPath(tempPath);
    disk.setPath(diskPath);

    if (mInstallMorrowind)
    {
        emit textChanged(QLatin1String("Installing Morrowind\n"));

        if (!temp.mkdir(QLatin1String("morrowind"))) {
            qDebug() << "Can't make dir";
            return;
        }

        if (!temp.cd(QLatin1String("morrowind"))) {
            qDebug() << "Can't cd to dir";
            return;
        }

        if (!disk.exists(QLatin1String("data1.hdr"))) {
            qDebug() << "No data found!";
            return;
        }

        // Extract the installation files
        extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath());

        // TODO: Throw error;
        // Move the files from the temporary path to the destination folder

        //qDebug() << "rename: " << morrowindTempPath << " to: " << mPath;
        if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), mPath))
            qDebug() << "failed!";

        // Install files outside of cab archives
        qDebug() << temp.absolutePath() << disk.absolutePath();
        installDirectories(disk.absolutePath());

        // Copy Morrowind configuration file
        QString iniPath(temp.absoluteFilePath(QLatin1String("App Executables")));
        iniPath.append(QDir::separator() + QLatin1String("Morrowind.ini"));

        QFileInfo info(iniPath);

        qDebug() << info.absoluteFilePath() << mPath;

        if (info.exists()) {
            emit textChanged(tr("Extracting: Morrowind.ini"));
            moveFile(info.absoluteFilePath(), mPath + QDir::separator() + QLatin1String("Morrowind.ini"));
        } else {
            qDebug() << "Could not find ini file!";
        }

        mMorrowindDone = true;

    }

    temp.setPath(tempPath);
    disk.setPath(diskPath);

    if (mInstallTribunal)
    {
        emit textChanged(QLatin1String("Installing Tribunal\n"));

        if (!temp.mkdir(QLatin1String("tribunal"))) {
            qDebug() << "Can't make dir";
            return;
        }

        if (!temp.cd(QLatin1String("tribunal"))) {
            qDebug() << "Can't cd to dir";
            return;
        }

        if (!disk.cd(QLatin1String("Tribunal")))
            qDebug() << "Show file selector";

        if (!disk.exists(QLatin1String("data1.hdr"))) {
            qDebug() << "No data found!";
            return;
        }

        // Extract the installation files
        extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath());

        if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), mPath))
            qDebug() << "failed!";

        // Install files outside of cab archives
        installDirectories(disk.absolutePath());

        mTribunalDone = true;
    }

    temp.setPath(tempPath);
    disk.setPath(diskPath);

    if (mInstallBloodmoon)
    {
        emit textChanged(QLatin1String("Installing Bloodmoon\n"));

        if (!temp.mkdir(QLatin1String("bloodmoon"))) {
            qDebug() << "Can't make dir";
            return;
        }

        if (!temp.cd(QLatin1String("bloodmoon"))) {
            qDebug() << "Can't cd to dir";
            return;
        }

        if (!disk.cd(QLatin1String("Bloodmoon")))
            qDebug() << "Show file selector";

        if (!disk.exists(QLatin1String("data1.hdr"))) {
            qDebug() << "No data found!";
            return;
        }

        // Extract the installation files
        extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath());

        if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), mPath))
            qDebug() << "failed!";

        // Install files outside of cab archives
        installDirectories(disk.absolutePath());

        mBloodmoonDone = true;
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

    qDebug() << "path is: " << path << QString::fromLatin1(unshield_directory_name(unshield, directory)) + QLatin1Char('/');
    // Ensure the target path exists
    QDir dir;
    dir.mkpath(path);

    QString fileName(path);
    fileName.append(QString::fromLatin1(unshield_file_name(unshield, index)));

    // Calculate the percentage done
    int progress = qFloor(((float) counter / (float) unshield_file_count(unshield)) * 100);

    if (mMorrowindDone)
        progress = progress + 100;

    if (mTribunalDone)
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

void Wizard::UnshieldWorker::extractCab(const QString &cabFile, const QString &outputDir)
{
    Unshield *unshield;
    unshield = unshield_open(cabFile.toLatin1().constData());

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
