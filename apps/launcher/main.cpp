#include <QApplication>
#include <QMessageBox>
#include <QTextStream>
#include <QTextCodec>
#include <QDir>
#include <QAbstractButton>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>

#include <QDebug>

#include <components/files/configurationmanager.hpp>

#include "maindialog.hpp"
#include "settings/gamesettings.hpp"
#include "settings/graphicssettings.hpp"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Now we make sure the current dir is set to application path
    QDir dir(QCoreApplication::applicationDirPath());

    #ifdef Q_OS_MAC
    if (dir.dirName() == "MacOS") {
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
    }

    // force Qt to load only LOCAL plugins, don't touch system Qt installation
    QDir pluginsPath(QCoreApplication::applicationDirPath());
    pluginsPath.cdUp();
    pluginsPath.cd("Plugins");

    QStringList libraryPaths;
    libraryPaths << pluginsPath.path() << QCoreApplication::applicationDirPath();
    app.setLibraryPaths(libraryPaths);
    #endif

    QDir::setCurrent(dir.absolutePath());

    // Create setting file handlers

    Files::ConfigurationManager cfgMgr;
    QString userPath = QString::fromStdString(cfgMgr.getUserPath().string());
    QString globalPath = QString::fromStdString(cfgMgr.getGlobalPath().string());

    GameSettings gameSettings(cfgMgr);
    GraphicsSettings graphicsSettings;

    QStringList paths;
    paths.append(userPath + QString("openmw.cfg"));
    paths.append(QString("openmw.cfg"));
    paths.append(globalPath + QString("openmw.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading: " << path;
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Error opening OpenMW configuration file");
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return 0;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            gameSettings.readFile(stream);
        }
        file.close();
    }

    if (gameSettings.getDataDirs().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error detecting Morrowind installation");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(QObject::tr("<br><b>Could not find the Data Files location</b><br><br> \
                          The directory containing the data files was not found.<br><br> \
                          Press \"Browse...\" to specify the location manually.<br>"));

        QAbstractButton *dirSelectButton =
                msgBox.addButton(QObject::tr("B&rowse..."), QMessageBox::ActionRole);

        msgBox.exec();

        QString selectedFile;
        if (msgBox.clickedButton() == dirSelectButton) {
            selectedFile = QFileDialog::getOpenFileName(
                        NULL,
                        QObject::tr("Select master file"),
                        QDir::currentPath(),
                        QString("Morrowind master file (*.esm)"));
        }

        if (selectedFile.isEmpty())
            return 0; // Cancel was clicked;

        qDebug() << selectedFile;
        QFileInfo info(selectedFile);

        // Add the new dir to the settings file and to the data dir container
        gameSettings.setValue(QString("data"), info.absolutePath());
        gameSettings.addDataDir(info.absolutePath());

    }

    // On to the graphics settings
    qDebug() << userPath;

    QFile localDefault(QString("settings-default.cfg"));
    QFile globalDefault(globalPath + QString("settings-default.cfg"));

    if (!localDefault.exists() && !globalDefault.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error reading OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(QObject::tr("<br><b>Could not find settings-default.cfg</b><br><br> \
                          The problem may be due to an incomplete installation of OpenMW.<br> \
                          Reinstalling OpenMW may resolve the problem."));
        msgBox.exec();
        return 0;
    }

    paths.clear();
    paths.append(globalPath + QString("settings-default.cfg"));
    paths.append(QString("settings-default.cfg"));
    paths.append(userPath + QString("settings.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading: " << path;
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Error opening OpenMW configuration file");
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(QObject::tr("<br><b>Could not open %0 for reading</b><br><br> \
                                  Please make sure you have the right permissions \
                                  and try again.<br>").arg(file.fileName()));
                msgBox.exec();
                return 0;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            graphicsSettings.readFile(stream);
        }
        file.close();
    }


    MainDialog mainWin(gameSettings, graphicsSettings);

    if (mainWin.setup()) {
        mainWin.show();
    } else {
        return 0;
    }

    return app.exec();
}

