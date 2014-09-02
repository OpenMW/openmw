
#include "runner.hpp"


CSMDoc::Runner::Runner()
{
    connect (&mProcess, SIGNAL (finished (int, QProcess::ExitStatus)),
        this, SLOT (finished (int, QProcess::ExitStatus)));
}

void CSMDoc::Runner::start()
{
    QString path = "openmw";
#ifdef Q_OS_WIN
    path.append(QString(".exe"));
#elif defined(Q_OS_MAC)
    QDir dir(QCoreApplication::applicationDirPath());
    path = dir.absoluteFilePath(name);
#else
    path.prepend(QString("./"));
#endif

    mProcess.start (path);
    emit runStateChanged (true);
}

void CSMDoc::Runner::stop()
{
    mProcess.kill();
}

void CSMDoc::Runner::finished (int exitCode, QProcess::ExitStatus exitStatus)
{
    emit runStateChanged (false);
}
