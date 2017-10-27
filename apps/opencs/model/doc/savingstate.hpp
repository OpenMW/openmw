#ifndef CSM_DOC_SAVINGSTATE_H
#define CSM_DOC_SAVINGSTATE_H

#include <fstream>
#include <map>
#include <deque>

#include <experimental/filesystem>

#include <components/esm/esmwriter.hpp>

#include <components/to_utf8/to_utf8.hpp>



namespace CSMDoc
{
    class Operation;
    class Document;

    class SavingState
    {
            Operation& mOperation;
            std::experimental::filesystem::path mPath;
            std::experimental::filesystem::path mTmpPath;
            ToUTF8::Utf8Encoder mEncoder;
            std::ofstream mStream;
            ESM::ESMWriter mWriter;
            std::experimental::filesystem::path mProjectPath;
            bool mProjectFile;
            std::map<std::string, std::deque<int> > mSubRecords; // record ID, list of subrecords

        public:

            SavingState (Operation& operation, const std::experimental::filesystem::path& projectPath,
                ToUTF8::FromType encoding);

            bool hasError() const;

            void start (Document& document, bool project);
            ///< \param project Save project file instead of content file.

            const std::experimental::filesystem::path& getPath() const;

            const std::experimental::filesystem::path& getTmpPath() const;

            std::ofstream& getStream();

            ESM::ESMWriter& getWriter();

            bool isProjectFile() const;
            ///< Currently saving project file? (instead of content file)

            std::map<std::string, std::deque<int> >& getSubRecords();
    };


}

#endif
