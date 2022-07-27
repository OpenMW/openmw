#ifndef OPENMW_COMPONENTS_FONTLOADER_H
#define OPENMW_COMPONENTS_FONTLOADER_H

#include <MyGUI_XmlDocument.h>
#include <MyGUI_Version.h>

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
        FontLoader (ToUTF8::FromType encoding, const VFS::Manager* vfs, float scalingFactor);

        void loadBitmapFonts ();
        void loadTrueTypeFonts ();

        void loadFontFromXml(MyGUI::xml::ElementPtr _node, const std::string& _file, MyGUI::Version _version);

        int getFontHeight();

        static std::string getFontForFace(const std::string& face);

    private:
        ToUTF8::FromType mEncoding;
        const VFS::Manager* mVFS;
        int mFontHeight;
        float mScalingFactor;

        std::vector<MyGUI::ResourceManualFont*> mFonts;

        std::string getInternalFontName(const std::string& name);

        void loadBitmapFont (const std::string& fileName);

        FontLoader(const FontLoader&);
        void operator=(const FontLoader&);
    };

}

#endif
