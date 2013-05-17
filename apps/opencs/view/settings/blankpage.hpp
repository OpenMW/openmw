#ifndef BLANKPAGE_HPP
#define BLANKPAGE_HPP

#include "abstractpage.hpp"

class QGroupBox;

namespace CSVSettings {

    class UserSettings;
    class AbstractBlock;

    class BlankPage : public AbstractPage
    {

    public:

        BlankPage (QWidget *parent = 0);
        BlankPage (const QString &title, QWidget *parent);

        void setupUi();
        void initializeWidgets (const CSMSettings::SettingMap &settings);

    private:
        void initPage();
    };
}

#endif // BLANKPAGE_HPP
