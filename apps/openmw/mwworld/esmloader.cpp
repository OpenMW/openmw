#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mEsm(readers)
  , mStore(store)
  , mEncoder(encoder)
{
}

// FIXME: tesVerIndex stuff is rather clunky, needs to be refactored
void EsmLoader::load(const boost::filesystem::path& filepath, std::vector<std::vector<std::string> >& contentFiles)
{
    int tesVerIndex = 0; // FIXME: hard coded, 0 = MW, 1 = TES4, 2 = TES5 (TODO: Fallout)
    int index = 0;

    ContentLoader::load(filepath.filename(), contentFiles); // set the label on the loading bar

    ESM::ESMReader *lEsm = new ESM::ESMReader();
    lEsm->setEncoder(mEncoder);
    lEsm->setGlobalReaderList(&mEsm[tesVerIndex]);  // global reader list is used by ESMStore::load only
    lEsm->open(filepath.string());

    int esmVer = lEsm->getVer();
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_17;
    bool isFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;

    if (isTes4 || isTes5 || isFONV)
    {
        if (isTes4)
            tesVerIndex = 1;
        else if (isTes5)
            tesVerIndex = 2;
        else
            tesVerIndex = 3;

        // do nothing for now
        return;
    }
    else
    {
        tesVerIndex = 0; // 0 = MW
        index = contentFiles[tesVerIndex].size();
        contentFiles[tesVerIndex].push_back(filepath.filename().string());
        lEsm->setIndex(index);
        mEsm[tesVerIndex].push_back(lEsm);
    }

    mStore.load(*mEsm[tesVerIndex][index], &mListener);
}

} /* namespace MWWorld */
