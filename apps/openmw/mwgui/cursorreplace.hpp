#ifndef GAME_CURSORREPLACE_H
#define GAME_CURSORREPLACE_H

#include <string>
#include <MyGUI.h>
#include <MyGUI_IPointer.h>

using namespace MyGUI;

namespace MWGui
{
    /// \brief A simple class that allows us to get the members of
    ///        ResourceImageSetPointer that we need. Use with
    ///        MyGUI
    /// \example MyGUI::FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>("Resource", "ResourceImageSetPointer");
    ///          MyGUI::ResourceManager::getInstance().load("core.xml");
    class ResourceImageSetPointerFix :
        public IPointer
    {
        MYGUI_RTTI_DERIVED( ResourceImageSetPointerFix )

    public:
        ResourceImageSetPointerFix();
        virtual ~ResourceImageSetPointerFix();

        virtual void deserialization(xml::ElementPtr _node, Version _version);

        virtual void setImage(ImageBox* _image);
        virtual void setPosition(ImageBox* _image, const IntPoint& _point);

        //and now for the whole point of this class, allow us to get
        //the hot spot, the image and the size of the cursor.
        virtual ResourceImageSetPtr getImageSet();
        virtual IntPoint getHotSpot();
        virtual IntSize getSize();

    private:
        IntPoint mPoint;
        IntSize mSize;
        ResourceImageSetPtr mImageSet;
    };

    /// \brief MyGUI does not support rotating cursors, so we have to do it manually
    class CursorReplace
    {
    public:
        CursorReplace();
    };
}

#endif
