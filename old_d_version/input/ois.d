/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (ois.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module input.ois;

// Mouse buttons
enum MB : int
  {
    Button0     = 0,
    Left        = Button0,
    Button1     = 1,
    Right       = Button1,
    Button2     = 2,
    Middle      = Button2,

    Button3     = 3,
    Button4     = 4,
    Button5     = 5,
    Button6     = 6,
    Button7     = 7,

    LastMouse
  }

	// Keyboard scan codes
enum KC : int
  {
    UNASSIGNED  = 0x00,
    ESCAPE      = 0x01,
    N1          = 0x02,
    N2          = 0x03,
    N3          = 0x04,
    N4          = 0x05,
    N5          = 0x06,
    N6          = 0x07,
    N7          = 0x08,
    N8          = 0x09,
    N9          = 0x0A,
    N0          = 0x0B,
    MINUS       = 0x0C,    // - on main keyboard
    EQUALS      = 0x0D,
    BACK        = 0x0E,    // backspace
    TAB         = 0x0F,
    Q           = 0x10,
    W           = 0x11,
    E           = 0x12,
    R           = 0x13,
    T           = 0x14,
    Y           = 0x15,
    U           = 0x16,
    I           = 0x17,
    O           = 0x18,
    P           = 0x19,
    LBRACKET    = 0x1A,
    RBRACKET    = 0x1B,
    RETURN      = 0x1C,    // Enter on main keyboard
    LCONTROL    = 0x1D,
    A           = 0x1E,
    S           = 0x1F,
    D           = 0x20,
    F           = 0x21,
    G           = 0x22,
    H           = 0x23,
    J           = 0x24,
    K           = 0x25,
    L           = 0x26,
    SEMICOLON   = 0x27,
    APOSTROPHE  = 0x28,
    GRAVE       = 0x29,    // accent
    LSHIFT      = 0x2A,
    BACKSLASH   = 0x2B,
    Z           = 0x2C,
    X           = 0x2D,
    C           = 0x2E,
    V           = 0x2F,
    B           = 0x30,
    N           = 0x31,
    M           = 0x32,
    COMMA       = 0x33,
    PERIOD      = 0x34,    // . on main keyboard
    SLASH       = 0x35,    // / on main keyboard
    RSHIFT      = 0x36,
    MULTIPLY    = 0x37,    // * on numeric keypad
    LMENU       = 0x38,    // left Alt
    SPACE       = 0x39,
    CAPITAL     = 0x3A,
    F1          = 0x3B,
    F2          = 0x3C,
    F3          = 0x3D,
    F4          = 0x3E,
    F5          = 0x3F,
    F6          = 0x40,
    F7          = 0x41,
    F8          = 0x42,
    F9          = 0x43,
    F10         = 0x44,
    NUMLOCK     = 0x45,
    SCROLL      = 0x46,    // Scroll Lock
    NUMPAD7     = 0x47,
    NUMPAD8     = 0x48,
    NUMPAD9     = 0x49,
    SUBTRACT    = 0x4A,    // - on numeric keypad
    NUMPAD4     = 0x4B,
    NUMPAD5     = 0x4C,
    NUMPAD6     = 0x4D,
    ADD         = 0x4E,    // + on numeric keypad
    NUMPAD1     = 0x4F,
    NUMPAD2     = 0x50,
    NUMPAD3     = 0x51,
    NUMPAD0     = 0x52,
    DECIMAL     = 0x53,    // . on numeric keypad
    OEM_102     = 0x56,    // < > | on UK/Germany keyboards
    F11         = 0x57,
    F12         = 0x58,
    F13         = 0x64,    //                     (NEC PC98)
    F14         = 0x65,    //                     (NEC PC98)
    F15         = 0x66,    //                     (NEC PC98)
    KANA        = 0x70,    // (Japanese keyboard)
    ABNT_C1     = 0x73,    // / ? on Portugese (Brazilian) keyboards
    CONVERT     = 0x79,    // (Japanese keyboard)
    NOCONVERT   = 0x7B,    // (Japanese keyboard)
    YEN         = 0x7D,    // (Japanese keyboard)
    ABNT_C2     = 0x7E,    // Numpad . on Portugese (Brazilian) keyboards
    NUMPADEQUALS= 0x8D,    // = on numeric keypad (NEC PC98)
    PREVTRACK   = 0x90,    // Previous Track (CIRCUMFLEX on Japanese keyboard)
    AT          = 0x91,    //                     (NEC PC98)
    COLON       = 0x92,    //                     (NEC PC98)
    UNDERLINE   = 0x93,    //                     (NEC PC98)
    KANJI       = 0x94,    // (Japanese keyboard)
    STOP        = 0x95,    //                     (NEC PC98)
    AX          = 0x96,    //                     (Japan AX)
    UNLABELED   = 0x97,    //                        (J3100)
    NEXTTRACK   = 0x99,    // Next Track
    NUMPADENTER = 0x9C,    // Enter on numeric keypad
    RCONTROL    = 0x9D,
    MUTE        = 0xA0,    // Mute
    CALCULATOR  = 0xA1,    // Calculator
    PLAYPAUSE   = 0xA2,    // Play / Pause
    MEDIASTOP   = 0xA4,    // Media Stop
    VOLUMEDOWN  = 0xAE,    // Volume -
    VOLUMEUP    = 0xB0,    // Volume +
    WEBHOME     = 0xB2,    // Web home
    NUMPADCOMMA = 0xB3,    // , on numeric keypad (NEC PC98)
    DIVIDE      = 0xB5,    // / on numeric keypad
    SYSRQ       = 0xB7,    // Also called print screen
    RMENU       = 0xB8,    // right Alt
    PAUSE       = 0xC5,    // Pause
    HOME        = 0xC7,    // Home on arrow keypad
    UP          = 0xC8,    // UpArrow on arrow keypad
    PGUP        = 0xC9,    // PgUp on arrow keypad
    LEFT        = 0xCB,    // LeftArrow on arrow keypad
    RIGHT       = 0xCD,    // RightArrow on arrow keypad
    END         = 0xCF,    // End on arrow keypad
    DOWN        = 0xD0,    // DownArrow on arrow keypad
    PGDOWN      = 0xD1,    // PgDn on arrow keypad
    INSERT      = 0xD2,    // Insert on arrow keypad
    DELETE      = 0xD3,    // Delete on arrow keypad
    LWIN        = 0xDB,    // Left Windows key
    RWIN        = 0xDC,    // Right Windows key
    APPS        = 0xDD,    // AppMenu key
    POWER       = 0xDE,    // System Power
    SLEEP       = 0xDF,    // System Sleep
    WAKE        = 0xE3,    // System Wake
    WEBSEARCH   = 0xE5,    // Web Search
    WEBFAVORITES= 0xE6,    // Web Favorites
    WEBREFRESH  = 0xE7,    // Web Refresh
    WEBSTOP     = 0xE8,    // Web Stop
    WEBFORWARD  = 0xE9,    // Web Forward
    WEBBACK     = 0xEA,    // Web Back
    MYCOMPUTER  = 0xEB,    // My Computer
    MAIL        = 0xEC,    // Mail
    MEDIASELECT = 0xED,    // Media Select

    CharOnly    = 0xFF,    // Set when the keysym is 0 but the
                           // character is set. This happens with many
                           // international characters or reassigned
                           // characters.

    Mouse0      = 0x100,   // Mouse button events can be handled as
    Mouse1      = 0x101,   // keypresses too.
    Mouse2      = 0x102,
    Mouse3      = 0x103,
    Mouse4      = 0x104,
    Mouse5      = 0x105,
    Mouse6      = 0x106,
    Mouse7      = 0x107,
  }

// Sigh. I guess we have to do this for Monster at some poing anyway,
// so this work isn't completely wasted. Later we can make a generic
// conversion between OIS-keysyms, SDL-keysyms and others to the
// Monster keysyms. It sucks that everybody tries to reinvent the
// wheel as often as they can, but that's the way it goes.

const char[][] keysymToString =
[
    KC.UNASSIGNED  : "UNASSIGNED",
    KC.ESCAPE      : "escape",
    KC.N1          : "1",
    KC.N2          : "2",
    KC.N3          : "3",
    KC.N4          : "4",
    KC.N5          : "5",
    KC.N6          : "6",
    KC.N7          : "7",
    KC.N8          : "8",
    KC.N9          : "9",
    KC.N0          : "0",
    KC.MINUS       : "minus",
    KC.EQUALS      : "equals",
    KC.BACK        : "backspace",
    KC.TAB         : "tab",
    KC.Q           : "q",
    KC.W           : "w",
    KC.E           : "e",
    KC.R           : "r",
    KC.T           : "t",
    KC.Y           : "y",
    KC.U           : "u",
    KC.I           : "i",
    KC.O           : "o",
    KC.P           : "p",
    KC.LBRACKET    : "{",
    KC.RBRACKET    : "}",
    KC.RETURN      : "enter",
    KC.LCONTROL    : "left_ctrl",
    KC.A           : "a",
    KC.S           : "s",
    KC.D           : "d",
    KC.F           : "f",
    KC.G           : "g",
    KC.H           : "h",
    KC.J           : "j",
    KC.K           : "k",
    KC.L           : "l",
    KC.SEMICOLON   : "semicolon",
    KC.APOSTROPHE  : "apostrophe",
    KC.GRAVE       : "grave",
    KC.LSHIFT      : "left_shift",
    KC.BACKSLASH   : "backslash",
    KC.Z           : "z",
    KC.X           : "x",
    KC.C           : "c",
    KC.V           : "v",
    KC.B           : "b",
    KC.N           : "n",
    KC.M           : "m",
    KC.COMMA       : "comma",
    KC.PERIOD      : "period",
    KC.SLASH       : "slash",
    KC.RSHIFT      : "right_shift",
    KC.MULTIPLY    : "numpad_mult",
    KC.LMENU       : "left_alt",
    KC.SPACE       : "space",
    KC.CAPITAL     : "capital",
    KC.F1          : "f1",
    KC.F2          : "f2",
    KC.F3          : "f3",
    KC.F4          : "f4",
    KC.F5          : "f5",
    KC.F6          : "f6",
    KC.F7          : "f7",
    KC.F8          : "f8",
    KC.F9          : "f9",
    KC.F10         : "f10",
    KC.NUMLOCK     : "numlock",
    KC.SCROLL      : "scroll",
    KC.NUMPAD7     : "numpad_7",
    KC.NUMPAD8     : "numpad_8",
    KC.NUMPAD9     : "numpad_9",
    KC.SUBTRACT    : "numpad_minus",
    KC.NUMPAD4     : "numpad_4",
    KC.NUMPAD5     : "numpad_5",
    KC.NUMPAD6     : "numpad_6",
    KC.ADD         : "numpad_plus",
    KC.NUMPAD1     : "numpad_1",
    KC.NUMPAD2     : "numpad_2",
    KC.NUMPAD3     : "numpad_3",
    KC.NUMPAD0     : "numpad_0",
    KC.DECIMAL     : "numpad_period",
    KC.OEM_102     : "oem102",
    KC.F11         : "f11",
    KC.F12         : "f12",
    KC.F13         : "f13",
    KC.F14         : "f14",
    KC.F15         : "f15",
    KC.KANA        : "kana",
    KC.ABNT_C1     : "abnt_c1",
    KC.CONVERT     : "convert",
    KC.NOCONVERT   : "noconvert",
    KC.YEN         : "yen",
    KC.ABNT_C2     : "abnt_c2",
    KC.NUMPADEQUALS: "numpad_equals",
    KC.PREVTRACK   : "prev_track",
    KC.AT          : "at",
    KC.COLON       : "colon",
    KC.UNDERLINE   : "underline",
    KC.KANJI       : "kanji",
    KC.STOP        : "stop",
    KC.AX          : "ax",
    KC.UNLABELED   : "unlabeled",
    KC.NEXTTRACK   : "next_track",
    KC.NUMPADENTER : "numpad_enter",
    KC.RCONTROL    : "right_control",
    KC.MUTE        : "mute",
    KC.CALCULATOR  : "calculator",
    KC.PLAYPAUSE   : "play_pause",
    KC.MEDIASTOP   : "media_stop",
    KC.VOLUMEDOWN  : "volume_down",
    KC.VOLUMEUP    : "volume_up",
    KC.WEBHOME     : "webhome",
    KC.NUMPADCOMMA : "numpad_comma",
    KC.DIVIDE      : "numpad_divide",
    KC.SYSRQ       : "print_screen",
    KC.RMENU       : "right_alt",
    KC.PAUSE       : "pause",
    KC.HOME        : "home",
    KC.UP          : "up",
    KC.PGUP        : "page_up",
    KC.LEFT        : "left",
    KC.RIGHT       : "right",
    KC.END         : "end",
    KC.DOWN        : "down",
    KC.PGDOWN      : "page_down",
    KC.INSERT      : "insert",
    KC.DELETE      : "delete",
    KC.LWIN        : "left_win",
    KC.RWIN        : "right_win",
    KC.APPS        : "app_menu",
    KC.POWER       : "power",
    KC.SLEEP       : "sleep",
    KC.WAKE        : "wake",
    KC.WEBSEARCH   : "web_search",
    KC.WEBFAVORITES: "web_favorites",
    KC.WEBREFRESH  : "web_refresh",
    KC.WEBSTOP     : "web_stop",
    KC.WEBFORWARD  : "web_forward",
    KC.WEBBACK     : "web_back",
    KC.MYCOMPUTER  : "my_computer",
    KC.MAIL        : "mail",
    KC.MEDIASELECT : "media_select",


    KC.CharOnly    : "CHAR_ONLY",  // Set when the keysym is 0 but the
                                   // character is set. This happens
                                   // with many international
                                   // characters or reassigned
                                   // characters in OIS (and it
                                   // SUCKS.)

    KC.Mouse0      : "mouse0",
    KC.Mouse1      : "mouse1",
    KC.Mouse2      : "mouse2",
    KC.Mouse3      : "mouse3",
    KC.Mouse4      : "mouse4",
    KC.Mouse5      : "mouse5",
    KC.Mouse6      : "mouse6",
    KC.Mouse7      : "mouse7",
 ];

enum ComponentType : int
  {
    Unknown = 0,
    Button  = 1, // ie. Key, mouse button, joy button, etc
    Axis    = 2, // ie. A joystick or mouse axis
    Slider  = 3, //
    POV     = 4, // ie. Arrow direction keys
    Vector3 = 5  // ie. WiiMote orientation
  }

align(4) struct Axis
{
  ComponentType type;
  int abs, rel;
  bool absOnly;
}

// The C++ size of Axis is 16
static assert(Axis.sizeof == 16);

struct MouseState
{
  /* Represents the height/width of your display area.. used if mouse
     clipping or mouse grabbed in case of X11 - defaults to 50.. Make
     sure to set this and change when your size changes.. */
  int width, height;

  // X Axis component
  Axis X;

  // Y Axis Component
  Axis Y;

  // Z Axis Component
  Axis Z;

  // represents all buttons - bit position indicates button down
  int buttons;

  // Button down test
  bool buttonDown( MB button )
  {
    return (buttons & ( 1 << button )) != 0;
  }
}

// Check that we match the C++ size
static assert(MouseState.sizeof == 60);
