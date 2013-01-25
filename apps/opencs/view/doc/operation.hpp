#ifndef CSV_DOC_OPERATION_H
#define CSV_DOC_OPERATION_H

#include <QProgressBar>

namespace CSVDoc
{
    class Operation : public QProgressBar
    {
            Q_OBJECT

            int mType;
            bool mStalling;

            // not implemented
            Operation (const Operation&);
            Operation& operator= (const Operation&);

            void updateLabel (int threads = -1);

        public:

            Operation (int type);

            void setProgress (int current, int max, int threads);

            int getType() const;
    };
}

#endif