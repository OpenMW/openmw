#ifndef BLANKPAGE_HPP
#define BLANKPAGE_HPP

#include "abstractpage.hpp"

class QGroupBox;

namespace CSVSettings {

    class UserSettings;
    class AbstractBlock;

    /// Derived page with no widgets
    /// Reference use only.
    class BlankPage : public AbstractPage
    {
        QLayout *mLayout;

    public:

        BlankPage (QWidget *parent = 0);
        BlankPage (const QString &title, QWidget *parent);

        void setupUi();

        //void initializeWidgets (const CSMSettings::SettingMap &settings);
        QLayout *getGroupBoxLayout() const { return mLayout; }
    };
}

#endif // BLANKPAGE_HPP
