#ifndef CSM_DOC_SAVINGSTATE_H
#define CSM_DOC_SAVINGSTATE_H

#include <fstream>

#include <boost/filesystem/path.hpp>

#include <components/esm/esmwriter.hpp>

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
            std::ofstream mStream;
            ESM::ESMWriter mWriter;

        public:

            SavingState (Operation& operation);

            bool hasError() const;

            void start (Document& document);

            const boost::filesystem::path& getPath() const;

            const boost::filesystem::path& getTmpPath() const;

            std::ofstream& getStream();

            ESM::ESMWriter& getWriter();
    };


}

#endif