#include "esmloader.hpp"
#include "esmstore.hpp"

#include "components/to_utf8/to_utf8.hpp"

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mStore(store)
  , mEsm(readers)
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
