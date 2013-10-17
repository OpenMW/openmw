#ifndef MWGUI_FONTLOADER_H
#define MWGUI_FONTLOADER_H

#include <components/to_utf8/to_utf8.hpp>

namespace MWGui
{


    /// @brief loads Morrowind's .fnt/.tex fonts for use with MyGUI and Ogre
    class FontLoader
    {
    public:
        FontLoader (ToUTF8::FromType encoding);
        void loadAllFonts ();

    private:
        ToUTF8::FromType mEncoding;

        void loadFont (const std::string& fileName);
    };

}

#endif
