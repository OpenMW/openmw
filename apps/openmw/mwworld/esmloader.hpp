#ifndef ESMLOADER_HPP
#define ESMLOADER_HPP

#include <optional>

#include "contentloader.hpp"

namespace ToUTF8
{
  class Utf8Encoder;
}

namespace ESM
{
    class ReadersCache;
    struct Dialogue;
}

namespace MWWorld
{

class ESMStore;

struct EsmLoader : public ContentLoader
{
    explicit EsmLoader(MWWorld::ESMStore& store, ESM::ReadersCache& readers, ToUTF8::Utf8Encoder* encoder);

    std::optional<int> getMasterFileFormat() const { return mMasterFileFormat; }

    void load(const boost::filesystem::path& filepath, int& index, Loading::Listener* listener) override;

    private:
        ESM::ReadersCache& mReaders;
        MWWorld::ESMStore& mStore;
        ToUTF8::Utf8Encoder* mEncoder;
        ESM::Dialogue* mDialogue;
        std::optional<int> mMasterFileFormat;
};

} /* namespace MWWorld */

#endif // ESMLOADER_HPP
