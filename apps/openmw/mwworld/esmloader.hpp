#ifndef ESMLOADER_HPP
#define ESMLOADER_HPP

#include <vector>

#include "contentloader.hpp"
#include "components/esm/esmreader.hpp"

namespace ToUTF8
{
  class Utf8Encoder;
}

namespace MWWorld
{

class ESMStore;

struct EsmLoader : public ContentLoader
{
    EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& readers,
      ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener);

    void load(const boost::filesystem::path& filepath, int& index);

    private:
      std::vector<ESM::ESMReader>& mEsm;
      MWWorld::ESMStore& mStore;
      ToUTF8::Utf8Encoder* mEncoder;
};

} /* namespace MWWorld */

#endif // ESMLOADER_HPP
