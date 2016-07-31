#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mEsm(readers)
  , mStore(store)
  , mEncoder(encoder)
{
}

void EsmLoader::load(const boost::filesystem::path& filepath, int& index)
{
  ContentLoader::load(filepath.filename(), index);

  ESM::ESMReader lEsm;
  lEsm.setEncoder(mEncoder);
  lEsm.setIndex(index);
  lEsm.setGlobalReaderList(&mEsm);
  lEsm.open(filepath.string());
  mEsm[index] = lEsm;
  mStore.load(mEsm[index], &mListener);
}

} /* namespace MWWorld */
