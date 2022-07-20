#ifndef CSV_DOC_ADJUSTERWIDGET_H
#define CSV_DOC_ADJUSTERWIDGET_H

#include <boost/filesystem/path.hpp>

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

            boost::filesystem::path mLocalData;
            QLabel *mMessage;
            QLabel *mIcon;
            bool mValid;
            boost::filesystem::path mResultPath;
            ContentAction mAction;
            bool mDoFilenameCheck;

        public:

            AdjusterWidget (QWidget *parent = nullptr);

            void setLocalData (const boost::filesystem::path& localData);
            void setAction (ContentAction action);

            void setFilenameCheck (bool doCheck);
            bool isValid() const;

            boost::filesystem::path getPath() const;
            ///< This function must not be called if there is no valid path.

        public slots:

            void setName (const QString& name, bool addon);

        signals:

            void stateChanged (bool valid);
    };
}

#endif
