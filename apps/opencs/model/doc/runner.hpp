#ifndef CSM_DOC_RUNNER_H
#define CSM_DOC_RUNNER_H

#include <QObject>
#include <QProcess>

namespace CSMDoc
{
    class Runner : public QObject
    {
            Q_OBJECT

            QProcess mProcess;

        public:

            Runner();

            void start();

            void stop();

        signals:

            void runStateChanged (bool running);

        private slots:

            void finished (int exitCode, QProcess::ExitStatus exitStatus);
    };
}

#endif
