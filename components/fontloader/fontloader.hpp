#ifndef MWGUI_FONTLOADER_H
#define MWGUI_FONTLOADER_H

#include <components/to_utf8/to_utf8.hpp>

namespace Gui
{


    /// @brief loads Morrowind's .fnt/.tex fonts for use with MyGUI and Ogre
    class FontLoader
    {
    public:
        FontLoader (ToUTF8::FromType encoding);

        /// @param exportToFile export the converted fonts (Images and XML with glyph metrics) to files?
        void loadAllFonts (bool exportToFile);

    private:
        ToUTF8::FromType mEncoding;

        /// @param exportToFile export the converted font (Image and XML with glyph metrics) to files?
        void loadFont (const std::string& fileName, bool exportToFile);
    };

}

#endif
