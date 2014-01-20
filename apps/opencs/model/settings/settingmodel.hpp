#ifndef CSMSETTINGS_SETTINGMODEL_HPP
#define CSMSETTINGS_SETTINGMODEL_HPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QMap>
#include <QStringList>

#include "setting.hpp"
#include "../../view/settings/support.hpp"
#include "declarationmodel.hpp"
#include "definitionmodel.hpp"

namespace CSMSettings
{

    class WriterSortModel : public QSortFilterProxyModel
    {
        bool lessThan(const QModelIndex &rLeft, const QModelIndex &rRight) const
        {
            int const leftRow  = rLeft.row();
            int const rightRow = rRight.row();

            for (int column = Setting_Page; column >= Setting_Name; --column)
            {
                QModelIndex const leftIdx = sourceModel()->index
                                    (leftRow, column, QModelIndex());

                QModelIndex const rightIdx = sourceModel()->index
                                    (rightRow, column, QModelIndex());

                QString const leftData = sourceModel()->
                                                    data(leftIdx).toString();

                QString const rightData = sourceModel()->
                                                    data(rightIdx).toString();

                int const compare = QString::localeAwareCompare
                                                        (leftData, rightData);
                if (compare != 0)
                    return (compare < 0);
            }
            return false;
        }
    };

    typedef QMap <QString, QStringList *> SettingMap;
    typedef QMap <QString, SettingMap *> PageMap;

    class SettingModel : public QObject
    {

        Q_OBJECT

        DefinitionModel mDefinitionModel;
        DeclarationModel mDeclarationModel;

        QString mReadOnlyMessage;
        QString mReadWriteMessage;

    public:

        explicit SettingModel(QObject *parent = 0);

        DeclarationModel &declarationModel()
                                                { return mDeclarationModel;}

        DefinitionModel &definitionModel()
                                                { return mDefinitionModel; }
    protected:

        void validate(PageMap &pageMap);

        PageMap readFilestream(QTextStream *stream);

        bool writeFilestream (QTextStream *stream);

        void mergeSettings (PageMap &destMap, PageMap &srcMap,
                            MergeMethod mergeMethod = Merge_Accept);

        QTextStream *openFilestream (const QString &filePath,
                                     bool isReadOnly) const;

        void destroyStream(QTextStream *stream) const;

        void displayFileErrorMessage(const QString &message,
                                     bool isReadOnly) const;

        void sortAndDump();
    };
}
#endif // CSMSETTINGS_SETTINGMODEL_HPP
