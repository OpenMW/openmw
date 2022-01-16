#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& readers,
    ToUTF8::Utf8Encoder* encoder)
    : mEsm(readers)
    , mStore(store)
    , mEncoder(encoder)
{
}

void EsmLoader::load(const boost::filesystem::path& filepath, int& index, Loading::Listener* listener)
{
    ESM::ESMReader lEsm;
    lEsm.setEncoder(mEncoder);
    lEsm.setIndex(index);
    lEsm.open(filepath.string());
    lEsm.resolveParentFileIndices(mEsm);
    mEsm[index] = lEsm;
    mStore.load(mEsm[index], listener);
}

} /* namespace MWWorld */
