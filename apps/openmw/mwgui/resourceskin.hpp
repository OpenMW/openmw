#ifndef MWGUI_RESOURCESKIN_H
#define MWGUI_RESOURCESKIN_H

#include <string>

#include <MyGUI_RTTI.h>
#include <MyGUI_ResourceSkin.h>
#include <MyGUI_XmlDocument.h>

namespace MyGUI
{
    class Version;
}

namespace MWGui
{
    class AutoSizedResourceSkin final : public MyGUI::ResourceSkin
    {
        MYGUI_RTTI_DERIVED(AutoSizedResourceSkin)

    public:
        void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version) override;
    };

}

#endif
