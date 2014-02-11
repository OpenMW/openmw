#ifndef CSMSETTINGS_OLDSETTINGMODEL_HPP
#define CSMSETTINGS_OLDSETTINGMODEL_HPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QMap>
#include <QStringList>
#include <QStandardItemModel>

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

            for (int column = Property_Page; column >= Property_Name; --column)
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



    class SettingModel : public QObject
    {

        Q_OBJECT

        QStandardItemModel mStdDefModel;
        QStandardItemModel mStdDecModel;
        /*
        DefinitionModel mDefinitionModel;
        DeclarationModel mDeclarationModel;
*/
        QString mReadOnlyMessage;
        QString mReadWriteMessage;

    public:

        explicit SettingModel(QObject *parent = 0);

        bool defineSetting (const QString &settingName,
                             const QString &pageName,
                             const QString &value);
/*
        ISettingDeclaration *declareSingleText (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleText, name, page, defaultValue); }

        ISettingDeclaration *declareMultiText  (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_MultiText, name, page, defaultValue); }

        ISettingDeclaration *declareSingleBool (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleBool, name, page, defaultValue); }

        ISettingDeclaration *declareMultiBool  (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_MultiBool, name, page, defaultValue); }

        ISettingDeclaration *declareSingleRange (const QString &name,
                                             const QString &page,
                                             const QString &defaultValue)
        { return declareSetting (Type_SingleRange, name, page, defaultValue); }

        ISettingDeclaration *declareMultiRange (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleRange, name, page, defaultValue); }

        ISettingDeclaration *declareSingleList (const QString &name,
                                            const QString &page,
                                            const QString &defaultValue)
        { return declareSetting (Type_SingleList, name, page, defaultValue); }

        ISettingDeclaration *declareMultiList (const QString &name,
                                           const QString &page,
                                           const QString &defaultValue)
        { return declareSetting (Type_SingleList, name, page, defaultValue); }

        ISettingDeclaration *declareSetting (SettingType typ,
                                             const QString &name,
                                             const QString &page,
                                             const QString &defaultValue);

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

        void buildModel (PageMap &pageMap);

    private:

        void removeUndeclaredDefinitions (PageMap &pageMap);
        void validateDefinitions (PageMap &pageMap);*/
    };
}
#endif // CSMSETTINGS_OLDSETTINGMODEL_HPP
