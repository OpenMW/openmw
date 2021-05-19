#ifndef CSV_DOC_OPERATION_H
#define CSV_DOC_OPERATION_H

#include <QObject>

class QHBoxLayout;
class QPushButton;
class QProgressBar;

namespace CSVDoc
{
    class Operation : public QObject
    {
            Q_OBJECT

            int mType;
            bool mStalling;
            QProgressBar *mProgressBar;
            QPushButton *mAbortButton;
            QHBoxLayout *mLayout;

            // not implemented
            Operation (const Operation&);
            Operation& operator= (const Operation&);

            void updateLabel (int threads = -1);

        public:

            Operation (int type, QWidget *parent);
            ~Operation() override;

            void setProgress (int current, int max, int threads);

            int getType() const;
            QHBoxLayout *getLayout() const;

        private:

            void setBarColor (int type);
            void initWidgets();

        signals:

            void abortOperation (int type);

        private slots:

            void abortOperation();
    };
}

#endif
