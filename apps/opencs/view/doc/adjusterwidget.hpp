#ifndef CSV_DOC_ADJUSTERWIDGET_H
#define CSV_DOC_ADJUSTERWIDGET_H

#include <boost/filesystem/path.hpp>

#include <QWidget>

class QLabel;

namespace CSVDoc
{
    class AdjusterWidget : public QWidget
    {
            Q_OBJECT

            boost::filesystem::path mLocalData;
            QLabel *mMessage;
            QLabel *mIcon;
            bool mValid;
            boost::filesystem::path mResultPath;

        public:

            AdjusterWidget (QWidget *parent = 0);

            void setLocalData (const boost::filesystem::path& localData);

            boost::filesystem::path getPath() const;
            ///< This function must not be called if there is no valid path.

        public slots:

            void setName (const QString& name, bool addon);

        signals:

            void stateChanged (bool valid);
    };
}

#endif