#include "savingstate.hpp"

#include "operation.hpp"
#include "document.hpp"

CSMDoc::SavingState::SavingState (Operation& operation, const std::experimental::filesystem::path& projectPath,
    ToUTF8::FromType encoding)
: mOperation (operation), mEncoder (encoding),  mProjectPath (projectPath), mProjectFile (false)
{
    mWriter.setEncoder (&mEncoder);
}

bool CSMDoc::SavingState::hasError() const
{
    return mOperation.hasError();
}

void CSMDoc::SavingState::start (Document& document, bool project)
{
    mProjectFile = project;

    if (mStream.is_open())
        mStream.close();

    mStream.clear();

    mSubRecords.clear();

    if (project)
        mPath = mProjectPath;
    else
        mPath = document.getSavePath();

    std::experimental::filesystem::path file (mPath.filename().string() + ".tmp");

    mTmpPath = mPath.parent_path();

    mTmpPath /= file;
}

const std::experimental::filesystem::path& CSMDoc::SavingState::getPath() const
{
    return mPath;
}

const std::experimental::filesystem::path& CSMDoc::SavingState::getTmpPath() const
{
    return mTmpPath;
}

std::ofstream& CSMDoc::SavingState::getStream()
{
    return mStream;
}

ESM::ESMWriter& CSMDoc::SavingState::getWriter()
{
    return mWriter;
}

bool CSMDoc::SavingState::isProjectFile() const
{
    return mProjectFile;
}

std::map<std::string, std::deque<int> >& CSMDoc::SavingState::getSubRecords()
{
    return mSubRecords;
}
