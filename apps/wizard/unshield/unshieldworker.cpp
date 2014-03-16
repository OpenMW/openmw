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

    qRegisterMetaType<Wizard::Component>("Wizard::Component");
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

void Wizard::UnshieldWorker::writeSettings()
{
    if (getIniPath().isEmpty())
        return;

    QFile file(getIniPath());

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qDebug() << "Error opening .ini file!";
        emit error(tr("Failed to open Morrowind configuration file!"),
                   tr("Opening %1 failed: %2.").arg(getIniPath(), file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec(mIniCodec);

    if (!mIniSettings.writeFile(getIniPath(), stream)) {
         emit error(tr("Failed to write Morrowind configuration file!"),
                    tr("Writing to %1 failed: %2.").arg(getIniPath(), file.errorString()));
    }
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

bool Wizard::UnshieldWorker::installFile(const QString &fileName, const QString &path)
{
    qDebug() << "Attempting to find file: " << fileName << " in: " << path;

    bool result = true;
    QDir dir(path);

    if (!dir.exists())
        return false;

    QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                         QDir::System | QDir::Hidden |
                                         QDir::AllDirs | QDir::Files, QDir::DirsFirst));
    foreach(QFileInfo info, list) {
        if (info.isDir()) {
            result = installFile(fileName, info.absoluteFilePath());
        } else {
            if (info.fileName() == fileName) {
                qDebug() << "File found at: " << info.absoluteFilePath();

                emit textChanged(tr("Installing: %1").arg(info.fileName()));
                return moveFile(info.absoluteFilePath(), getPath() + QDir::separator() + info.fileName());
            }
        }
    }

    return result;
}

bool Wizard::UnshieldWorker::installDirectory(const QString &dirName, const QString &path, bool recursive)
{
    qDebug() << "Attempting to find: " << dirName << " in: " << path;
    bool result = true;
    QDir dir(path);

    if (!dir.exists())
        return false;

    QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                         QDir::System | QDir::Hidden |
                                         QDir::AllDirs));
    foreach(QFileInfo info, list) {
        if (info.isSymLink())
            continue;

        if (info.isDir()) {
            if (info.fileName() == dirName) {
                qDebug() << "Directory found at: " << info.absoluteFilePath();
                emit textChanged(tr("Installing: %1 directory").arg(info.fileName()));
                return copyDirectory(info.absoluteFilePath(), getPath() + QDir::separator() + info.fileName());
            } else {
                if (recursive)
                    result = installDirectory(dirName, info.absoluteFilePath());
            }
        }
    }

    return result;
}

