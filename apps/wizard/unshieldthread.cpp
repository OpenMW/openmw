#include "unshieldthread.hpp"

#include <QDebug>

#include <QMessageBox>
#include <QStringList>
#include <QTextStream>
#include <QTextCodec>
#include <QFile>
#include <QDir>

#include <qmath.h>

Wizard::UnshieldThread::UnshieldThread(QObject *parent) :
    QThread(parent),
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

void Wizard::UnshieldThread::setInstallMorrowind(bool install)
{
    mInstallMorrowind = install;
}

void Wizard::UnshieldThread::setInstallTribunal(bool install)
{
    mInstallTribunal = install;
}

void Wizard::UnshieldThread::setInstallBloodmoon(bool install)
{
    mInstallBloodmoon = install;
}

void Wizard::UnshieldThread::setPath(const QString &path)
{
    mPath = path;
}

void Wizard::UnshieldThread::setIniPath(const QString &path)
{
    mIniPath = path;
}

void Wizard::UnshieldThread::setIniCodec(QTextCodec *codec)
{
    mIniCodec = codec;
}

void Wizard::UnshieldThread::setupSettings()
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

void Wizard::UnshieldThread::extract()
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

        extractCab(morrowindCab, morrowindTempPath);

        // TODO: Throw error;
        // Move the files from the temporary path to the destination folder

        qDebug() << "rename: " << morrowindTempPath << " to: " << mPath;
        if (!dir.rename(morrowindTempPath, mPath))
            qDebug() << "failed!";

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

}

bool Wizard::UnshieldThread::extractFile(Unshield *unshield, const QString &outputDir, const QString &prefix, int index, int counter)
{
    bool success;
    QString path(outputDir);
    path.append('/');

    int directory = unshield_file_directory(unshield, index);

    if (!prefix.isEmpty())
        path.append(prefix + QLatin1Char('/'));

    if (directory >= 0)
        path.append(QString::fromLatin1(unshield_directory_name(unshield, directory)) + QLatin1Char('/'));

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

void Wizard::UnshieldThread::extractCab(const QString &cabFile, const QString &outputDir)
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

void Wizard::UnshieldThread::run()
{
    qDebug() << "From worker thread: " << currentThreadId();

    setupSettings();
    extract();
}
