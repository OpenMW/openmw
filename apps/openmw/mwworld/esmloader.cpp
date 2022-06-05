#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/readerscache.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, ESM::ReadersCache& readers, ToUTF8::Utf8Encoder* encoder)
    : mReaders(readers)
    , mStore(store)
    , mEncoder(encoder)
    , mDialogue(nullptr) // A content file containing INFO records without a DIAL record appends them to the previous file's dialogue
{
}

void EsmLoader::load(const boost::filesystem::path& filepath, int& index, Loading::Listener* listener)
{
    const ESM::ReadersCache::BusyItem reader = mReaders.get(static_cast<std::size_t>(index));

    reader->setEncoder(mEncoder);
    reader->setIndex(index);
    reader->open(filepath.string());
    reader->resolveParentFileIndices(mReaders);

    assert(reader->getGameFiles().size() == reader->getParentFileIndices().size());
    for (std::size_t i = 0, n = reader->getParentFileIndices().size(); i < n; ++i)
        if (i == static_cast<std::size_t>(reader->getIndex()))
            throw std::runtime_error("File " + reader->getName() + " asks for parent file "
                + reader->getGameFiles()[i].name
                + ", but it is not available or has been loaded in the wrong order. "
                  "Please run the launcher to fix this issue.");

    mStore.load(*reader, listener, mDialogue);

    if (!mMasterFileFormat.has_value() && (Misc::StringUtils::ciEndsWith(reader->getName(), ".esm")
                                           || Misc::StringUtils::ciEndsWith(reader->getName(), ".omwgame")))
        mMasterFileFormat = reader->getFormat();
}

} /* namespace MWWorld */
