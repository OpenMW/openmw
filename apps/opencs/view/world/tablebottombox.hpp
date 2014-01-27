#ifndef CSV_WORLD_BOTTOMBOX_H
#define CSV_WORLD_BOTTOMBOX_H

#include <QWidget>
#include <apps/opencs/model/world/universalid.hpp>

class QLabel;
class QStackedLayout;
class QStatusBar;
class QUndoStack;

namespace CSMWorld
{
    class Data;
    class UniversalId;
}

namespace CSVWorld
{
    class CreatorFactoryBase;
    class Creator;

    class TableBottomBox : public QWidget
    {
            Q_OBJECT

            bool mShowStatusBar;
            QLabel *mStatus;
            QStatusBar *mStatusBar;
            int mStatusCount[4];
            Creator *mCreator;
            bool mCreating;
            QStackedLayout *mLayout;

        private:

            // not implemented
            TableBottomBox (const TableBottomBox&);
            TableBottomBox& operator= (const TableBottomBox&);

            void updateStatus();

        public:

            TableBottomBox (const CreatorFactoryBase& creatorFactory, CSMWorld::Data& data,
                QUndoStack& undoStack, const CSMWorld::UniversalId& id, QWidget *parent = 0);

            virtual ~TableBottomBox();

            void setEditLock (bool locked);

            void setStatusBar (bool show);

            bool canCreateAndDelete() const;
            ///< Is record creation and deletion supported?
            ///
            /// \note The BotomBox does not partake in the deletion of records.

        signals:

            void requestFocus (const std::string& id);
            ///< Request owner of this box to focus the just created \a id. The owner may
            /// ignore this request.

        private slots:

            void createRequestDone();
            ///< \note This slot being called does not imply success.

        public slots:

            void selectionSizeChanged (int size);

            void tableSizeChanged (int size, int deleted, int modified);
            ///< \param size Number of not deleted records
            /// \param deleted Number of deleted records
            /// \param modified Number of added and modified records

            void createRequest();
            void cloneRequest(const std::string& id,
                              const CSMWorld::UniversalId::Type type);
    };
}

#endif
