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

            ~Runner();

            /// \param delayed Flag as running but do not start the OpenMW process yet (the
            /// process must be started by another call of start with delayed==false)
            void start (bool delayed = false);

            void stop();

            /// \note Running state is entered when the start function is called. This
            /// is not necessarily identical to the moment the child process is started.
            bool isRunning() const;

        signals:

            void runStateChanged();

        private slots:

            void finished (int exitCode, QProcess::ExitStatus exitStatus);
    };

    class Operation;

    /// \brief Watch for end of save operation and restart or stop runner
    class SaveWatcher : public QObject
    {
            Q_OBJECT

            Runner *mRunner;

        public:

            /// *this attaches itself to runner
            SaveWatcher (Runner *runner, Operation *operation);

        private slots:

            void saveDone (int type, bool failed);
    };
}

#endif
