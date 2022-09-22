#ifndef CSV_DOC_ADJUSTERWIDGET_H
#define CSV_DOC_ADJUSTERWIDGET_H

#include <QWidget>

#include <filesystem>

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
        std::filesystem::path mLocalData;
        QLabel* mMessage;
        QLabel* mIcon;
        bool mValid;
        std::filesystem::path mResultPath;
        ContentAction mAction;
        bool mDoFilenameCheck;

    public:
        AdjusterWidget(QWidget* parent = nullptr);

        void setLocalData(const std::filesystem::path& localData);
        void setAction(ContentAction action);

        void setFilenameCheck(bool doCheck);
        bool isValid() const;

        std::filesystem::path getPath() const;
        ///< This function must not be called if there is no valid path.

    public slots:

        void setName(const QString& name, bool addon);

    signals:

        void stateChanged(bool valid);
    };
}

#endif
