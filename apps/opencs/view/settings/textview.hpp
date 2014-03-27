#ifndef CSVSETTINGS_TEXTVIEW_HPP
#define CSVSETTINGS_TEXTVIEW_HPP

#include "view.hpp"
#include "../../model/settings/setting.hpp"

namespace CSVSettings
{
    class TextView : public View
    {
        Q_OBJECT

        QWidget *mTextWidget;

    public:
        explicit TextView (CSMSettings::Setting *setting,
                           Page *parent = 0);

        void slotUpdateView (QStringList list);

    protected slots:

        void slotTextEdited (QString value);
    };

    class TextViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit TextViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        TextView *createView (CSMSettings::Setting *setting,
                              Page *parent);
    };
}
#endif // CSVSETTINGS_TEXTVIEW_HPP
