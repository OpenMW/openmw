#ifndef CSV_DOC_OPERATIONS_H
#define CSV_DOC_OPERATIONS_H

#include <vector>

#include <QDockWidget>

class QVBoxLayout;

namespace CSVDoc
{
    class Operation;

    class Operations : public QDockWidget
    {
            Q_OBJECT

            QVBoxLayout *mLayout;
            std::vector<Operation *> mOperations;

            // not implemented
            Operations (const Operations&);
            Operations& operator= (const Operations&);

        public:

            Operations();

            void setProgress (int current, int max, int type, int threads);
            ///< Implicitly starts the operation, if it is not running already.

            void quitOperation (int type);
            ///< Calling this function for an operation that is not running is a no-op.
    };
}

#endif