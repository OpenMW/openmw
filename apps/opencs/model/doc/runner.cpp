
#include "runner.hpp"


CSMDoc::Runner::Runner() : mRunning (false)
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
    mRunning = true;
    emit runStateChanged();
}

void CSMDoc::Runner::stop()
{
    mProcess.kill();
}

bool CSMDoc::Runner::isRunning() const
{
    return mRunning;
}

void CSMDoc::Runner::finished (int exitCode, QProcess::ExitStatus exitStatus)
{
    mRunning = false;
    emit runStateChanged();
}
