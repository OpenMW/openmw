#ifndef ESMLOADER_HPP
#define ESMLOADER_HPP

#include <vector>

#include "contentloader.hpp"

namespace ToUTF8
{
  class Utf8Encoder;
}

namespace ESM
{
    class ESMReader;
}

namespace MWWorld
{

class ESMStore;

struct EsmLoader : public ContentLoader
{
    EsmLoader(MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& readers,
        ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener);

    void load(const boost::filesystem::path& filepath, std::vector<std::vector<std::string> >& contentFiles);

    private:
        std::vector<std::vector<ESM::ESMReader*> >& mEsm; // Note: the ownership of the readers is with the caller
        MWWorld::ESMStore& mStore;
        ToUTF8::Utf8Encoder* mEncoder;
};

} /* namespace MWWorld */

#endif // ESMLOADER_HPP
