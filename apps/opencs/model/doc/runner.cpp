#include "runner.hpp"

#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QTextStream>

#include <components/files/qtconversion.hpp>

#include "operationholder.hpp"

CSMDoc::Runner::Runner(std::filesystem::path projectPath)
    : mRunning(false)
    , mStartup(nullptr)
    , mProjectPath(std::move(projectPath))
{
    connect(&mProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &Runner::finished);

    connect(&mProcess, &QProcess::readyReadStandardOutput, this, &Runner::readyReadStandardOutput);

    mProcess.setProcessChannelMode(QProcess::MergedChannels);

    mProfile.blank();
}

CSMDoc::Runner::~Runner()
{
    if (mRunning)
    {
        disconnect(&mProcess, nullptr, this, nullptr);
        mProcess.kill();
        mProcess.waitForFinished();
    }
}

void CSMDoc::Runner::start(bool delayed)
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
        path.append(QLatin1String(".exe"));
#endif
        QDir dir(QCoreApplication::applicationDirPath());
#ifdef Q_OS_MAC
        // the CS and engine are in separate .app directories
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
        path.prepend("OpenMW.app/Contents/MacOS/");
#endif
        path = dir.absoluteFilePath(path);

        mStartup = new QTemporaryFile(this);
        mStartup->open();

        {
            QTextStream stream(mStartup);

            if (!mStartupInstruction.empty())
                stream << QString::fromUtf8(mStartupInstruction.c_str()) << '\n';

            stream << QString::fromUtf8(mProfile.mScriptText.c_str());
        }

        mStartup->close();

        QStringList arguments;
        arguments << "--skip-menu";

        if (mProfile.mFlags & ESM::DebugProfile::Flag_BypassNewGame)
            arguments << "--new-game=0";
        else
            arguments << "--new-game=1";

        arguments << ("--script-run=" + mStartup->fileName());

        arguments << "--data=\"" + Files::pathToQString(mProjectPath.parent_path()) + "\"";

        arguments << "--replace=content";

        for (const auto& mContentFile : mContentFiles)
        {
            arguments << "--content=" + Files::pathToQString(mContentFile);
        }

        arguments << "--content=" + Files::pathToQString(mProjectPath.filename());

        mProcess.start(path, arguments);
    }

    mRunning = true;
    emit runStateChanged();
}

void CSMDoc::Runner::stop()
{
    delete mStartup;
    mStartup = nullptr;

    if (mProcess.state() == QProcess::NotRunning)
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

void CSMDoc::Runner::configure(const ESM::DebugProfile& profile, const std::vector<std::filesystem::path>& contentFiles,
    const std::string& startupInstruction)
{
    mProfile = profile;
    mContentFiles = contentFiles;
    mStartupInstruction = startupInstruction;
}

void CSMDoc::Runner::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    mRunning = false;
    emit runStateChanged();
}

QTextDocument* CSMDoc::Runner::getLog()
{
    return &mLog;
}

void CSMDoc::Runner::readyReadStandardOutput()
{
    mLog.setPlainText(mLog.toPlainText() + QString::fromUtf8(mProcess.readAllStandardOutput()));
}

CSMDoc::SaveWatcher::SaveWatcher(Runner* runner, OperationHolder* operation)
    : QObject(runner)
    , mRunner(runner)
{
    connect(operation, &OperationHolder::done, this, &SaveWatcher::saveDone);
}

void CSMDoc::SaveWatcher::saveDone(int type, bool failed)
{
    if (failed)
        mRunner->stop();
    else
        mRunner->start();

    deleteLater();
}
