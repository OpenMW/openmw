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

        QString mDelimiter;

    public:
        explicit TextView (CSMSettings::Setting *setting,
                           Page *parent = 0);

    protected:

        /// virtual function called through View
        void updateView (bool signalUpdate = true) const;

    protected slots:

        ///Receives updates to the widget for signalling
        void slotTextEdited (QString value);

    private:

        ///Comparison function that returns true if the trimmed() strings
        ///are equal
        bool isEquivalent (const QString &lhs, const QString &rhs) const;
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