void Wizard::UnshieldWorker::extract()
{
    qDebug() << "extract!";

    qDebug() << findFiles(QLatin1String("data1.hdr"), QLatin1String("/mnt/cdrom"));
//    QDir disk;

//    if (getInstallComponent(Wizard::Component_Morrowind))
//    {
//        if (!getComponentDone(Wizard::Component_Morrowind))
//        {
//            if (getComponentPath(Wizard::Component_Morrowind).isEmpty()) {
//                qDebug() << "request file dialog";
//                QReadLocker readLock(&mLock);
//                emit requestFileDialog(Wizard::Component_Morrowind);
//                mWait.wait(&mLock);
//            }

//            if (!getComponentPath(Wizard::Component_Morrowind).isEmpty()) {
//                disk.setPath(getComponentPath(Wizard::Component_Morrowind));

//                if (!findInCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), QLatin1String("Morrowind.bsa")))
//                {
//                    QReadLocker readLock(&mLock);
//                    emit requestFileDialog(Wizard::Component_Morrowind);
//                    mWait.wait(&mLock);
//                } else {
//                    if (installComponent(Wizard::Component_Morrowind)) {
//                        setComponentDone(Wizard::Component_Morrowind, true);
//                    } else {
//                        qDebug() << "Erorr installing Morrowind";

//                        return;
//                    }
//                }
//            }
//        }
//    }

//    if (getInstallComponent(Wizard::Component_Tribunal))
//    {
//        setupAddon(Wizard::Component_Tribunal);
//    }

//    if (getInstallComponent(Wizard::Component_Bloodmoon))
//    {
//        setupAddon(Wizard::Component_Bloodmoon);
//    }

//    // Update Morrowind configuration
//    if (getInstallComponent(Wizard::Component_Tribunal))
//    {
//        mIniSettings.setValue(QLatin1String("Archives/Archive0"), QVariant(QString("Tribunal.bsa")));
//        mIniSettings.setValue(QLatin1String("Game Files/Game File1"), QVariant(QString("Tribunal.esm")));
//    }

//    if (getInstallComponent(Wizard::Component_Bloodmoon))
//    {
//        mIniSettings.setValue(QLatin1String("Archives/Archive0"), QVariant(QString("Bloodmoon.bsa")));
//        mIniSettings.setValue(QLatin1String("Game Files/Game File1"), QVariant(QString("Bloodmoon.esm")));
//    }

//    if (getInstallComponent(Wizard::Component_Tribunal) &&
//            getInstallComponent(Wizard::Component_Bloodmoon))
//    {
//        mIniSettings.setValue(QLatin1String("Archives/Archive0"), QVariant(QString("Tribunal.bsa")));
//        mIniSettings.setValue(QLatin1String("Archives/Archive1"), QVariant(QString("Bloodmoon.bsa")));
//        mIniSettings.setValue(QLatin1String("Game Files/Game File1"), QVariant(QString("Tribunal.esm")));
//        mIniSettings.setValue(QLatin1String("Game Files/Game File2"), QVariant(QString("Bloodmoon.esm")));
//    }


//    // Write the settings to the Morrowind config file
//    writeSettings();

//    // Remove the temporary directory
//    //removeDirectory(getPath() + QDir::separator() + QLatin1String("extract-temp"));

//    // Fill the progress bar
//    int total = 0;

//    if (getInstallComponent(Wizard::Component_Morrowind))
//        total = 100;

//    if (getInstallComponent(Wizard::Component_Tribunal))
//        total = total + 100;

//    if (getInstallComponent(Wizard::Component_Bloodmoon))
//        total = total + 100;

//    emit textChanged(tr("Installation finished!"));
//    emit progressChanged(total);
//    emit finished();

//    qDebug() << "installation finished!";
}

