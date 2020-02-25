#include "unshieldworker.hpp"

#include <QDebug>

#include <QReadLocker>
#include <QWriteLocker>
#include <QFileDialog>
#include <QFileInfo>
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

    mStopped = false;

    qRegisterMetaType<Wizard::Component>("Wizard::Component");
}

Wizard::UnshieldWorker::~UnshieldWorker()
{
}

void Wizard::UnshieldWorker::stopWorker()
{
    mStopped = true;
    mWait.wakeOne();
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

bool Wizard::UnshieldWorker::setupSettings()
{
    // Create Morrowind.ini settings map
    if (getIniPath().isEmpty())
        return false;

    QFile file(getIniPath());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error(tr("Failed to open Morrowind configuration file!"),
                   tr("Opening %1 failed: %2.").arg(getIniPath(), file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec(mIniCodec);

    mIniSettings.readFile(stream);

    return true;
}

bool Wizard::UnshieldWorker::writeSettings()
{
    if (getIniPath().isEmpty())
        return false;

    QFile file(getIniPath());

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        emit error(tr("Failed to open Morrowind configuration file!"),
                   tr("Opening %1 failed: %2.").arg(getIniPath(), file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec(mIniCodec);

    if (!mIniSettings.writeFile(getIniPath(), stream)) {
         emit error(tr("Failed to write Morrowind configuration file!"),
                    tr("Writing to %1 failed: %2.").arg(getIniPath(), file.errorString()));
         return false;
    }

    return true;
}

bool Wizard::UnshieldWorker::removeDirectory(const QString &dirName)
{
    bool result = false;
    QDir dir(dirName);

    if (dir.exists(dirName))
    {
        QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                               QDir::System | QDir::Hidden |
                                               QDir::AllDirs | QDir::Files, QDir::DirsFirst));
        for (const QFileInfo& info : list)
        {
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

    if (info.exists()) {
        if (!dir.remove(info.absoluteFilePath()))
            return false;
    }

    if (file.copy(source, destination)) {
        if (!keepSource) {
            if (!file.remove(source))
                return false;
        } else {
            return true;
        }
    } else {
        return false;
    }

    return true;
}

bool Wizard::UnshieldWorker::copyDirectory(const QString &source, const QString &destination, bool keepSource)
{
    QDir sourceDir(source);
    QDir destDir(destination);
    bool result = true;

    if (!destDir.exists()) {
        if (!sourceDir.mkpath(destination))
            return false;
    }

    destDir.refresh();

    if (!destDir.exists())
        return false;

    QFileInfoList list(sourceDir.entryInfoList(QDir::NoDotAndDotDot |
                                                 QDir::System | QDir::Hidden |
                                                 QDir::AllDirs | QDir::Files, QDir::DirsFirst));

    for (const QFileInfo &info : list)
    {
        QString relativePath(info.absoluteFilePath());
        relativePath.remove(source);

        QString destinationPath(destDir.absolutePath() + relativePath);

        if (info.isSymLink())
            continue;

        if (info.isDir()) {
            result = copyDirectory(info.absoluteFilePath(), destinationPath);
        } else {
            result = copyFile(info.absoluteFilePath(), destinationPath);
        }
    }

    if (!keepSource)
        return result && removeDirectory(sourceDir.absolutePath());

    return result;
}

bool Wizard::UnshieldWorker::installFile(const QString &fileName, const QString &path, Qt::MatchFlags flags, bool keepSource)
{
    return installFiles(fileName, path, flags, keepSource, true);
}

bool Wizard::UnshieldWorker::installFiles(const QString &fileName, const QString &path, Qt::MatchFlags flags, bool keepSource, bool single)
{
    QDir dir(path);

    if (!dir.exists())
        return false;

    QStringList files(findFiles(fileName, path, flags));

    for (const QString &file : files)
    {
        QFileInfo info(file);
        emit textChanged(tr("Installing: %1").arg(info.fileName()));

        if (single) {
            return copyFile(info.absoluteFilePath(), getPath() + QDir::separator() + info.fileName(), keepSource);
        } else {
            if (!copyFile(info.absoluteFilePath(), getPath() + QDir::separator() + info.fileName(), keepSource))
                return false;
        }
    }

    return true;
}

bool Wizard::UnshieldWorker::installDirectories(const QString &dirName, const QString &path, bool recursive, bool keepSource)
{
    QDir dir(path);

    if (!dir.exists())
        return false;

    QStringList directories(findDirectories(dirName, path, recursive));

    for (const QString &dir : directories)
    {
        QFileInfo info(dir);
        emit textChanged(tr("Installing: %1 directory").arg(info.fileName()));
        if (!copyDirectory(info.absoluteFilePath(), getPath() + QDir::separator() + info.fileName(), keepSource))
            return false;
    }

    return true;
}

void Wizard::UnshieldWorker::extract()
{
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
        mIniSettings.setValue(QLatin1String("Archives/Archive 0"), QVariant(QString("Tribunal.bsa")));
        mIniSettings.setValue(QLatin1String("Game Files/GameFile1"), QVariant(QString("Tribunal.esm")));
    }

    if (getInstallComponent(Wizard::Component_Bloodmoon))
    {
        mIniSettings.setValue(QLatin1String("Archives/Archive 0"), QVariant(QString("Bloodmoon.bsa")));
        mIniSettings.setValue(QLatin1String("Game Files/GameFile1"), QVariant(QString("Bloodmoon.esm")));
    }

    if (getInstallComponent(Wizard::Component_Tribunal) &&
            getInstallComponent(Wizard::Component_Bloodmoon))
    {
        mIniSettings.setValue(QLatin1String("Archives/Archive 0"), QVariant(QString("Tribunal.bsa")));
        mIniSettings.setValue(QLatin1String("Archives/Archive 1"), QVariant(QString("Bloodmoon.bsa")));
        mIniSettings.setValue(QLatin1String("Game Files/GameFile1"), QVariant(QString("Tribunal.esm")));
        mIniSettings.setValue(QLatin1String("Game Files/GameFile2"), QVariant(QString("Bloodmoon.esm")));
    }

    // Write the settings to the Morrowind config file
    if (!writeSettings())
        return;

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
            if(mStopped) {
                qDebug() << "We are asked to stop !!";
                break;
            }
            disk.setPath(getDiskPath());
        } else {
            disk.setPath(getDiskPath());
        }

        QStringList list(findFiles(QLatin1String("data1.hdr"), disk.absolutePath()));

        for (const QString &file : list)
        {

            qDebug() << "current archive: " << file;

            if (component == Wizard::Component_Morrowind)
            {
                bool morrowindFound = findInCab(QLatin1String("Morrowind.bsa"), file);
                bool tribunalFound = findInCab(QLatin1String("Tribunal.bsa"), file);
                bool bloodmoonFound = findInCab(QLatin1String("Bloodmoon.bsa"), file);

                if (morrowindFound) {
                    // Check if we have correct archive, other archives have Morrowind.bsa too
                    if (tribunalFound == bloodmoonFound)
                    {
                        cabFile = file;
                        found = true; // We have a GoTY disk or a Morrowind-only disk
                    }
                }
            } else {

                if (findInCab(name + QLatin1String(".bsa"), file)) {
                    cabFile = file;
                    found = true;
                }
            }

        }

        if (!found)
        {
            emit textChanged(tr("Failed to find a valid archive containing %1.bsa! Retrying.").arg(name));
            QReadLocker readLock(&mLock);
            emit requestFileDialog(component);
            mWait.wait(&mLock);
        }
    }

    if (installComponent(component, cabFile)) {
        setComponentDone(component, true);
        return true;
    } else {
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

    for (const QString &dir : directories)
    {
        if (!installDirectories(dir, temp.absolutePath())) {
            emit error(tr("Could not install directory!"),
                       tr("Installing %1 to %2 failed.").arg(dir, temp.absolutePath()));
            return false;
        }
    }

    // Install directories from disk
    for (const QString &dir : directories)
    {
        if (!installDirectories(dir, info.absolutePath(), false, true)) {
            emit error(tr("Could not install directory!"),
                       tr("Installing %1 to %2 failed.").arg(dir, info.absolutePath()));
            return false;
        }

    }

    // Install translation files
    QStringList extensions;
    extensions << QLatin1String(".cel")
               << QLatin1String(".top")
               << QLatin1String(".mrk");

    for (const QString &extension : extensions)
    {
        if (!installFiles(extension, info.absolutePath(), Qt::MatchEndsWith)) {
            emit error(tr("Could not install translation file!"),
                       tr("Failed to install *%1 files.").arg(extension));
            return false;
        }
    }

    if (component == Wizard::Component_Morrowind)
    {
        QStringList files;
        files << QLatin1String("Morrowind.esm")
              << QLatin1String("Morrowind.bsa");

        for (const QString &file : files)
        {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not install Morrowind data file!"),
                           tr("Failed to install %1.").arg(file));
                return false;
            }
        }

        // Copy Morrowind configuration file
        if (!installFile(QLatin1String("Morrowind.ini"), temp.absolutePath())) {
            emit error(tr("Could not install Morrowind configuration file!"),
                       tr("Failed to install %1.").arg(QLatin1String("Morrowind.ini")));
            return false;
        }

        // Setup Morrowind configuration
        setIniPath(getPath() + QDir::separator() + QLatin1String("Morrowind.ini"));

        if (!setupSettings())
            return false;
    }

    if (component == Wizard::Component_Tribunal)
    {
        QFileInfo sounds(temp.absoluteFilePath(QLatin1String("Sounds")));
        QString dest(getPath() + QDir::separator() + QLatin1String("Sound"));

        if (sounds.exists()) {
            emit textChanged(tr("Installing: Sound directory"));
            if (!copyDirectory(sounds.absoluteFilePath(), dest)) {
                emit error(tr("Could not install directory!"),
                           tr("Installing %1 to %2 failed.").arg(sounds.absoluteFilePath(), dest));
                return false;
            }

        }

        QStringList files;
        files << QLatin1String("Tribunal.esm")
              << QLatin1String("Tribunal.bsa");

        for (const QString &file : files)
        {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not find Tribunal data file!"),
                           tr("Failed to find %1.").arg(file));
                return false;
            }
        }
    }

    if (component == Wizard::Component_Bloodmoon)
    {
        QFileInfo original(getPath() + QDir::separator() + QLatin1String("Tribunal.esm"));

        if (original.exists()) {
            if (!installFile(QLatin1String("Tribunal.esm"), temp.absolutePath())) {
                emit error(tr("Could not find Tribunal patch file!"),
                           tr("Failed to find %1.").arg(QLatin1String("Tribunal.esm")));
                return false;
            }
        }

        QStringList files;
        files << QLatin1String("Bloodmoon.esm")
              << QLatin1String("Bloodmoon.bsa");

        for (const QString &file : files)
        {
            if (!installFile(file, temp.absolutePath())) {
                emit error(tr("Could not find Bloodmoon data file!"),
                           tr("Failed to find %1.").arg(file));
                return false;
            }
        }

        // Load Morrowind configuration settings from the setup script
        QStringList list(findFiles(QLatin1String("setup.inx"), getDiskPath()));

        emit textChanged(tr("Updating Morrowind configuration file"));

        for (const QString &inx : list)
        {
             mIniSettings.parseInx(inx);
        }
    }

    // Finally, install Data Files directories from temp and disk
    QStringList datafiles(findDirectories(QLatin1String("Data Files"), temp.absolutePath()));
    datafiles.append(findDirectories(QLatin1String("Data Files"), info.absolutePath()));

    for (const QString &dir : datafiles)
    {
        QFileInfo info(dir);
        emit textChanged(tr("Installing: %1 directory").arg(info.fileName()));

        if (!copyDirectory(info.absoluteFilePath(), getPath())) {
            emit error(tr("Could not install directory!"),
                       tr("Installing %1 to %2 failed.").arg(info.absoluteFilePath(), getPath()));
            return false;
        }
    }

    emit textChanged(tr("%1 installation finished!").arg(name));
    return true;

}

