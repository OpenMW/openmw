#ifndef MWGUI_CURSOR_H
#define MWGUI_CURSOR_H

#include <MyGUI_IPointer.h>
#include <MyGUI_ResourceImageSet.h>

namespace MWGui
{

    /// \brief Allows us to get the members of
    ///        ResourceImageSetPointer that we need.
    /// \example MyGUI::FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>("Resource", "ResourceImageSetPointer");
    ///          MyGUI::ResourceManager::getInstance().load("core.xml");
    class ResourceImageSetPointerFix final :
        public MyGUI::IPointer
    {
        MYGUI_RTTI_DERIVED( ResourceImageSetPointerFix )

    public:
        ResourceImageSetPointerFix();
        virtual ~ResourceImageSetPointerFix();

        void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version) final;

        void setImage(MyGUI::ImageBox* _image) final;
        void setPosition(MyGUI::ImageBox* _image, const MyGUI::IntPoint& _point) final;

        //and now for the whole point of this class, allow us to get
        //the hot spot, the image and the size of the cursor.
        MyGUI::ResourceImageSetPtr getImageSet();
        MyGUI::IntPoint getHotSpot();
        MyGUI::IntSize getSize();
        int getRotation();

    private:
        MyGUI::IntPoint mPoint;
        MyGUI::IntSize mSize;
        MyGUI::ResourceImageSetPtr mImageSet;
        int mRotation; // rotation in degrees
    };

}

#endif
