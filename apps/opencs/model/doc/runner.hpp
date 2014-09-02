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
            bool mRunning;

        public:

            Runner();

            void start();

            void stop();

            /// \note Running state is entered when the start function is called. This
            /// is not necessarily identical to the moment the child process is started.
            bool isRunning() const;

        signals:

            void runStateChanged();

        private slots:

            void finished (int exitCode, QProcess::ExitStatus exitStatus);
    };
}

#endif
