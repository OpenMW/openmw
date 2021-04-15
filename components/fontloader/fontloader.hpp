#ifndef OPENMW_COMPONENTS_FONTLOADER_H
#define OPENMW_COMPONENTS_FONTLOADER_H

#include "boost/filesystem/operations.hpp"

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
        FontLoader (ToUTF8::FromType encoding, const VFS::Manager* vfs, const std::string& userDataPath, float scalingFactor);
        ~FontLoader();

        /// @param exportToFile export the converted fonts (Images and XML with glyph metrics) to files?
        void loadBitmapFonts (bool exportToFile);
        void loadTrueTypeFonts ();

        void loadFontFromXml(MyGUI::xml::ElementPtr _node, const std::string& _file, MyGUI::Version _version);

        int getFontHeight();

    private:
        ToUTF8::FromType mEncoding;
        const VFS::Manager* mVFS;
        std::string mUserDataPath;
        int mFontHeight;
        float mScalingFactor;

        std::vector<MyGUI::ITexture*> mTextures;
        std::vector<MyGUI::ResourceManualFont*> mFonts;

        /// @param exportToFile export the converted font (Image and XML with glyph metrics) to files?
        void loadBitmapFont (const std::string& fileName, bool exportToFile);

        FontLoader(const FontLoader&);
        void operator=(const FontLoader&);
    };

}

#endif
