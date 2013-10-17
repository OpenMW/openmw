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

        int build (CustomBlockDef *def);

    private:
        /// Constructor for toggle widgets that are specific to toggle block
        /// Widgets are not a part of the user preference settings
        GroupBox *buildToggleWidgets (GroupBlockDef *def, QString &defaultToggle);
    };
}
#endif // TOGGLEBLOCK_HPP
