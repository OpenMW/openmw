#ifndef CSVSETTINGS_PAGE_HPP
#define CSVSETTINGS_PAGE_HPP

#include <QWidget>

#include "support.hpp"
#include "settingbox.hpp"

class QSortFilterProxyModel;
class QGroupBox;
class QAbstractItemModel;

namespace CSMSettings
{
    class Setting;
    class DeclarationModel;
    class DefinitionModel;
}

namespace CSVSettings
{
    class View;
    class SettingBox;
    class IViewFactory;

    class Page : public QWidget
    {
        Q_OBJECT

        QList<View *> mViews;
        SettingBox *mBox;
        static QMap <ViewType, IViewFactory *> mViewFactories;

    public:
        explicit Page(const QString &pageName,
                      CSMSettings::DeclarationModel &declarationModel,
                      CSMSettings::DefinitionModel &definitionModel,
                      bool isHoriztonal, QWidget *parent = 0);

        void addView (CSMSettings::DefinitionModel &definitionModel,
                      const CSMSettings::Setting *setting);

        QGroupBox *pageFrame() { return mBox; }

    private:

        void setupPage (bool isHorizontal);
        void setupViews (CSMSettings::DeclarationModel &declarationModel,
                         CSMSettings::DefinitionModel &definitionModel);

        QSortFilterProxyModel *buildFilter (QAbstractItemModel &model,
                                            CSMSettings::SettingProperty column,
                                            const QString &expression);
        void buildFactories();
    };
}
#endif // CSVSETTINGS_PAGE_HPP
