#ifndef CSM_DOC_OPERATION_H
#define CSM_DOC_OPERATION_H

#include <vector>

#include <QObject>
#include <QTimer>

namespace CSMWorld
{
    class UniversalId;
}

namespace CSMDoc
{
    class Stage;

    class Operation : public QObject
    {
            Q_OBJECT

            int mType;
            std::vector<std::pair<Stage *, int> > mStages; // stage, number of steps
            std::vector<std::pair<Stage *, int> >::iterator mCurrentStage;
            int mCurrentStep;
            int mCurrentStepTotal;
            int mTotalSteps;
            int mOrdered;
            bool mFinalAlways;
            bool mError;
            bool mConnected;
            QTimer *mTimer;

            void prepareStages();

        public:

            Operation (int type, bool ordered, bool finalAlways = false);
            ///< \param ordered Stages must be executed in the given order.
            /// \param finalAlways Execute last stage even if an error occurred during earlier stages.

            virtual ~Operation();

            void appendStage (Stage *stage);
            ///< The ownership of \a stage is transferred to *this.
            ///
            /// \attention Do no call this function while this Operation is running.

            bool hasError() const;

        signals:

            void progress (int current, int max, int type);

            void reportMessage (const CSMWorld::UniversalId& id, const std::string& message,
                const std::string& hint, int type);

            void done (int type, bool failed);

        public slots:

            void abort();

            void run();

        private slots:

            void executeStage();

            void operationDone();
    };
}

#endif
