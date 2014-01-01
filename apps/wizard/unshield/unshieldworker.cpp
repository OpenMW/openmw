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

bool Wizard::UnshieldWorker::moveFile(const QString &source, const QString &destination)
{
    QDir dir;
    QFile file;

    if (dir.rename(source, destination)) {
        return true;
    } else {
        if (file.copy(source, destination)) {
            return file.remove(source);
        } else {
            qDebug() << "copy failed! " << file.errorString();
        }
    }

    return false;
}

bool Wizard::UnshieldWorker::moveDirectory(const QString &source, const QString &destination)
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

        //if (!result)
        //    return result;
    }

    return result && removeDirectory(sourceDir.absolutePath());
}

void Wizard::UnshieldWorker::extract()
{
    emit textChanged(QLatin1String("Starting installation"));
    emit textChanged(QLatin1String("Installation target: ") + mPath);

    QStringList components;
    if (mInstallMorrowind)
        components << QLatin1String("Morrowind");

    if (mInstallTribunal)
        components << QLatin1String("Tribunal");

    if (mInstallBloodmoon)
        components << QLatin1String("Bloodmoon");

    emit textChanged(QLatin1String("Components: ") + components.join(QLatin1String(", ")));

    emit textChanged(QLatin1String("Updating Morrowind.ini: ") + mIniPath);

    //emit progressChanged(45);

    ///
//    bfs::path outputDataFilesDir = mOutputPath;
//    outputDataFilesDir /= "Data Files";
//    bfs::path extractPath = mOutputPath;
//    extractPath /= "extract-temp";

    // Create temporary extract directory
    // TODO: Use QTemporaryDir in Qt 5.0
    QString tempPath(mPath + QLatin1String("/extract-temp"));
    QDir dir;
    dir.mkpath(tempPath);

    if (mInstallMorrowind)
    {
        QString morrowindTempPath(tempPath + QLatin1String("/morrowind"));
        QString morrowindCab(QLatin1String("/mnt/cdrom/data1.hdr"));

        //extractCab(morrowindCab, morrowindTempPath);

        // TODO: Throw error;
        // Move the files from the temporary path to the destination folder

        //qDebug() << "rename: " << morrowindTempPath << " to: " << mPath;
        morrowindTempPath.append(QDir::separator() + QLatin1String("Data Files"));
//        if (!moveDirectory(morrowindTempPath, mPath))
//            qDebug() << "failed!";

        QDir sourceDir(QLatin1String("/mnt/cdrom/"));
        QStringList directories;
        directories << QLatin1String("Fonts")
                    << QLatin1String("Music")
                    << QLatin1String("Sound")
                    << QLatin1String("Splash")
                    << QLatin1String("Video");

        QFileInfoList list(sourceDir.entryInfoList(QDir::NoDotAndDotDot |
                                                   QDir::System | QDir::Hidden |
                                                   QDir::AllDirs));


        foreach(QFileInfo info, list) {
            if (info.isSymLink())
                continue;

            qDebug() << "not found " << info.fileName();

            if (directories.contains(info.fileName()))
                qDebug() << "found " << info.fileName();
//                copyDirectory(info.absoluteFilePath(), mPath);
        }

    }

//    if(!mMorrowindDone && mMorrowindPath.string().length() > 0)
//    {
//        mMorrowindDone = true;

//        bfs::path mwExtractPath = extractPath / "morrowind";
//        extract_cab(mMorrowindPath, mwExtractPath, true);

//        bfs::path dFilesDir = findFile(mwExtractPath, "morrowind.esm").parent_path();

//        installToPath(dFilesDir, outputDataFilesDir);

//        install_dfiles_outside(mwExtractPath, outputDataFilesDir);

//        // Videos are often kept uncompressed on the cd
//        bfs::path videosPath = findFile(mMorrowindPath.parent_path(), "video", false);
//        if(videosPath.string() != "")
//        {
//            emit signalGUI(QString("Installing Videos..."));
//            installToPath(videosPath, outputDataFilesDir / "Video", true);
//        }

//        bfs::path cdDFiles = findFile(mMorrowindPath.parent_path(), "data files", false);
//        if(cdDFiles.string() != "")
//        {
//            emit signalGUI(QString("Installing Uncompressed Data files from CD..."));
//            installToPath(cdDFiles, outputDataFilesDir, true);
//        }


//        bfs::rename(findFile(mwExtractPath, "morrowind.ini"), outputDataFilesDir / "Morrowind.ini");

//        mTribunalDone = contains(outputDataFilesDir, "tribunal.esm");
//        mBloodmoonDone = contains(outputDataFilesDir, "bloodmoon.esm");

//    }



    ///
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
    int progress = qCeil(((float) counter / (float) unshield_file_count(unshield)) * 100);

    qDebug() << progress << counter << unshield_file_count(unshield);

    emit textChanged(QLatin1String("Extracting: ") + QString::fromLatin1(unshield_file_name(unshield, index)));
    emit progressChanged(progress);

    success = unshield_file_save(unshield, index, fileName.toLatin1().constData());

    if (!success)
        dir.remove(fileName);

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
