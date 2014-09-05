
#include "runner.hpp"

#include <QTemporaryFile>
#include <QTextStream>

#include "operation.hpp"

CSMDoc::Runner::Runner() : mRunning (false), mStartup (0)
{
    connect (&mProcess, SIGNAL (finished (int, QProcess::ExitStatus)),
        this, SLOT (finished (int, QProcess::ExitStatus)));

    mProfile.blank();
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
    if (mStartup)
    {
        delete mStartup;
        mStartup = 0;
    }

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

        mStartup = new QTemporaryFile (this);
        mStartup->open();

        {
            QTextStream stream (mStartup);

            if (!mStartupInstruction.empty())
                stream << QString::fromUtf8 (mStartupInstruction.c_str()) << '\n';

            stream << QString::fromUtf8 (mProfile.mScriptText.c_str());
        }

        mStartup->close();

        QStringList arguments;
        arguments << "--skip-menu";

        if (mProfile.mFlags & ESM::DebugProfile::Flag_BypassNewGame)
            arguments << "--new-game=0";
        else
            arguments << "--new-game=1";

        arguments << ("--script-run="+mStartup->fileName());

        mProcess.start (path, arguments);
    }

    mRunning = true;
    emit runStateChanged();
}

void CSMDoc::Runner::stop()
{
    delete mStartup;
    mStartup = 0;

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

void CSMDoc::Runner::configure (const ESM::DebugProfile& profile,
    const std::string& startupInstruction)
{
    mProfile = profile;
    mStartupInstruction = startupInstruction;
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
