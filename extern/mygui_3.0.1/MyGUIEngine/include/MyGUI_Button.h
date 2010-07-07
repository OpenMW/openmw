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
#ifndef __MYGUI_BUTTON_H__
#define __MYGUI_BUTTON_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_StaticText.h"

namespace MyGUI
{

	class MYGUI_EXPORT Button :
		public StaticText
	{
		MYGUI_RTTI_DERIVED( Button )

	public:
		Button();

		//! OLD Set button check state
		void setButtonPressed(bool _value) { setStateCheck(_value); }
		//! OLD Get buton check
		bool getButtonPressed() { return getStateCheck(); }

		//! Set button check state
		void setStateCheck(bool _value);

		//! Get buton check
		bool getStateCheck() { return mIsStateCheck; }

		//! Set image index (image should be defined in skin)
		void setImageIndex(size_t _value);
		//! Get image index
		size_t getImageIndex();

		/** Enable or disable Image mode\n
			Image mode: when button state changed Image on button also change it's picture.\n
			Disabled (false) by default.
		*/
		void setModeImage(bool _value);
		/** Get Image mode flag */
		bool getModeImage() { return mModeImage; }

		/** Get pointer to glyph image for this button (if it exist in button skin) */
		StaticImage* getStaticImage() { return mImage; }

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

		void _setMouseFocus(bool _focus);

	protected:
		virtual ~Button();

		virtual void baseChangeWidgetSkin(ResourceSkin* _info);

		virtual void onMouseLostFocus(Widget* _new);
		virtual void onMouseSetFocus(Widget* _old);
		virtual void onMouseButtonPressed(int _left, int _top, MouseButton _id);
		virtual void onMouseButtonReleased(int _left, int _top, MouseButton _id);

		virtual void baseUpdateEnable();

		bool _setState(const std::string& _value);
		void setImageResource(const std::string& _name);

	private:
		void updateButtonState();

		void shutdownWidgetSkin();
		void initialiseWidgetSkin(ResourceSkin* _info);

	private:
		// нажата ли кнопка
		bool mIsMousePressed;
		// в фокусе ли кнопка
		bool mIsMouseFocus;
		// статус кнопки нажата или нет
		bool mIsStateCheck;

		StaticImage* mImage;
		bool mModeImage;

	};

} // namespace MyGUI

#endif // __MYGUI_BUTTON_H__
