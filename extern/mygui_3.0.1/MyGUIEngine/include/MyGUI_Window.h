/*!
	@file
	@author		Albert Semenov
	@date		11/2007
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __MYGUI_WINDOW_H__
#define __MYGUI_WINDOW_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Widget.h"
#include "MyGUI_EventPair.h"
#include "MyGUI_ControllerFadeAlpha.h"

namespace MyGUI
{

	// OBSOLETE
	typedef delegates::CDelegate2<Widget*, const std::string&> EventHandle_WidgetString;
	typedef delegates::CDelegate2<Window*, const std::string&> EventHandle_WindowPtrCStringRef;
	typedef delegates::CDelegate1<Window*> EventHandle_WindowPtr;

	class MYGUI_EXPORT Window :
		public Widget
	{
		MYGUI_RTTI_DERIVED( Window )

	public:
		Window();

		/** @copydoc Widget::setVisible */
		virtual void setVisible(bool _value);

		/** Hide or show window smooth */
		void setVisibleSmooth(bool _value);
		/** Hide window smooth and then destroy it */
		void destroySmooth();

		/** Enable or disable auto alpha mode */
		void setAutoAlpha(bool _value);
		/** Get auto alpha mode flag */
		bool getAutoAlpha() { return mIsAutoAlpha; }

		/** Set window caption */
		virtual void setCaption(const UString& _value);
		/** Get window caption */
		virtual const UString& getCaption();

		/** Get window caption widget */
		Widget* getCaptionWidget() { return mWidgetCaption; }

		/** Set minimal possible window size */
		void setMinSize(const IntSize& _value);
		/** Set minimal possible window size */
		void setMinSize(int _width, int _height) { setMinSize(IntSize(_width, _height)); }
		/** Get minimal possible window size */
		IntSize getMinSize();

		/** Set maximal possible window size */
		void setMaxSize(const IntSize& _value);
		/** Set maximal possible window size */
		void setMaxSize(int _width, int _height) { setMaxSize(IntSize(_width, _height)); }
		/** Get maximal possible window size */
		IntSize getMaxSize();

		//! @copydoc Widget::setPosition(const IntPoint& _value)
		virtual void setPosition(const IntPoint& _value);
		//! @copydoc Widget::setSize(const IntSize& _value)
		virtual void setSize(const IntSize& _value);
		//! @copydoc Widget::setCoord(const IntCoord& _value)
		virtual void setCoord(const IntCoord& _value);

		/** @copydoc Widget::setPosition(int _left, int _top) */
		void setPosition(int _left, int _top) { setPosition(IntPoint(_left, _top)); }
		/** @copydoc Widget::setSize(int _width, int _height) */
		void setSize(int _width, int _height) { setSize(IntSize(_width, _height)); }
		/** @copydoc Widget::setCoord(int _left, int _top, int _width, int _height) */
		void setCoord(int _left, int _top, int _width, int _height) { setCoord(IntCoord(_left, _top, _width, _height)); }

		/** Get snap to borders mode flag */
		bool getSnap() { return mSnap; }
		/** Enable or disable snap to borders mode */
		void setSnap(bool _value) { mSnap = _value; }

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*event:*/
		/** Event : Window button pressed.\n
			signature : void method(MyGUI::Window* _sender, const std::string& _name)
			@param _sender widget that called this event
			@param _name of pressed button
		*/
		EventPair<EventHandle_WidgetString, EventHandle_WindowPtrCStringRef> eventWindowButtonPressed;

		/** Event : Window coordinate changed (window was resized or moved).\n
			signature : void method(MyGUI::Window* _sender)
			@param _sender widget that called this event
		*/
		EventPair<EventHandle_WidgetVoid, EventHandle_WindowPtr> eventWindowChangeCoord;

	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void Widget::setCoord(const IntCoord& _coord)")
		void setPosition(const IntCoord& _coord) { setCoord(_coord); }
		MYGUI_OBSOLETE("use : void Widget::setCoord(int _left, int _top, int _width, int _height)")
		void setPosition(int _left, int _top, int _width, int _height) { setCoord(_left, _top, _width, _height); }
		MYGUI_OBSOLETE("use : void setVisibleSmooth(bool _visible)")
		void showSmooth(bool _reset = false) { setVisibleSmooth(true); }
		MYGUI_OBSOLETE("use : void setVisibleSmooth(bool _visible)")
		void hideSmooth() { setVisibleSmooth(false); }
		MYGUI_OBSOLETE("use : void setMinSize(const IntSize& _min) , void setMaxSize(const IntSize& _min)")
		void setMinMax(const IntRect& _minmax) { setMinSize(_minmax.left, _minmax.top); setMaxSize(_minmax.right, _minmax.bottom); }
		MYGUI_OBSOLETE("use : void setMinSize(const IntSize& _min) , void setMaxSize(const IntSize& _min)")
		void setMinMax(int _min_w, int _min_h, int _max_w, int _max_h) { setMinSize(_min_w, _min_h); setMaxSize(_max_w, _max_h); }
		MYGUI_OBSOLETE("use : IntSize getMinSize() , IntSize getMaxSize()")
		IntRect getMinMax() { return IntRect(getMinSize().width, getMinSize().height, getMaxSize().width, getMaxSize().height); }

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		virtual ~Window();

		void baseChangeWidgetSkin(ResourceSkin* _info);

		// переопределяем для присвоению клиенту
		virtual Widget* baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name);

		void onMouseChangeRootFocus(bool _focus);
		void onKeyChangeRootFocus(bool _focus);
		void onMouseDrag(int _left, int _top);
		void onMouseButtonPressed(int _left, int _top, MouseButton _id);

		void notifyMousePressed(MyGUI::Widget* _sender, int _left, int _top, MouseButton _id);
		void notifyPressedButtonEvent(MyGUI::Widget* _sender);
		void notifyMouseDrag(MyGUI::Widget* _sender, int _left, int _top);

		// просто обновляет альфу взависимости от флагов
		void updateAlpha();

		void animateStop(Widget* _widget);

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		float getAlphaVisible();
		void getSnappedCoord(IntCoord& _coord);

		ControllerFadeAlpha* createControllerFadeAlpha(float _alpha, float _coef, bool _enable);

	private:
		Widget* mWidgetCaption;

		// размеры окна перед началом его изменений
		IntCoord mPreActionCoord;

		// наши главные фокусы
		bool mMouseRootFocus;
		bool mKeyRootFocus;

		// автоматическое или ручное управление альфой
		bool mIsAutoAlpha;

		// минимальные и максимальные размеры окна
		IntRect mMinmax;

		bool mSnap; // прилеплять ли к краям

		IntCoord mCurrentActionScale;
		bool mAnimateSmooth;

	};

} // namespace MyGUI

#endif // __MYGUI_WINDOW_H__
