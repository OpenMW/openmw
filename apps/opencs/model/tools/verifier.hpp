#ifndef CSM_TOOLS_VERIFIER_H
#define CSM_TOOLS_VERIFIER_H

#include <QThread>

namespace CSMTools
{
    class Verifier : public QThread
    {
            Q_OBJECT

            int mStep;

        public:

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
