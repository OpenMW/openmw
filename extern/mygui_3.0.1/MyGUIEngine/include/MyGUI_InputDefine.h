/*!
	@file
	@author		Albert Semenov
	@date		03/2008
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
#ifndef __MYGUI_INPUT_DEFINE_H__
#define __MYGUI_INPUT_DEFINE_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	struct MYGUI_EXPORT MouseButton
	{
		enum Enum
		{
			Left = 0,
			Right,
			Middle,

			Button0 = 0,
			Button1,
			Button2,
			Button3,
			Button4,
			Button5,
			Button6,
			Button7,

			None = -1
		};

		MouseButton(Enum _value = None) : value(_value) { }

		friend bool operator == (MouseButton const& a, MouseButton const& b) { return a.value == b.value; }
		friend bool operator != (MouseButton const& a, MouseButton const& b) { return a.value != b.value; }

		int toValue() const { return (int)value; }

	private:
		Enum value;
	};

	struct MYGUI_EXPORT KeyCode
	{
		enum Enum
		{
			None          = 0x00,
			Escape          = 0x01,
			One             = 0x02,
			Two             = 0x03,
			Three           = 0x04,
			Four            = 0x05,
			Five            = 0x06,
			Six             = 0x07,
			Seven           = 0x08,
			Eight           = 0x09,
			Nine            = 0x0A,
			Zero            = 0x0B,
			Minus           = 0x0C,    /* - on main keyboard */
			Equals			= 0x0D,
			Backspace		= 0x0E,    /* Backspace */
			Tab				= 0x0F,
			Q               = 0x10,
			W               = 0x11,
			E               = 0x12,
			R               = 0x13,
			T               = 0x14,
			Y               = 0x15,
			U               = 0x16,
			I               = 0x17,
			O               = 0x18,
			P               = 0x19,
			LeftBracket     = 0x1A,
			RightBracket    = 0x1B,
			Return			= 0x1C,    /* Enter on main keyboard */
			LeftControl		= 0x1D,
			A               = 0x1E,
			S               = 0x1F,
			D               = 0x20,
			F               = 0x21,
			G               = 0x22,
			H               = 0x23,
			J               = 0x24,
			K               = 0x25,
			L               = 0x26,
			Semicolon       = 0x27,
			Apostrophe		= 0x28,
			Grave           = 0x29,    /* Accent grave ( ~ ) */
			LeftShift       = 0x2A,
			Backslash       = 0x2B,
			Z               = 0x2C,
			X               = 0x2D,
			C               = 0x2E,
			V               = 0x2F,
			B               = 0x30,
			N               = 0x31,
			M               = 0x32,
			Comma           = 0x33,
			Period          = 0x34,    /* . on main keyboard */
			Slash           = 0x35,    /* '/' on main keyboard */
			RightShift      = 0x36,
			Multiply        = 0x37,    /* * on numeric keypad */
			LeftAlt        = 0x38,    /* Left Alt */
			Space           = 0x39,
			Capital         = 0x3A,
			F1              = 0x3B,
			F2              = 0x3C,
			F3              = 0x3D,
			F4              = 0x3E,
			F5              = 0x3F,
			F6              = 0x40,
			F7              = 0x41,
			F8              = 0x42,
			F9              = 0x43,
			F10             = 0x44,
			NumLock         = 0x45,
			ScrollLock      = 0x46,    /* Scroll Lock */
			Numpad7         = 0x47,
			Numpad8         = 0x48,
			Numpad9         = 0x49,
			Subtract        = 0x4A,    /* - on numeric keypad */
			Numpad4         = 0x4B,
			Numpad5         = 0x4C,
			Numpad6         = 0x4D,
			Add				= 0x4E,    /* + on numeric keypad */
			Numpad1         = 0x4F,
			Numpad2         = 0x50,
			Numpad3         = 0x51,
			Numpad0         = 0x52,
			Decimal			= 0x53,    /* . on numeric keypad */
			OEM_102         = 0x56,    /* < > | on UK/Germany keyboards */
			F11             = 0x57,
			F12             = 0x58,
			F13             = 0x64,    /*                     (NEC PC98) */
			F14             = 0x65,    /*                     (NEC PC98) */
			F15             = 0x66,    /*                     (NEC PC98) */
			Kana            = 0x70,    /* (Japanese keyboard)            */
			ABNT_C1         = 0x73,    /* / ? on Portugese (Brazilian) keyboards */
			Convert         = 0x79,    /* (Japanese keyboard)            */
			NoConvert       = 0x7B,    /* (Japanese keyboard)            */
			Yen             = 0x7D,    /* (Japanese keyboard)            */
			ABNT_C2         = 0x7E,    /* Numpad . on Portugese (Brazilian) keyboards */
			NumpadEquals    = 0x8D,    /* = on numeric keypad (NEC PC98) */
			PrevTrack       = 0x90,    /* Previous Track (KC_CIRCUMFLEX on Japanese keyboard) */
			At              = 0x91,    /*                     (NEC PC98) */
			Colon           = 0x92,    /*                     (NEC PC98) */
			Underline       = 0x93,    /*                     (NEC PC98) */
			Kanji           = 0x94,    /* (Japanese keyboard)            */
			Stop            = 0x95,    /*                     (NEC PC98) */
			AX              = 0x96,    /*                     (Japan AX) */
			Unlabeled       = 0x97,    /*                        (J3100) */
			NextTrack       = 0x99,    /* Next Track */
			NumpadEnter     = 0x9C,    /* Enter on numeric keypad */
			RightControl    = 0x9D,
			Mute            = 0xA0,
			Calculator      = 0xA1,
			PlayPause       = 0xA2,    /* Play / Pause */
			MediaStop       = 0xA4,    /* Media Stop */
			VolumeDown      = 0xAE,    /* Volume - */
			VolumeUp        = 0xB0,    /* Volume + */
			WebHome         = 0xB2,    /* Web home */
			NumpadComma     = 0xB3,    /* , on numeric keypad (NEC PC98) */
			Divide          = 0xB5,    /* / on numeric keypad */
			SysRq           = 0xB7,
			RightAlt        = 0xB8,    /* Right Alt */
			Pause           = 0xC5,
			Home            = 0xC7,    /* Home on arrow keypad */
			ArrowUp         = 0xC8,    /* UpArrow on arrow keypad */
			PageUp          = 0xC9,    /* PgUp on arrow keypad */
			ArrowLeft       = 0xCB,    /* LeftArrow on arrow keypad */
			ArrowRight      = 0xCD,    /* RightArrow on arrow keypad */
			End             = 0xCF,    /* End on arrow keypad */
			ArrowDown       = 0xD0,    /* DownArrow on arrow keypad */
			PageDown		= 0xD1,    /* PgDn on arrow keypad */
			Insert          = 0xD2,    /* Insert on arrow keypad */
			Delete          = 0xD3,    /* Delete on arrow keypad */
			LeftWindows     = 0xDB,    /* Left Windows key */
			RightWindow     = 0xDC,    /* Right Windows key */
			RightWindows    = 0xDC,    /* Right Windows key - Correct spelling :) */
			AppMenu         = 0xDD,    /* AppMenu key */
			Power           = 0xDE,    /* System Power */
			Sleep           = 0xDF,    /* System Sleep */
			Wake			= 0xE3,    /* System Wake */
			WebSearch		= 0xE5,
			WebFavorites	= 0xE6,
			WebRefresh		= 0xE7,
			WebStop			= 0xE8,
			WebForward		= 0xE9,
			WebBack			= 0xEA,
			MyComputer		= 0xEB,
			Mail			= 0xEC,
			MediaSelect		= 0xED
		};

		KeyCode(Enum _value = None) : value(_value) { }

		friend bool operator == (KeyCode const& a, KeyCode const& b) { return a.value == b.value; }
		friend bool operator != (KeyCode const& a, KeyCode const& b) { return a.value != b.value; }

		int toValue() const { return (int)value; }

	private:
		Enum value;
	};

} // namespace MyGUI

#endif // __MYGUI_INPUT_DEFINE_H__
