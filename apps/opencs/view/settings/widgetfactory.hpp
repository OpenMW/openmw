#ifndef CSVSETTINGS_WIDGETFACTORY_HPP
#define CSVSETTINGS_WIDGETFACTORY_HPP

#include "support.hpp"

class QWidget;
class QSortFilterProxyModel;
class QDataWidgetMapper;
class QLayout;
class QString;

namespace CSVSettings {

    class WidgetFactory
    {

        QWidget *mParent;
        QSortFilterProxyModel *mSourceModel;

    public:

        explicit WidgetFactory (QSortFilterProxyModel *model, QWidget *parent)
            : mParent (parent), mSourceModel (model)
        {}

        template <typename T>
        QLayout *createWidget (const QString &name, Orientation = Orient_Horizontal)
        {
            //type-check the template parameter to ensure it is of type QWidget
            //compile-time fail if not
            (void)static_cast<QWidget *>((T*)0);

            T *widget  = new T (mParent);

            return build(widget, name, orientation);
        }

    private:

        QLayout *build                      (QWidget *widget, const QString &name, Orientation orientation);
        void buildMapper                    (QWidget *widget, QSortFilterProxyModel *filter);
        QSortFilterProxyModel *buildModel   (QWidget *widget);
        QLayout *buildLayout                (QWidget *widget, Orientation orientation);

        // need support functions for:
        //
        // setInputMask
        // alignment
    };
}
#endif // CSVSETTINGS_WIDGETFACTORY_HPP
