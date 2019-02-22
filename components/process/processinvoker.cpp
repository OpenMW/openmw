#include "processinvoker.hpp"

#include <QMessageBox>
#include <QStringList>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

Process::ProcessInvoker::ProcessInvoker()
{
    mProcess = new QProcess(this);

    connect(mProcess, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError(QProcess::ProcessError)));

    connect(mProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processFinished(int,QProcess::ExitStatus)));


    mName = QString();
    mArguments = QStringList();
}

Process::ProcessInvoker::~ProcessInvoker()
{
}

//void Process::ProcessInvoker::setProcessName(const QString &name)
//{
//    mName = name;
//}

//void Process::ProcessInvoker::setProcessArguments(const QStringList &arguments)
//{
//    mArguments = arguments;
//}

QProcess* Process::ProcessInvoker::getProcess()
{
    return mProcess;
}

//QString Process::ProcessInvoker::getProcessName()
//{
//    return mName;
//}

//QStringList Process::ProcessInvoker::getProcessArguments()
//{
//    return mArguments;
//}

bool Process::ProcessInvoker::startProcess(const QString &name, const QStringList &arguments, bool detached)
{
//    mProcess = new QProcess(this);
    mName = name;
    mArguments = arguments;

    QString path(name);
#ifdef Q_OS_WIN
    path.append(QLatin1String(".exe"));
#elif defined(Q_OS_MAC)
    QDir dir(QCoreApplication::applicationDirPath());
    path = dir.absoluteFilePath(name);
#else
    path.prepend(QLatin1String("./"));
#endif

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
        if (!mProcess->startDetached(path, arguments)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error starting executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not start %1</b></p> \
                              <p>An error occurred while starting %1.</p> \
                              <p>Press \"Show Details...\" for more information.</p></body></html>").arg(info.fileName()));
            msgBox.setDetailedText(mProcess->errorString());
            msgBox.exec();
            return false;
        }
    } else {
        mProcess->start(path, arguments);

        /*
        if (!mProcess->waitForFinished()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error starting executable"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not start %1</b></p> \
                              <p>An error occurred while starting %1.</p> \
                              <p>Press \"Show Details...\" for more information.</p></body></html>").arg(info.fileName()));
            msgBox.setDetailedText(mProcess->errorString());
            msgBox.exec();

            return false;
        }

        if (mProcess->exitCode() != 0 || mProcess->exitStatus() == QProcess::CrashExit) {
            QString error(mProcess->readAllStandardError());
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
        */
    }

    return true;

}

void Process::ProcessInvoker::processError(QProcess::ProcessError error)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Error running executable"));
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(tr("<html><head/><body><p><b>Executable %1 returned an error</b></p> \
                      <p>An error occurred while running %1.</p> \
                      <p>Press \"Show Details...\" for more information.</p></body></html>").arg(mName));
    msgBox.setDetailedText(mProcess->errorString());
    msgBox.exec();

}

void Process::ProcessInvoker::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit) {
        QString error(mProcess->readAllStandardError());
        error.append(tr("\nArguments:\n"));
        error.append(mArguments.join(" "));

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error running executable"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>Executable %1 returned an error</b></p> \
                          <p>An error occurred while running %1.</p> \
                          <p>Press \"Show Details...\" for more information.</p></body></html>").arg(mName));
        msgBox.setDetailedText(error);
        msgBox.exec();
    }
}