void Wizard::UnshieldWorker::setupAddon(Component component)
{
    qDebug() << "SetupAddon!" << getComponentPath(component) << getComponentPath(Wizard::Component_Morrowind);

    if (!getComponentDone(component))
    {
        qDebug() << "Component not done!";

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

        qDebug() << "Determine if file is in current data1.hdr: " << name;

        if (!disk.isEmpty()) {
            if (!findInCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), name + QLatin1String(".bsa")))
            {
                if (!disk.cd(name)) {
                    qDebug() << "not found on cd!";
                    QReadLocker locker(&mLock);
                    emit requestFileDialog(component);
                    mWait.wait(&mLock);

                } else if (disk.exists(QLatin1String("data1.hdr"))) {
                    if (!findInCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), name + QLatin1String(".bsa")))
                    {
                        QReadLocker locker(&mLock);
                        emit requestFileDialog(component);
                        mWait.wait(&mLock);
                    } else {
                        setComponentPath(component, disk.absolutePath());
                        disk.setPath(getComponentPath(component));
                    }
                }
            }

        } else {
            QReadLocker locker(&mLock);
            emit requestFileDialog(component);
            mWait.wait(&mLock);
        }

        disk.setPath(getComponentPath(component));

        if (!findInCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), name + QLatin1String(".bsa")))
        {
            if (!disk.cd(name)) {
                qDebug() << "not found on cd!";
                QReadLocker locker(&mLock);
                emit requestFileDialog(component);
                mWait.wait(&mLock);

            } else if (disk.exists(QLatin1String("data1.hdr"))) {
                if (!findInCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), name + QLatin1String(".bsa")))
                {
                    QReadLocker locker(&mLock);
                    emit requestFileDialog(component);
                    mWait.wait(&mLock);
                } else {
                    setComponentPath(component, disk.absolutePath());
                    disk.setPath(getComponentPath(component));
                }
            }

            // Make sure the dir is up-to-date
            //disk.setPath(getComponentPath(component));
        }

        // Now do the actual installing

        if (installComponent(component)) {
            setComponentDone(component, true);
        } else {
            qDebug() << "Error installing " << name;
            return;
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

    emit textChanged(tr("Installing %1").arg(name));

    QDir disk(getComponentPath(component));

    if (!disk.exists()) {
        qDebug() << "Component path not set: " << getComponentPath(Wizard::Component_Morrowind);
        emit error(tr("Component path not set!"), tr("The source path for %1 was not set.").arg(name));
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
        emit error(tr("Cannot create temporary directory!"), tr("Failed to create %1.").arg(tempPath));
        return false;
    }

    temp.setPath(tempPath);

    if (!temp.mkdir(name)) {
        emit error(tr("Cannot create temporary directory!"), tr("Failed to create %1.").arg(temp.absoluteFilePath(name)));
        return false;
    }

    if (!temp.cd(name)) {
        qDebug() << "Can't cd to dir";
        emit error(tr("Cannot move into temporary directory!"), tr("Failed to move into %1.").arg(temp.absoluteFilePath(name)));
        return false;
    }

    // Extract the installation files
    if (!extractCab(disk.absoluteFilePath(QLatin1String("data1.hdr")), temp.absolutePath()))
        return false;

    // Move the files from the temporary path to the destination folder
//    emit textChanged(tr("Moving installation files"));
//    if (!moveDirectory(temp.absoluteFilePath(QLatin1String("Data Files")), getPath())) {
//        qDebug() << "failed to move files!";
//        emit error(tr("Moving extracted files failed!"),
//                   tr("Failed to move files from %1 to %2.").arg(temp.absoluteFilePath(QLatin1String("Data Files")),
//                                                                getPath()));
//        return false;
//    }
    emit textChanged(tr("Moving installation files"));

    // Install extracted directories
    QStringList directories;
    directories << QLatin1String("BookArt")
                << QLatin1String("Fonts")
                << QLatin1String("Icons")
                << QLatin1String("Meshes")
                << QLatin1String("Music")
                << QLatin1String("Sound")
                << QLatin1String("Splash")
                << QLatin1String("Textures")
                << QLatin1String("Video");

    foreach (const QString &dir, directories) {
        installDirectory(dir, temp.absolutePath());
    }

    // Install directories from disk
    foreach (const QString &dir, directories) {
        qDebug() << "\n\nDISK DIRS!";
        installDirectory(dir, disk.absolutePath(), false);
    }

    QFileInfo info(disk.absoluteFilePath("Data Files"));
    if (info.exists()) {
        emit textChanged(tr("Installing: Data Files directory"));
        copyDirectory(info.absoluteFilePath(), getPath());
    }

    if (component == Wizard::Component_Morrowind)
    {
        QStringList files;
        files << QLatin1String("Morrowind.esm")
              << QLatin1String("Morrowin.bsa");

        foreach (const QString &file, files) {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not find Morrowind data file!"), tr("Failed to find %1.").arg(file));
                return false;
            }
        }

        // Copy Morrowind configuration file
        if (!installFile(QLatin1String("Morrowind.ini"), temp.absolutePath())) {
            emit error(tr("Could not find Morrowind configuration file!"), tr("Failed to find %1.").arg(QLatin1String("Morrowind.ini")));
            return false;
        }

        // Setup Morrowind configuration
        setIniPath(getPath() + QDir::separator() + QLatin1String("Morrowind.ini"));
        setupSettings();
    }

    if (component == Wizard::Component_Tribunal)
    {
        QFileInfo sounds(temp.absoluteFilePath(QLatin1String("Sounds")));

        if (sounds.exists()) {
            emit textChanged(tr("Installing: Sound directory"));
            copyDirectory(sounds.absoluteFilePath(), getPath() + QDir::separator() + QLatin1String("Sound"));
        }

        QStringList files;
        files << QLatin1String("Tribunal.esm")
              << QLatin1String("Tribunal.bsa");

        foreach (const QString &file, files) {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not find Tribunal data file!"), tr("Failed to find %1.").arg(file));
                return false;
            }
        }

    }

    if (component == Wizard::Component_Bloodmoon)
    {
        QFileInfo original(getPath() + QDir::separator() + QLatin1String("Tribunal.esm"));

        if (original.exists()) {
            if (!installFile(QLatin1String("Tribunal.esm"), temp.absolutePath())) {
                emit error(tr("Could not find Tribunal patch file!"), tr("Failed to find %1.").arg(QLatin1String("Tribunal.esm")));
                return false;
            }
        }

        QStringList files;
        files << QLatin1String("Bloodmoon.esm")
              << QLatin1String("Bloodmoon.bsa");

        foreach (const QString &file, files) {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not find Bloodmoon data file!"), tr("Failed to find %1.").arg(file));
                return false;
            }
        }

        // Load Morrowind configuration settings from the setup script
        QFileInfo inx(disk.absoluteFilePath(QLatin1String("setup.inx")));

        if (inx.exists()) {
            emit textChanged(tr("Updating Morrowind configuration file"));
            mIniSettings.parseInx(inx.absoluteFilePath());
        } else {
            qDebug() << "setup.inx not found!";
        }
    }

    emit textChanged(tr("%1 installation finished!").arg(name));
    return true;

}

