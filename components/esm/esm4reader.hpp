#ifndef COMPONENT_ESM_4READER_H
#define COMPONENT_ESM_4READER_H

#include <extern/esm4/tes4.hpp>
#include <extern/esm4/reader.hpp>

#include "esmreader.hpp"

namespace ESM
{
    // Wrapper class for integrating into OpenCS
    class ESM4Reader : public ESMReader, public ESM4::ReaderObserver
    {
        ESM4::Reader mReader;

    public:

        ESM4Reader(bool oldHeader = true);
        virtual ~ESM4Reader();

        ESM4::Reader& reader() { return mReader; }

        // Added for use with OpenMW (loading progress bar)
        inline size_t getFileSize() { return mReader.getFileSize(); }
        inline size_t getFileOffset() { return mReader.getFileOffset(); }

        // Added for loading Cell/Land
        ESM4::ReaderContext getESM4Context();
        void restoreESM4Context(const ESM4::ReaderContext& ctx);
        void restoreCellChildrenContext(const ESM4::ReaderContext& ctx);

        void openTes4File(const std::string &name);

        // callback from mReader to ensure hasMoreRecs() can reliably track to EOF
        inline void update(std::size_t size) { mCtx.leftFile -= size; }
    };
}
#endif // COMPONENT_ESM_4READER_H
