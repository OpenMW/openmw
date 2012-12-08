#ifndef CSM_TOOLS_TOOLS_H
#define CSM_TOOLS_TOOLS_H

#include <QObject>

namespace CSMTools
{
    class Verifier;

    class Tools : public QObject
    {
            Q_OBJECT

            Verifier *mVerifier;

            // not implemented
            Tools (const Tools&);
            Tools& operator= (const Tools&);

            Verifier *getVerifier();

        public:

            Tools();

            virtual ~Tools();

            void runVerifier();

            void abortOperation (int type);
            ///< \attention The operation is not aborted immediately.

        private slots:

            void verifierDone();

        signals:

            void progress (int current, int max, int type);

            void done (int type);
    };
}

#endif
