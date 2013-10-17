#ifndef MWGUI_CONTROLLERS_H
#define MWGUI_CONTROLLERS_H

#include <MyGUI_Widget.h>
#include <MyGUI_ControllerItem.h>


namespace MWGui
{
    namespace Controllers
    {
        class ControllerRepeatClick :
            public MyGUI::ControllerItem
        {
            MYGUI_RTTI_DERIVED( ControllerRepeatClick )

        public:
            ControllerRepeatClick();
            virtual ~ControllerRepeatClick();

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
    }
}

#endif
