#ifndef CSM_DOC_OPERATION_H
#define CSM_DOC_OPERATION_H

#include <vector>

#include <QThread>

namespace CSMWorld
{
    class UniversalId;
}

namespace CSMDoc
{
    class Stage;

    class Operation : public QThread
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

            void prepareStages();

        public:

            Operation (int type, bool ordered, bool finalAlways = false);
            ///< \param ordered Stages must be executed in the given order.
            /// \param finalAlways Execute last stage even if an error occurred during earlier stages.

            virtual ~Operation();

            virtual void run();

            void appendStage (Stage *stage);
            ///< The ownership of \a stage is transferred to *this.
            ///
            /// \attention Do no call this function while this Operation is running.

            bool hasError() const;

        signals:

            void progress (int current, int max, int type);

            void reportMessage (const CSMWorld::UniversalId& id, const std::string& message,
                int type);

            void done (int type);

        public slots:

            void abort();

        private slots:

            void executeStage();

            void operationDone();
    };
}

#endif