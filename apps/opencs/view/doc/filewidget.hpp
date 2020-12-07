#ifndef CSV_DOC_FILEWIDGET_H
#define CSV_DOC_FILEWIDGET_H

#include <QWidget>

#include <string>

class QLabel;
class QString;
class QLineEdit;

namespace CSVDoc
{
    class FileWidget : public QWidget
    {
            Q_OBJECT

            bool mAddon;
            QLineEdit *mInput;
            QLabel *mType;

            QString getExtension() const;

        public:

            FileWidget (QWidget *parent = nullptr);

            void setType (bool addon);

            QString getName() const;

            void extensionLabelIsVisible(bool visible);

            void setName (const std::string& text);

        private slots:

            void textChanged (const QString& text);

        signals:

            void nameChanged (const QString& file, bool addon);
    };
}

#endif
