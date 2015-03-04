#ifndef MWGUI_CONTROLLERS_H
#define MWGUI_CONTROLLERS_H

#include <string>
#include <MyGUI_ControllerItem.h>

namespace MyGUI
{
    class Widget;
}

namespace MWGui
{
    namespace Controllers
    {
        // Should be removed when upgrading to MyGUI 3.2.2 (current git), it has ControllerRepeatClick
        class ControllerRepeatEvent :
            public MyGUI::ControllerItem
        {
            MYGUI_RTTI_DERIVED( ControllerRepeatEvent )

        public:
            ControllerRepeatEvent();
            virtual ~ControllerRepeatEvent();

            void setRepeat(float init, float step);
            void setEnabled(bool enable);
            virtual void setProperty(const std::string& _key, const std::string& _value);

            // Events
            typedef MyGUI::delegates::CMultiDelegate2<MyGUI::Widget*, MyGUI::ControllerItem*> EventHandle_RepeatClickVoid;

            /** Event : Repeat Click.\n
            signature : void method(MyGUI::Widget* _sender, MyGUI::ControllerItem *_controller)\n
            */
            EventHandle_RepeatClickVoid eventRepeatClick;

        private:
            bool addTime(MyGUI::Widget* _widget, float _time);
            void prepareItem(MyGUI::Widget* _widget);

        private:
            float mInit;
            float mStep;
            bool mEnabled;
            float mTimeLeft;
        };

        /// Automatically positions a widget below the mouse cursor.
        class ControllerFollowMouse :
            public MyGUI::ControllerItem
        {
            MYGUI_RTTI_DERIVED( ControllerFollowMouse )

        private:
            bool addTime(MyGUI::Widget* _widget, float _time);
            void prepareItem(MyGUI::Widget* _widget);
        };
    }
}

#endif
