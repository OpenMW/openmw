#ifndef CSV_DOC_ADJUSTERWIDGET_H
#define CSV_DOC_ADJUSTERWIDGET_H

#include <experimental/filesystem>

#include <QWidget>



class QLabel;

namespace CSVDoc
{
    enum ContentAction
    {
        ContentAction_New,
        ContentAction_Edit,
        ContentAction_Undefined
    };

    class AdjusterWidget : public QWidget
    {
            Q_OBJECT

        public:

            std::experimental::filesystem::path mLocalData;
            QLabel *mMessage;
            QLabel *mIcon;
            bool mValid;
            std::experimental::filesystem::path mResultPath;
            ContentAction mAction;
            bool mDoFilenameCheck;

        public:

            AdjusterWidget (QWidget *parent = 0);

            void setLocalData (const std::experimental::filesystem::path& localData);
            void setAction (ContentAction action);

            void setFilenameCheck (bool doCheck);
            bool isValid() const;

            std::experimental::filesystem::path getPath() const;
            ///< This function must not be called if there is no valid path.

        public slots:

            void setName (const QString& name, bool addon);

        signals:

            void stateChanged (bool valid);
    };
}

#endif
