#ifndef CSM_DOC_SAVINGSTATE_H
#define CSM_DOC_SAVINGSTATE_H

#include <fstream>
#include <map>
#include <deque>

#include <experimental/filesystem>

#include <components/esm/esmwriter.hpp>

#include <components/to_utf8/to_utf8.hpp>

namespace sfs = std::experimental::filesystem;

namespace CSMDoc
{
    class Operation;
    class Document;

    class SavingState
    {
            Operation& mOperation;
            sfs::path mPath;
            sfs::path mTmpPath;
            ToUTF8::Utf8Encoder mEncoder;
            std::ofstream mStream;
            ESM::ESMWriter mWriter;
            sfs::path mProjectPath;
            bool mProjectFile;
            std::map<std::string, std::deque<int> > mSubRecords; // record ID, list of subrecords

        public:

            SavingState (Operation& operation, const sfs::path& projectPath,
                ToUTF8::FromType encoding);

            bool hasError() const;

            void start (Document& document, bool project);
            ///< \param project Save project file instead of content file.

            const sfs::path& getPath() const;

            const sfs::path& getTmpPath() const;

            std::ofstream& getStream();

            ESM::ESMWriter& getWriter();

            bool isProjectFile() const;
            ///< Currently saving project file? (instead of content file)

            std::map<std::string, std::deque<int> >& getSubRecords();
    };


}

#endif
