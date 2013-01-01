#ifndef CSM_TOOLS_TOOLS_H
#define CSM_TOOLS_TOOLS_H

#include <QObject>

#include <map>

namespace CSMWorld
{
    class Data;
    class UniversalId;
}

namespace CSMTools
{
    class Verifier;
    class Operation;
    class ReportModel;

    class Tools : public QObject
    {
            Q_OBJECT

            CSMWorld::Data& mData;
            Verifier *mVerifier;
            std::map<int, ReportModel *> mReports;
            int mNextReportNumber;
            std::map<int, int> mActiveReports; // type, report number

            // not implemented
            Tools (const Tools&);
            Tools& operator= (const Tools&);

            Verifier *getVerifier();

            Operation *get (int type);
            ///< Returns a 0-pointer, if operation hasn't been used yet.

            const Operation *get (int type) const;
            ///< Returns a 0-pointer, if operation hasn't been used yet.

        public:

            Tools (CSMWorld::Data& data);

            virtual ~Tools();

            CSMWorld::UniversalId runVerifier();
            ///< \return ID of the report for this verification run

            void abortOperation (int type);
            ///< \attention The operation is not aborted immediately.

            int getRunningOperations() const;

            ReportModel *getReport (const CSMWorld::UniversalId& id);
            ///< The ownership of the returned report is not transferred.

        private slots:

            void verifierDone();

            void verifierMessage (const QString& message, int type);

        signals:

            void progress (int current, int max, int type);

            void done (int type);
    };
}

#endif
