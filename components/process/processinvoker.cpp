#include "processinvoker.hpp"

#include <QMessageBox>
#include <QProcess>
#include <QStringList>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QDir>

Process::ProcessInvoker::ProcessInvoker()
{
}

Process::ProcessInvoker::~ProcessInvoker()
{
}

bool Process::ProcessInvoker::startProcess(const QString &name, const QStringList &arguments, bool detached)
{
    QString path(name);
#ifdef Q_OS_WIN
    path.append(QLatin1String(".exe"));
#elif defined(Q_OS_MAC)
    QDir dir(QCoreApplication::applicationDirPath());
    path = dir.absoluteFilePath(name);
#else
    path.prepend(QLatin1String("./"));
#endif

    QProcess process;
    QFileInfo info(path);

    if (!info.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error starting executable"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>Could not find %1</b></p> \
                          <p>The application is not found.</p> \
                          <p>Please make sure OpenMW is installed correctly and try again.</p></body></html>").arg(info.fileName()));
        msgBox.exec();
        return false;
    }

    if (!info.isExecutable()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error starting executable"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>Could not start %1</b></p> \
                          <p>The application is not executable.</p> \
                          <p>Please make sure you have the right permissions and try again.</p></body></html>").arg(info.fileName()));
        msgBox.exec();
        return false;
    }

    // Start the executable
    if (detached) {
        if (!process.startDetached(path, arguments)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error starting executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not start %1</b></p> \
                              <p>An error occurred while starting %1.</p> \
                              <p>Press \"Show Details...\" for more information.</p></body></html>").arg(info.fileName()));
            msgBox.setDetailedText(process.errorString());
            msgBox.exec();
            return false;
        }
    } else {
        process.start(path, arguments);
        if (!process.waitForFinished()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error starting executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not start %1</b></p> \
                              <p>An error occurred while starting %1.</p> \
                              <p>Press \"Show Details...\" for more information.</p></body></html>").arg(info.fileName()));
            msgBox.setDetailedText(process.errorString());
            msgBox.exec();

            return false;
        }

        if (process.exitCode() != 0 || process.exitStatus() == QProcess::CrashExit) {
            QString error(process.readAllStandardError());
            error.append(tr("\nArguments:\n"));
            error.append(arguments.join(" "));

            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error running executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Executable %1 returned an error</b></p> \
                              <p>An error occurred while running %1.</p> \
                              <p>Press \"Show Details...\" for more information.</p></body></html>").arg(info.fileName()));
            msgBox.setDetailedText(error);
            msgBox.exec();

            return false;
        }
    }

    return true;

}
