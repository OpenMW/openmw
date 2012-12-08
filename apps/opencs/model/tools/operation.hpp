#ifndef CSM_TOOLS_OPERATION_H
#define CSM_TOOLS_OPERATION_H

#include <QThread>

namespace CSMTools
{
    class Operation : public QThread
    {
            Q_OBJECT

            int mType;
            int mStep;

        public:

            Operation (int type);

            virtual void run();

        signals:

            void progress (int current, int max, int type);

        public slots:

            void abort();

        private slots:

            void verify();
    };
}

#endif