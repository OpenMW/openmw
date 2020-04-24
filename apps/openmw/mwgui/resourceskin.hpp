#ifndef MWGUI_RESOURCESKIN_H
#define MWGUI_RESOURCESKIN_H

#include <MyGUI_ResourceSkin.h>

namespace MWGui
{
    class AutoSizedResourceSkin final : public MyGUI::ResourceSkin
    {
        MYGUI_RTTI_DERIVED( AutoSizedResourceSkin )

    public:
        void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version) final;
    };

}

#endif
