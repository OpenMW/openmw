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
    struct CreatureStats;
    struct InventoryState;
    struct NpcStats;
}

namespace MWWorld
{

class ESMStore;

struct EsmLoader : public ContentLoader
{
    EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& readers,
      ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener);

    void load(const boost::filesystem::path& filepath, int& index) override;

    private:
      std::vector<ESM::ESMReader>& mEsm;
      MWWorld::ESMStore& mStore;
      ToUTF8::Utf8Encoder* mEncoder;
};

void convertMagicEffects(ESM::CreatureStats& creatureStats, ESM::InventoryState& inventory, ESM::NpcStats* npcStats = nullptr);

} /* namespace MWWorld */

#endif // ESMLOADER_HPP
