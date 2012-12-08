#ifndef CSM_TOOLS_TOOLS_H
#define CSM_TOOLS_TOOLS_H

#include <QObject>

namespace CSMTools
{
    class Verifier;
    class Operation;

    class Tools : public QObject
    {
            Q_OBJECT

            Verifier *mVerifier;

            // not implemented
            Tools (const Tools&);
            Tools& operator= (const Tools&);

            Verifier *getVerifier();

            Operation *get (int type);
            ///< Returns a 0-pointer, if operation hasn't been used yet.

            const Operation *get (int type) const;
            ///< Returns a 0-pointer, if operation hasn't been used yet.

        public:

            Tools();

            virtual ~Tools();

            void runVerifier();

            void abortOperation (int type);
            ///< \attention The operation is not aborted immediately.

            int getRunningOperations() const;

        private slots:

            void verifierDone();

            void verifierMessage (const QString& message, int type);

        signals:

            void progress (int current, int max, int type);

            void done (int type);
    };
}

#endif
