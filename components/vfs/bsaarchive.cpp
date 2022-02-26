#include "bsaarchive.hpp"

#include <memory>
#include <algorithm>

namespace VFS
{

BsaArchive::BsaArchive(const std::string &filename)
{
    mFile = std::make_unique<Bsa::BSAFile>();
    mFile->open(filename);

    const Bsa::BSAFile::FileList &filelist = mFile->getList();
    for(Bsa::BSAFile::FileList::const_iterator it = filelist.begin();it != filelist.end();++it)
    {
        mResources.emplace_back(&*it, mFile.get());
    }
}

BsaArchive::BsaArchive()
{
}

BsaArchive::~BsaArchive() {
}

void BsaArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
{
    for (std::vector<BsaArchiveFile>::iterator it = mResources.begin(); it != mResources.end(); ++it)
    {
        std::string ent = it->mInfo->name();
        std::transform(ent.begin(), ent.end(), ent.begin(), normalize_function);

        out[ent] = &*it;
    }
}

bool BsaArchive::contains(const std::string& file, char (*normalize_function)(char)) const
{
    for (const auto& it : mResources)
    {
        std::string ent = it.mInfo->name();
        std::transform(ent.begin(), ent.end(), ent.begin(), normalize_function);
        if(file == ent)
            return true;
    }
    return false;
}

std::string BsaArchive::getDescription() const
{
    return std::string{"BSA: "} + mFile->getFilename();
}

// ------------------------------------------------------------------------------

BsaArchiveFile::BsaArchiveFile(const Bsa::BSAFile::FileStruct *info, Bsa::BSAFile* bsa)
    : mInfo(info)
    , mFile(bsa)
{

}

Files::IStreamPtr BsaArchiveFile::open()
{
    return mFile->getFile(mInfo);
}

CompressedBsaArchive::CompressedBsaArchive(const std::string &filename)
    : Archive()
{
    mCompressedFile = std::make_unique<Bsa::CompressedBSAFile>();
    mCompressedFile->open(filename);

    const Bsa::BSAFile::FileList &filelist = mCompressedFile->getList();
    for(Bsa::BSAFile::FileList::const_iterator it = filelist.begin();it != filelist.end();++it)
    {
        mCompressedResources.emplace_back(&*it, mCompressedFile.get());
    }
}

void CompressedBsaArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
{
    for (std::vector<CompressedBsaArchiveFile>::iterator it = mCompressedResources.begin(); it != mCompressedResources.end(); ++it)
    {
        std::string ent = it->mInfo->name();
        std::transform(ent.begin(), ent.end(), ent.begin(), normalize_function);

        out[ent] = &*it;
    }
}

bool CompressedBsaArchive::contains(const std::string& file, char (*normalize_function)(char)) const
{
    for (const auto& it : mCompressedResources)
    {
        std::string ent = it.mInfo->name();
        std::transform(ent.begin(), ent.end(), ent.begin(), normalize_function);
        if(file == ent)
            return true;
    }
    return false;
}

std::string CompressedBsaArchive::getDescription() const
{
    return std::string{"BSA: "} + mCompressedFile->getFilename();
}


CompressedBsaArchiveFile::CompressedBsaArchiveFile(const Bsa::BSAFile::FileStruct *info, Bsa::CompressedBSAFile* bsa)
    : mInfo(info)
    , mCompressedFile(bsa)
{

}

Files::IStreamPtr CompressedBsaArchiveFile::open()
{
    return mCompressedFile->getFile(mInfo);
}

}
