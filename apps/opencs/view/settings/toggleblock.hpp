#ifndef TOGGLEBLOCK_HPP
#define TOGGLEBLOCK_HPP

#include <QObject>

#include "customblock.hpp"

namespace CSVSettings
{
    class GroupBlock;
    class GroupBox;
    class ToggleWidget;
    class ItemBlock;

    class ToggleBlock : public CustomBlock
    {

    public:
        explicit ToggleBlock(QWidget *parent = 0);

        int build (CustomBlockDef &def);

    private:
        GroupBox *buildToggleWidgets (GroupBlockDef &def, QString &defaultToggle);
    };
}
#endif // TOGGLEBLOCK_HPP
