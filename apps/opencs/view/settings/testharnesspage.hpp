#ifndef CSVSETTINGS_TESTHARNESSPAGE_HPP
#define CSVSETTINGS_TESTHARNESSPAGE_HPP

#include "page.hpp"

class QAbstractItemModel;

namespace CSVSettings
{
    class TestHarnessPage: public Page
    {
    public:
        TestHarnessPage(CSMSettings::DeclarationModel &declarationModel,
                        CSMSettings::DefinitionModel &definitionModel,
                        QWidget *parent = 0);
    };
}
#endif // CSVSETTINGS_TESTHARNESSPAGE_HPP