bool Wizard::UnshieldWorker::extractFile(Unshield *unshield, const QString &destination, const QString &prefix, int index, int counter)
{
    bool success = false;
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
    if (!dir.mkpath(path))
        return false;

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
        qDebug() << "error";
        dir.remove(fileName);
    }

    return success;
}

bool Wizard::UnshieldWorker::extractCab(const QString &cabFile, const QString &destination)
{
    bool success = false;

    QByteArray array(cabFile.toUtf8());

    Unshield *unshield;
    unshield = unshield_open(array.constData());

    if (!unshield) {
        emit error(tr("Failed to open InstallShield Cabinet File."), tr("Opening %1 failed.").arg(cabFile));
        unshield_close(unshield);
        return false;
    }

    int counter = 0;

    for (int i=0; i<unshield_file_group_count(unshield); ++i)
    {
        UnshieldFileGroup *group = unshield_file_group_get(unshield, i);

        for (size_t j=group->first_file; j<=group->last_file; ++j)
        {
            if (mStopped) {
                qDebug() << "We're asked to stop!";

                unshield_close(unshield);
                return true;
            }

            if (unshield_file_is_valid(unshield, j)) {
                success = extractFile(unshield, destination, group->name, j, counter);

                if (!success) {
                    QString name(QString::fromUtf8(unshield_file_name(unshield, j)));

                    emit error(tr("Failed to extract %1.").arg(name),
                               tr("Complete path: %1").arg(destination + QDir::separator() + name));

                    unshield_close(unshield);
                    return false;
                }

                ++counter;
            }
        }
    }

    unshield_close(unshield);
    return success;
}

