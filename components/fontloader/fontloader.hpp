#ifndef OPENMW_COMPONENTS_FONTLOADER_H
#define OPENMW_COMPONENTS_FONTLOADER_H

#include <components/to_utf8/to_utf8.hpp>

namespace VFS
{
    class Manager;
}

namespace MyGUI
{
    class ITexture;
    class ResourceManualFont;
}

namespace Gui
{


    /// @brief loads Morrowind's .fnt/.tex fonts for use with MyGUI and OSG
    /// @note The FontLoader needs to remain in scope as long as you want to use the loaded fonts.
    class FontLoader
    {
    public:
        FontLoader (ToUTF8::FromType encoding, const VFS::Manager* vfs);
        ~FontLoader();

        /// @param exportToFile export the converted fonts (Images and XML with glyph metrics) to files?
        void loadAllFonts (bool exportToFile);

    private:
        ToUTF8::FromType mEncoding;
        const VFS::Manager* mVFS;

        std::vector<MyGUI::ITexture*> mTextures;
        std::vector<MyGUI::ResourceManualFont*> mFonts;

        /// @param exportToFile export the converted font (Image and XML with glyph metrics) to files?
        void loadFont (const std::string& fileName, bool exportToFile);

        FontLoader(const FontLoader&);
        void operator=(const FontLoader&);
    };

}

#endif
