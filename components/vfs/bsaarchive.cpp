#include "bsaarchive.hpp"

namespace VFS
{


BsaArchive::BsaArchive(const std::string &filename)
    : mFileName(filename)
{
    load();
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

void BsaArchive::load()
{
    mFile.open(mFileName);

    const Bsa::BSAFile::FileList &filelist = mFile.getList();
    for(Bsa::BSAFile::FileList::const_iterator it = filelist.begin();it != filelist.end();++it)
    {
        mResources.push_back(BsaArchiveFile(&*it, &mFile));
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
