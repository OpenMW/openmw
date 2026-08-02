#ifndef CSM_DOC_OPERATIONHOLDER_H
#define CSM_DOC_OPERATIONHOLDER_H

#include <QObject>
#include <QThread>

namespace CSMDoc
{
    class Operation;
    struct Message;

    class OperationHolder : public QObject
    {
        Q_OBJECT

        QThread mThread;
        Operation* mOperation;

    public:
        OperationHolder(QObject* parent, Operation* operation);

        ~OperationHolder();

        bool isRunning() const;

        void start();

        void abort();

        /// Stop the operation, wait for the thread to finish, and delete the operation.
        /// Safe to call multiple times.
        void quit();

    private slots:

        void doneSlot(int type, bool failed);

    signals:

        void progress(int current, int max, int type);

        void reportMessage(const CSMDoc::Message& message, int type);

        void done(int type, bool failed);

        void abortSignal();
    };
}

#endif
