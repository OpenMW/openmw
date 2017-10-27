#ifndef CSV_DOC_NEWGAME_H
#define CSV_DOC_NEWGAME_H

#include <experimental/filesystem>

#include <QDialog>
#include <QMetaType>

namespace sfs = std::experimental::filesystem;

#ifndef CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
#define CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
Q_DECLARE_METATYPE (sfs::path)
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

            void setLocalData (const sfs::path& localData);

        signals:

            void createRequest (const sfs::path& file);

            void cancelCreateGame ();

        private slots:

            void stateChanged (bool valid);

            void create();

            void reject();
    };
}

#endif