bool Wizard::UnshieldWorker::extractFile(Unshield *unshield, const QString &destination, const QString &prefix, int index, int counter)
{
    bool success;
    QString path(destination);
    path.append(QDir::separator());

    int directory = unshield_file_directory(unshield, index);

    if (!prefix.isEmpty())
        path.append(prefix + QDir::separator());

    if (directory >= 0)
        path.append(QString::fromUtf8(unshield_directory_name(unshield, directory)) + QDir::separator());

    // Ensure the path has the right separators
    path.replace(QLatin1Char('\\'), QDir::separator());
    path = QDir::toNativeSeparators(path);

    // Ensure the target path exists
    QDir dir;
    dir.mkpath(path);

    QString fileName(path);
    fileName.append(QString::fromUtf8(unshield_file_name(unshield, index)));

    // Calculate the percentage done
    int progress = (((float) counter / (float) unshield_file_count(unshield)) * 100);

    if (getComponentDone(Wizard::Component_Morrowind))
        progress = progress + 100;

    if (getComponentDone(Wizard::Component_Tribunal))
        progress = progress + 100;

    emit textChanged(tr("Extracting: %1").arg(QString::fromUtf8(unshield_file_name(unshield, index))));
    emit progressChanged(progress);

    QByteArray array(fileName.toUtf8());
    success = unshield_file_save(unshield, index, array.constData());

    if (!success) {
        emit error(tr("Failed to extract %1.").arg(QString::fromUtf8(unshield_file_name(unshield, index))), tr("Complete path: %1.").arg(fileName));
        dir.remove(fileName);
    }

    return success;
}

QString Wizard::UnshieldWorker::findFile(const QString &fileName, const QString &path)
{
    return findFiles(fileName, path).first();
}

QStringList Wizard::UnshieldWorker::findFiles(const QString &fileName, const QString &path)
{
    QStringList result;
    QDir dir(source);

    if (!dir.exists())
        return QStringList();

    QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                         QDir::System | QDir::Hidden |
                                         QDir::AllDirs | QDir::Files, QDir::DirsFirst));
    foreach(QFileInfo info, list) {

        if (info.isSymLink())
            continue;

        if (info.isDir()) {
            result = findFiles(file, info.absoluteFilePath());
        } else {
            if (info.fileName() == fileName) {
                qDebug() << "File found at: " << info.absoluteFilePath();
                result.append(info.absoluteFilePath());
            }
        }
    }

    return result;

}

bool Wizard::UnshieldWorker::findInCab(const QString &cabFile, const QString &fileName)
{
    QByteArray array(cabFile.toUtf8());

    Unshield *unshield;
    unshield = unshield_open(array.constData());

    if (!unshield) {
        emit error(tr("Failed to open InstallShield Cabinet File."), tr("Opening %1 failed.").arg(cabFile));
        return false;
    }

    for (int i=0; i<unshield_file_group_count(unshield); ++i)
    {
        UnshieldFileGroup *group = unshield_file_group_get(unshield, i);

        for (size_t j=group->first_file; j<=group->last_file; ++j)
        {

            if (unshield_file_is_valid(unshield, j)) {
                QString current(QString::fromUtf8(unshield_file_name(unshield, j)));
                qDebug() << "Current is: " << current;
                if (current.toLower() == fileName.toLower())
                    return true; // File is found!
            }
        }
    }

    unshield_close(unshield);
    return false;
}

bool Wizard::UnshieldWorker::extractCab(const QString &cabFile, const QString &destination)
{
    bool success;

    QByteArray array(cabFile.toUtf8());

    Unshield *unshield;
    unshield = unshield_open(array.constData());

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
                success = extractFile(unshield, destination, group->name, j, counter);
                ++counter;
            }
        }
    }

    unshield_close(unshield);
    return success;
}
