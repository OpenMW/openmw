
#include "runner.hpp"

#include "operation.hpp"

CSMDoc::Runner::Runner() : mRunning (false)
{
    connect (&mProcess, SIGNAL (finished (int, QProcess::ExitStatus)),
        this, SLOT (finished (int, QProcess::ExitStatus)));
}

CSMDoc::Runner::~Runner()
{
    if (mRunning)
    {
        disconnect (&mProcess, 0, this, 0);
        mProcess.kill();
        mProcess.waitForFinished();
    }
}

void CSMDoc::Runner::start (bool delayed)
{
    if (!delayed)
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
    }

    mRunning = true;
    emit runStateChanged();
}

void CSMDoc::Runner::stop()
{
    if (mProcess.state()==QProcess::NotRunning)
    {
        mRunning = false;
        emit runStateChanged();
    }
    else
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


CSMDoc::SaveWatcher::SaveWatcher (Runner *runner, Operation *operation)
: QObject (runner), mRunner (runner)
{
    connect (operation, SIGNAL (done (int, bool)), this, SLOT (saveDone (int, bool)));
}

void CSMDoc::SaveWatcher::saveDone (int type, bool failed)
{
    if (failed)
        mRunner->stop();
    else
        mRunner->start();

    deleteLater();
}
