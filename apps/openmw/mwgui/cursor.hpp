#ifndef MWGUI_CURSOR_H
#define MWGUI_CURSOR_H

#include <MyGUI_IPointer.h>
#include <MyGUI_ResourceImageSet.h>
#include <MyGUI_RTTI.h>

namespace MWGui
{

    /// \brief Allows us to get the members of
    ///        ResourceImageSetPointer that we need.
    /// \example MyGUI::FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>("Resource", "ResourceImageSetPointer");
    ///          MyGUI::ResourceManager::getInstance().load("core.xml");
    class ResourceImageSetPointerFix :
        public MyGUI::IPointer
    {
        MYGUI_RTTI_DERIVED( ResourceImageSetPointerFix )

    public:
        ResourceImageSetPointerFix();
        virtual ~ResourceImageSetPointerFix();

        virtual void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version);

        virtual void setImage(MyGUI::ImageBox* _image);
        virtual void setPosition(MyGUI::ImageBox* _image, const MyGUI::IntPoint& _point);

        //and now for the whole point of this class, allow us to get
        //the hot spot, the image and the size of the cursor.
        virtual MyGUI::ResourceImageSetPtr getImageSet();
        virtual MyGUI::IntPoint getHotSpot();
        virtual MyGUI::IntSize getSize();
        virtual int getRotation();

    private:
        MyGUI::IntPoint mPoint;
        MyGUI::IntSize mSize;
        MyGUI::ResourceImageSetPtr mImageSet;
        int mRotation; // rotation in degrees
    };

    class Cursor
    {
    public:
        Cursor();
        ~Cursor();
        void update ();

        void setVisible (bool visible);

        void onCursorChange (const std::string& name);

    private:
        MyGUI::ImageBox* mWidget;

        MyGUI::IntSize mSize;
        MyGUI::IntPoint mHotSpot;
    };
}

#endif
