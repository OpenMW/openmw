#ifndef OPENMW_COMPONENTS_FONTLOADER_H
#define OPENMW_COMPONENTS_FONTLOADER_H

#include <MyGUI_Version.h>
#include <MyGUI_XmlDocument.h>

#include <components/myguiplatform/myguidatamanager.hpp>
#include <components/toutf8/toutf8.hpp>

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
        /// @param exportFonts export the converted fonts (Images and XML with glyph metrics) to files?
        FontLoader(ToUTF8::FromType encoding, const VFS::Manager* vfs, float scalingFactor, bool exportFonts);

        void overrideLineHeight(MyGUI::xml::ElementPtr node, std::string_view file, MyGUI::Version version);

        static std::string_view getFontForFace(std::string_view face);

    private:
        struct FontId
        {
            std::string_view mValue;
        };

        ToUTF8::FromType mEncoding;
        const VFS::Manager* mVFS;
        float mScalingFactor;
        bool mExportFonts;

        void loadFonts();
        void loadFont(FontId fontId, std::string_view fontName);

        void loadBitmapFont(FontId fontId, const VFS::Path::Normalized& path, std::istream& stream);
        void loadTrueTypeFont(FontId fontId, const VFS::Path::Normalized& path, std::istream& stream);

        FontLoader(const FontLoader&);
        void operator=(const FontLoader&);
    };

}

#endif
