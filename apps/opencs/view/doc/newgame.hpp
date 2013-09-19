#ifndef CSV_DOC_NEWGAME_H
#define CSV_DOC_NEWGAME_H

#include <boost/filesystem/path.hpp>

#include <QDialog>
#include <QMetaType>

Q_DECLARE_METATYPE (boost::filesystem::path)

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

            void setLocalData (const boost::filesystem::path& localData);

        signals:

            void createRequest (const boost::filesystem::path& file);

        private slots:

            void stateChanged (bool valid);

            void create();
    };
}

#endif
