#ifndef CSV_DOC_NEWGAME_H
#define CSV_DOC_NEWGAME_H

#include <experimental/filesystem>

#include <QDialog>
#include <QMetaType>



#ifndef CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
#define CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
Q_DECLARE_METATYPE (std::experimental::filesystem::path)
#endif

class QPushButton;

namespace CSVDoc
{
    class FileWidget;
    class AdjusterWidget;

    class NewGameDialogue : public QDialog
    {
            Q_OBJECT

            QPushButton *mCreate;
            FileWidget *mFileWidget;
            AdjusterWidget *mAdjusterWidget;

        public:

            NewGameDialogue();

            void setLocalData (const std::experimental::filesystem::path& localData);

        signals:

            void createRequest (const std::experimental::filesystem::path& file);

            void cancelCreateGame ();

        private slots:

            void stateChanged (bool valid);

            void create();

            void reject();
    };
}

#endif