QString Wizard::UnshieldWorker::findFile(const QString &fileName, const QString &path)
{
    return findFiles(fileName, path).first();
}

QStringList Wizard::UnshieldWorker::findFiles(const QString &fileName, const QString &path, int depth, bool recursive,
                                              bool directories, Qt::MatchFlags flags)
{
    static const int MAXIMUM_DEPTH = 10;

    if (depth >= MAXIMUM_DEPTH) {
        qWarning("Maximum directory depth limit reached.");
        return QStringList();
    }

    QStringList result;
    QDir dir(path);

    // Prevent parsing over the complete filesystem
    if (dir == QDir::rootPath())
        return QStringList();

    if (!dir.exists())
        return QStringList();

    QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot |
                                         QDir::AllDirs | QDir::Files, QDir::DirsFirst));
    for (const QFileInfo& info : list)
    {
        if (info.isSymLink())
            continue;

        if (info.isDir()) {
            if (directories)
            {
                if (!info.fileName().compare(fileName, Qt::CaseInsensitive)) {
                    result.append(info.absoluteFilePath());
                } else {
                    if (recursive)
                        result.append(findFiles(fileName, info.absoluteFilePath(), depth + 1, recursive, true));
                }
            } else {
                if (recursive)
                    result.append(findFiles(fileName, info.absoluteFilePath(), depth + 1));
            }
        } else {
            if (directories)
                break;

            switch (flags) {
            case Qt::MatchExactly:
                if (!info.fileName().compare(fileName, Qt::CaseInsensitive))
                    result.append(info.absoluteFilePath());
                break;
            case Qt::MatchEndsWith:
                if (info.fileName().endsWith(fileName, Qt::CaseInsensitive))
                    result.append(info.absoluteFilePath());
                break;
            }
        }
    }

    return result;
}

QStringList Wizard::UnshieldWorker::findDirectories(const QString &dirName, const QString &path, bool recursive)
{
    return findFiles(dirName, path, 0, true, true);
}

bool Wizard::UnshieldWorker::findInCab(const QString &fileName, const QString &cabFile)
{
    QByteArray array(cabFile.toUtf8());

    Unshield *unshield;
    unshield = unshield_open(array.constData());

    if (!unshield) {
        emit error(tr("Failed to open InstallShield Cabinet File."), tr("Opening %1 failed.").arg(cabFile));
        unshield_close(unshield);
        return false;
    }

    for (int i=0; i<unshield_file_group_count(unshield); ++i)
    {
        UnshieldFileGroup *group = unshield_file_group_get(unshield, i);

        for (size_t j=group->first_file; j<=group->last_file; ++j)
        {

            if (unshield_file_is_valid(unshield, j)) {
                QString current(QString::fromUtf8(unshield_file_name(unshield, j)));
                if (current.toLower() == fileName.toLower()) {
                    unshield_close(unshield);
                    return true; // File is found!
                }
            }
        }
    }

    unshield_close(unshield);
    return false;
}
