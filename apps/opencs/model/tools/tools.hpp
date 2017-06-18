#ifndef CSM_TOOLS_TOOLS_H
#define CSM_TOOLS_TOOLS_H

#include <memory>
#include <map>

#include <components/to_utf8/to_utf8.hpp>

#include <QObject>

#include <boost/filesystem/path.hpp>

#include "../doc/operationholder.hpp"

namespace CSMWorld
{
    class Data;
    class UniversalId;
}

namespace CSMDoc
{
    class Operation;
    class Document;
}

namespace CSMTools
{
    class ReportModel;
    class Search;
    class SearchOperation;
    class MergeOperation;

    class Tools : public QObject
    {
            Q_OBJECT

            CSMDoc::Document& mDocument;
            CSMWorld::Data& mData;
            CSMDoc::Operation *mVerifierOperation;
            CSMDoc::OperationHolder mVerifier;
            SearchOperation *mSearchOperation;
            CSMDoc::OperationHolder mSearch;
            MergeOperation *mMergeOperation;
            CSMDoc::OperationHolder mMerge;
            std::map<int, ReportModel *> mReports;
            int mNextReportNumber;
            std::map<int, int> mActiveReports; // type, report number
            ToUTF8::FromType mEncoding;

            // not implemented
            Tools (const Tools&);
            Tools& operator= (const Tools&);

            CSMDoc::OperationHolder *getVerifier();

            CSMDoc::OperationHolder *get (int type);
            ///< Returns a 0-pointer, if operation hasn't been used yet.

            const CSMDoc::OperationHolder *get (int type) const;
            ///< Returns a 0-pointer, if operation hasn't been used yet.

        public:

            Tools (CSMDoc::Document& document, ToUTF8::FromType encoding);

            virtual ~Tools();

            /// \param reportId If a valid VerificationResults ID, run verifier for the
            /// specified report instead of creating a new one.
            ///
            /// \return ID of the report for this verification run
            CSMWorld::UniversalId runVerifier (const CSMWorld::UniversalId& reportId = CSMWorld::UniversalId());

            /// Return ID of the report for this search.
            CSMWorld::UniversalId newSearch();

            void runSearch (const CSMWorld::UniversalId& searchId, const Search& search);

            void runMerge (std::unique_ptr<CSMDoc::Document> target);

            void abortOperation (int type);
            ///< \attention The operation is not aborted immediately.

            int getRunningOperations() const;

            ReportModel *getReport (const CSMWorld::UniversalId& id);
            ///< The ownership of the returned report is not transferred.

        private slots:

            void verifierMessage (const CSMDoc::Message& message, int type);

        signals:

            void progress (int current, int max, int type);

            void done (int type, bool failed);

            /// \attention When this signal is emitted, *this hands over the ownership of the
            /// document. This signal must be handled to avoid a leak.
            void mergeDone (CSMDoc::Document *document);
    };
}

#endif
