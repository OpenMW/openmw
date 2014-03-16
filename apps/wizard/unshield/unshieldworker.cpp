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

    mPath = QString();
    mIniPath = QString();
    mDiskPath = QString();

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

void Wizard::UnshieldWorker::setDiskPath(const QString &path)
{
    QWriteLocker writeLock(&mLock);
    mDiskPath = path;
    mWait.wakeAll();
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

QString Wizard::UnshieldWorker::getDiskPath()
{
    QReadLocker readLock(&mLock);
    return mDiskPath;
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
    foreach(const QFileInfo &info, list) {
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
    foreach(const QFileInfo &info, list) {
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

    if (getInstallComponent(Wizard::Component_Morrowind))
    {
        if (!getComponentDone(Wizard::Component_Morrowind))
            if (!setupComponent(Wizard::Component_Morrowind))
                return;
    }

    if (getInstallComponent(Wizard::Component_Tribunal))
    {
        if (!getComponentDone(Wizard::Component_Tribunal))
            if (!setupComponent(Wizard::Component_Tribunal))
                return;
    }

    if (getInstallComponent(Wizard::Component_Bloodmoon))
    {
        if (!getComponentDone(Wizard::Component_Bloodmoon))
            if (!setupComponent(Wizard::Component_Bloodmoon))
                return;
    }

    // Update Morrowind configuration
    if (getInstallComponent(Wizard::Component_Tribunal))
    {
        mIniSettings.setValue(QLatin1String("Archives/Archive0"), QVariant(QString("Tribunal.bsa")));
        mIniSettings.setValue(QLatin1String("Game Files/Game File1"), QVariant(QString("Tribunal.esm")));
    }

    if (getInstallComponent(Wizard::Component_Bloodmoon))
    {
        mIniSettings.setValue(QLatin1String("Archives/Archive0"), QVariant(QString("Bloodmoon.bsa")));
        mIniSettings.setValue(QLatin1String("Game Files/Game File1"), QVariant(QString("Bloodmoon.esm")));
    }

    if (getInstallComponent(Wizard::Component_Tribunal) &&
            getInstallComponent(Wizard::Component_Bloodmoon))
    {
        mIniSettings.setValue(QLatin1String("Archives/Archive0"), QVariant(QString("Tribunal.bsa")));
        mIniSettings.setValue(QLatin1String("Archives/Archive1"), QVariant(QString("Bloodmoon.bsa")));
        mIniSettings.setValue(QLatin1String("Game Files/Game File1"), QVariant(QString("Tribunal.esm")));
        mIniSettings.setValue(QLatin1String("Game Files/Game File2"), QVariant(QString("Bloodmoon.esm")));
    }

    // Write the settings to the Morrowind config file
    writeSettings();

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

bool Wizard::UnshieldWorker::setupComponent(Component component)
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

    bool found = false;
    QString cabFile;
    QDir disk;

    // Keep showing the file dialog until we find the necessary install files
    while (!found) {
        if (getDiskPath().isEmpty()) {
            QReadLocker readLock(&mLock);
            emit requestFileDialog(component);
            mWait.wait(&mLock);
            disk.setPath(getDiskPath());
        } else {
            disk.setPath(getDiskPath());
        }

        QStringList list(findFiles(QLatin1String("data1.hdr"), disk.absolutePath()));

        foreach (const QString &file, list) {
            qDebug() << "current cab file is: " << file;
            if (findInCab(file, name + QLatin1String(".bsa"))) {
                cabFile = file;
                found = true;
            }
        }

        if (!found) {
            QReadLocker readLock(&mLock);
            emit requestFileDialog(component);
            mWait.wait(&mLock);
        }
    }

    if (installComponent(component, cabFile)) {
        setComponentDone(component, true);
        return true;
    } else {
        qDebug() << "Erorr installing " << name;
        return false;
    }

    return true;
}

bool Wizard::UnshieldWorker::installComponent(Component component, const QString &path)
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

    qDebug() << "Install " << name << " from " << path;


    emit textChanged(tr("Installing %1").arg(name));

    QFileInfo info(path);

    if (!info.exists()) {
        emit error(tr("Installation media path not set!"), tr("The source path for %1 was not set.").arg(name));
        return false;
    }

    // Create temporary extract directory
    // TODO: Use QTemporaryDir in Qt 5.0
    QString tempPath(getPath() + QDir::separator() + QLatin1String("extract-temp"));
    QDir temp;

    // Make sure the temporary folder is empty
    removeDirectory(tempPath);

    if (!temp.mkpath(tempPath)) {
        emit error(tr("Cannot create temporary directory!"), tr("Failed to create %1.").arg(tempPath));
        return false;
    }

    temp.setPath(tempPath);

    if (!temp.mkdir(name)) {
        emit error(tr("Cannot create temporary directory!"), tr("Failed to create %1.").arg(temp.absoluteFilePath(name)));
        return false;
    }

    if (!temp.cd(name)) {
        emit error(tr("Cannot move into temporary directory!"), tr("Failed to move into %1.").arg(temp.absoluteFilePath(name)));
        return false;
    }

    // Extract the installation files
    if (!extractCab(info.absoluteFilePath(), temp.absolutePath()))
        return false;

    // Move the files from the temporary path to the destination folder
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
        installDirectory(dir, info.absolutePath(), false);
    }

    QFileInfo datafiles(info.absolutePath() + QDir::separator() + QLatin1String("Data Files"));
    if (datafiles.exists()) {
        emit textChanged(tr("Installing: Data Files directory"));
        copyDirectory(datafiles.absoluteFilePath(), getPath());
    }

    if (component == Wizard::Component_Morrowind)
    {
        QStringList files;
        files << QLatin1String("Morrowind.esm")
              << QLatin1String("Morrowind.bsa");

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
              << QLatin1String("Tribunal.bsa")
              << QLatin1String("Morrowind.esm")
              << QLatin1String("Morrowind.bsa");

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
              << QLatin1String("Bloodmoon.bsa")
              << QLatin1String("Morrowind.esm")
              << QLatin1String("Morrowind.bsa");

        foreach (const QString &file, files) {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not find Bloodmoon data file!"), tr("Failed to find %1.").arg(file));
                return false;
            }
        }

        // Load Morrowind configuration settings from the setup script
        QStringList list(findFiles(QLatin1String("setup.inx"), getDiskPath()));

        emit textChanged(tr("Updating Morrowind configuration file"));

        foreach (const QString &inx, list) {
             mIniSettings.parseInx(inx);
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

QString Wizard::UnshieldWorker::findFile(const QString &fileName, const QString &path, int depth)
{
    return findFiles(fileName, path, depth).first();
}

QStringList Wizard::UnshieldWorker::findFiles(const QString &fileName, const QString &path, int depth)
{
    qDebug() << "Searching path: " << path << " for: " << fileName;
    static const int MAXIMUM_DEPTH = 5;

    if (depth >= MAXIMUM_DEPTH) {
        qWarning("Maximum directory depth limit reached.");
        return QStringList();
    }

    QStringList result;
    QDir dir(path);

    if (!dir.exists())
        return QStringList();

    QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                         QDir::System | QDir::Hidden |
                                         QDir::AllDirs | QDir::Files, QDir::DirsFirst));
    foreach(QFileInfo info, list) {

        if (info.isSymLink())
            continue;

        if (info.isDir()) {
            result.append(findFiles(fileName, info.absoluteFilePath(), depth + 1));
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
