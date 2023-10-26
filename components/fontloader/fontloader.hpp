#ifndef OPENMW_COMPONENTS_FONTLOADER_H
#define OPENMW_COMPONENTS_FONTLOADER_H

#include <MyGUI_Version.h>
#include <MyGUI_XmlDocument.h>

#include <components/myguiplatform/myguidatamanager.hpp>
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
        FontLoader(ToUTF8::FromType encoding, const VFS::Manager* vfs, float scalingFactor);

        void overrideLineHeight(MyGUI::xml::ElementPtr _node, std::string_view _file, MyGUI::Version _version);

        static std::string_view getFontForFace(std::string_view face);

    private:
        ToUTF8::FromType mEncoding;
        const VFS::Manager* mVFS;
        float mScalingFactor;

        void loadFonts();
        void loadFont(const std::string& fontName, const std::string& fontId);

        void loadBitmapFont(const std::string& fileName, const std::string& fontId);
        void loadTrueTypeFont(const std::string& fileName, const std::string& fontId);

        FontLoader(const FontLoader&);
        void operator=(const FontLoader&);
    };

}

#endif
