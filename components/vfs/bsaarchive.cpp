#include "bsaarchive.hpp"
#include <components/bsa/tes4bsa_file.hpp>
#include <memory>

namespace VFS
{

BsaArchive::BsaArchive(const std::string &filename)
{
    Bsa::BsaVersion bsaVersion = Bsa::TES4BSAFile::detectVersion(filename);

    if (bsaVersion == Bsa::BSAVER_TES4PLUS) {
        mFile = std::unique_ptr<Bsa::TES4BSAFile>(new Bsa::TES4BSAFile());
    }
    else {
        mFile = std::unique_ptr<Bsa::BSAFile>(new Bsa::BSAFile());
    }

    mFile->open(filename);

    const Bsa::BSAFile::FileList &filelist = mFile->getList();
    for(Bsa::BSAFile::FileList::const_iterator it = filelist.begin();it != filelist.end();++it)
    {
        mResources.push_back(BsaArchiveFile(&*it, mFile.get()));
    }
}

BsaArchive::~BsaArchive() {
}

void BsaArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
{
    for (std::vector<BsaArchiveFile>::iterator it = mResources.begin(); it != mResources.end(); ++it)
    {
        std::string ent = it->mInfo->name;
        std::transform(ent.begin(), ent.end(), ent.begin(), normalize_function);

        out[ent] = &*it;
    }
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

}
