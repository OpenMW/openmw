#ifndef CSM_DOC_OPERATIONHOLDER_H
#define CSM_DOC_OPERATIONHOLDER_H

#include <QObject>
#include <QThread>

#include "messages.hpp"

namespace CSMWorld
{
    class UniversalId;
}

namespace CSMDoc
{
    class Operation;

    class OperationHolder : public QObject
    {
            Q_OBJECT
            
            QThread mThread;
            Operation *mOperation;
            bool mRunning;

        public:

            OperationHolder (Operation *operation = nullptr);

            void setOperation (Operation *operation);

            bool isRunning() const;

            void start();

            void abort();

            // Abort and wait until thread has finished.
            void abortAndWait();

        private slots:

            void doneSlot (int type, bool failed);
            
        signals:

            void progress (int current, int max, int type);

            void reportMessage (const CSMDoc::Message& message, int type);

            void done (int type, bool failed);

            void abortSignal();
    };
}

#endif
