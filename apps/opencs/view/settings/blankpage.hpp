#ifndef BLANKPAGE_HPP
#define BLANKPAGE_HPP

#include "abstractpage.hpp"

class QGroupBox;

namespace CsSettings {

    class UserSettings;
    class AbstractBlock;

    class BlankPage : public AbstractPage
    {
        Q_OBJECT

    public:

        BlankPage (QWidget *parent = 0);
        BlankPage (const QString &title, QWidget *parent);

        void setupUi();
        void initializeWidgets (const SettingMap &settings);

    private:
        void initPage();
    };
}

#endif // BLANKPAGE_HPP
