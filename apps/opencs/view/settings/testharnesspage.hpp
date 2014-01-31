#ifndef CSVSETTINGS_TESTHARNESSPAGE_HPP
#define CSVSETTINGS_TESTHARNESSPAGE_HPP

#include "page.hpp"
#include "../../model/settings/settingmodel.hpp"

class QAbstractItemModel;

namespace CSMSettings { class SettingModel; }
namespace CSVSettings
{
    class TestHarnessPage: public Page
    {
    public:
        TestHarnessPage(CSMSettings::SettingModel &model,
                        QWidget *parent = 0);
    };
}
#endif // CSVSETTINGS_TESTHARNESSPAGE_HPP
