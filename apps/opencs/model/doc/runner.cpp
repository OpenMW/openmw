#include "runner.hpp"

#include <QApplication>
#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>

#include "operationholder.hpp"

CSMDoc::Runner::Runner (const boost::filesystem::path& projectPath)
: mRunning (false), mStartup (nullptr), mProjectPath (projectPath)
{
    connect (&mProcess, SIGNAL (finished (int, QProcess::ExitStatus)),
        this, SLOT (finished (int, QProcess::ExitStatus)));

    connect (&mProcess, SIGNAL (readyReadStandardOutput()),
        this, SLOT (readyReadStandardOutput()));

    mProcess.setProcessChannelMode (QProcess::MergedChannels);

    mProfile.blank();
}

CSMDoc::Runner::~Runner()
{
    if (mRunning)
    {
        disconnect (&mProcess, nullptr, this, nullptr);
        mProcess.kill();
        mProcess.waitForFinished();
    }
}

void CSMDoc::Runner::start (bool delayed)
{
    if (mStartup)
    {
        delete mStartup;
        mStartup = nullptr;
    }

    if (!delayed)
    {
        mLog.clear();

        QString path = "openmw";
#ifdef Q_OS_WIN
        path.append(QString(".exe"));
#elif defined(Q_OS_MAC)
        QDir dir(QCoreApplication::applicationDirPath());
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
        path = dir.absoluteFilePath(path.prepend("OpenMW.app/Contents/MacOS/"));
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

        arguments << ("--script-run="+mStartup->fileName());;

        arguments <<
            QString::fromUtf8 (("--data=\""+mProjectPath.parent_path().string()+"\"").c_str());

        arguments << "--replace=content";

        for (std::vector<std::string>::const_iterator iter (mContentFiles.begin());
            iter!=mContentFiles.end(); ++iter)
        {
            arguments << QString::fromUtf8 (("--content="+*iter).c_str());
        }

        arguments
            << QString::fromUtf8 (("--content="+mProjectPath.filename().string()).c_str());

        mProcess.start (path, arguments);
    }

    mRunning = true;
    emit runStateChanged();
}

void CSMDoc::Runner::stop()
{
    delete mStartup;
    mStartup = nullptr;

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
    const std::vector<std::string>& contentFiles, const std::string& startupInstruction)
{
    mProfile = profile;
    mContentFiles = contentFiles;
    mStartupInstruction = startupInstruction;
}

void CSMDoc::Runner::finished (int exitCode, QProcess::ExitStatus exitStatus)
{
    mRunning = false;
    emit runStateChanged();
}

QTextDocument *CSMDoc::Runner::getLog()
{
    return &mLog;
}

void CSMDoc::Runner::readyReadStandardOutput()
{
    mLog.setPlainText (
        mLog.toPlainText() + QString::fromUtf8 (mProcess.readAllStandardOutput()));
}


CSMDoc::SaveWatcher::SaveWatcher (Runner *runner, OperationHolder *operation)
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
