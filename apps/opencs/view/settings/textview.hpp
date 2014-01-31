#ifndef CSVSETTINGS_TEXTVIEW_HPP
#define CSVSETTINGS_TEXTVIEW_HPP

#include "view.hpp"
#include "../../model/settings/setting.hpp"

class QLineEdit;

namespace CSVSettings
{
    class TextView : public View
    {

        QWidget *mTextWidget;

    public:
        explicit TextView (QAbstractItemModel *textAdapter,
                            const CSMSettings::Setting *setting,
                            QWidget *parent = 0);

    protected:

        void buildView (const CSMSettings::Setting *setting);
        void buildModel (const CSMSettings::Setting *setting);
    };

    class TextViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit TextViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        TextView *createView (CSMSettings::DefinitionModel &model,
                               const CSMSettings::Setting *setting);
    };
}
#endif // CSVSETTINGS_TEXTVIEW_HPP
