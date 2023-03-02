#include "savingstate.hpp"

#include <filesystem>
#include <utility>

#include "document.hpp"
#include "operation.hpp"

CSMDoc::SavingState::SavingState(Operation& operation, std::filesystem::path projectPath, ToUTF8::FromType encoding)
    : mOperation(operation)
    , mEncoder(encoding)
    , mProjectPath(std::move(projectPath))
    , mProjectFile(false)
{
    mWriter.setEncoder(&mEncoder);
}

bool CSMDoc::SavingState::hasError() const
{
    return mOperation.hasError();
}

void CSMDoc::SavingState::start(Document& document, bool project)
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

    std::filesystem::path file(mPath.filename().u8string() + u8".tmp");

    mTmpPath = mPath.parent_path();

    mTmpPath /= file;
}

const std::filesystem::path& CSMDoc::SavingState::getPath() const
{
    return mPath;
}

const std::filesystem::path& CSMDoc::SavingState::getTmpPath() const
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

const std::deque<int>* CSMDoc::SavingState::findSubRecord(const ESM::RefId& refId) const
{
    const auto it = mSubRecords.find(refId);
    if (it == mSubRecords.end())
        return nullptr;
    return &it->second;
}

std::deque<int>& CSMDoc::SavingState::getOrInsertSubRecord(const ESM::RefId& refId)
{
    return mSubRecords[refId];
}

void CSMDoc::SavingState::clearSubRecords()
{
    mSubRecords.clear();
}
