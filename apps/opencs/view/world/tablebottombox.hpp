#ifndef CSV_WORLD_BOTTOMBOX_H
#define CSV_WORLD_BOTTOMBOX_H

#include <QWidget>

class QLabel;

namespace CSVWorld
{
    class TableBottomBox : public QWidget
    {
            Q_OBJECT

            bool mShowStatusBar;
            QLabel *mStatus;
            int mStatusCount[4];

        private:

            void updateStatus();

        public:

            TableBottomBox (QWidget *parent = 0);

            void setStatusBar (bool show);

        public slots:

            void selectionSizeChanged (int size);

            void tableSizeChanged (int size, int deleted, int modified);
            ///< \param size Number of not deleted records
            /// \param deleted Number of deleted records
            /// \param modified Number of added and modified records
    };
}

#endif
