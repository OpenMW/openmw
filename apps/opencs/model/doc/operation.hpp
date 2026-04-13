#ifndef CSM_DOC_OPERATION_H
#define CSM_DOC_OPERATION_H

#include <chrono>
#include <optional>
#include <utility>
#include <vector>

#include <QObject>

#include "messages.hpp"
#include "state.hpp"

namespace CSMDoc
{
    class Stage;

    class Operation : public QObject
    {
        Q_OBJECT

        State mType;
        std::vector<std::pair<Stage*, int>> mStages; // stage, number of steps
        std::vector<std::pair<Stage*, int>>::iterator mCurrentStage;
        int mCurrentStep;
        int mCurrentStepTotal;
        int mTotalSteps;
        bool mFinalAlways;
        bool mError;
        bool mPrepared;
        Message::Severity mDefaultSeverity;
        std::optional<std::chrono::steady_clock::time_point> mStart;

        void prepareStages();

    public:
        Operation(State type, bool finalAlways = false);
        /// \param finalAlways Execute last stage even if an error occurred during earlier stages.

        virtual ~Operation();

        void appendStage(Stage* stage);
        ///< The ownership of \a stage is transferred to *this.
        ///
        /// \attention Do no call this function while this Operation is running.

        /// \attention Do no call this function while this Operation is running.
        void setDefaultSeverity(Message::Severity severity);

        bool hasError() const;

    signals:

        void progress(int current, int max, int type);

        void reportMessage(const CSMDoc::Message& message, int type);

        void done(int type, bool failed);

    public slots:

        void abort();

        void run();

        /// Stop the timer and move this operation back to the main thread.
        /// Called via DirectConnection from QThread::finished so it executes
        /// on the worker thread before it fully exits.
        void cleanup();

    private slots:

        void executeStage();

    protected slots:

        virtual void operationDone();
    };
}

#endif
