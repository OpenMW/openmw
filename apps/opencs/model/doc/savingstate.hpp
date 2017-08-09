#ifndef CSM_DOC_SAVINGSTATE_H
#define CSM_DOC_SAVINGSTATE_H

#include <fstream>
#include <map>
#include <deque>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/esm/esmwriter.hpp>

#include <components/to_utf8/to_utf8.hpp>

namespace CSMDoc
{
    class Operation;
    class Document;

    class SavingState
    {
            Operation& mOperation;
            boost::filesystem::path mPath;
            boost::filesystem::path mTmpPath;
            ToUTF8::Utf8Encoder mEncoder;
            boost::filesystem::ofstream mStream;
            ESM::ESMWriter mWriter;
            boost::filesystem::path mProjectPath;
            bool mProjectFile;
            std::map<std::string, std::deque<int> > mSubRecords; // record ID, list of subrecords

        public:

            SavingState (Operation& operation, const boost::filesystem::path& projectPath,
                ToUTF8::FromType encoding);

            bool hasError() const;

            void start (Document& document, bool project);
            ///< \param project Save project file instead of content file.

            const boost::filesystem::path& getPath() const;

            const boost::filesystem::path& getTmpPath() const;

            boost::filesystem::ofstream& getStream();

            ESM::ESMWriter& getWriter();

            bool isProjectFile() const;
            ///< Currently saving project file? (instead of content file)

            std::map<std::string, std::deque<int> >& getSubRecords();
    };


}

#endif
