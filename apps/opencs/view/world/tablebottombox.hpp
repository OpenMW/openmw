#ifndef CSV_WORLD_BOTTOMBOX_H
#define CSV_WORLD_BOTTOMBOX_H

#include <QWidget>

class QLabel;

namespace CSVWorld
{
    class CreatorFactoryBase;
    class Creator;

    class TableBottomBox : public QWidget
    {
            Q_OBJECT

            bool mShowStatusBar;
            QLabel *mStatus;
            int mStatusCount[4];
            Creator *mCreator;

        private:

            // not implemented
            TableBottomBox (const TableBottomBox&);
            TableBottomBox& operator= (const TableBottomBox&);

            void updateStatus();

        public:

            TableBottomBox (const CreatorFactoryBase& creatorFactory, QWidget *parent = 0);

            virtual ~TableBottomBox();

            void setStatusBar (bool show);

            bool canCreateAndDelete() const;
            ///< Is record creation and deletion supported?
            ///
            /// \note The BotomBox does not partake in the deletion of records.

        public slots:

            void selectionSizeChanged (int size);

            void tableSizeChanged (int size, int deleted, int modified);
            ///< \param size Number of not deleted records
            /// \param deleted Number of deleted records
            /// \param modified Number of added and modified records

            void createRequest();
    };
}

#endif
