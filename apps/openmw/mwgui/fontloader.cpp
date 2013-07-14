#include "fontloader.hpp"

#include <OgreResourceGroupManager.h>
#include <OgreTextureManager.h>

#include <MyGUI_ResourceManager.h>
#include <MyGUI_FontManager.h>
#include <MyGUI_ResourceManualFont.h>
#include <MyGUI_XmlDocument.h>
#include <MyGUI_FactoryManager.h>

#include <components/misc/stringops.hpp>

namespace
{
    unsigned long utf8ToUnicode(const std::string& utf8)
    {
        size_t i = 0;
        unsigned long unicode;
        size_t todo;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F)
        {
            unicode = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            throw std::logic_error("not a UTF-8 string");
        }
        else if (ch <= 0xDF)
        {
            unicode = ch&0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            unicode = ch&0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            unicode = ch&0x07;
            todo = 3;
        }
        else
        {
            throw std::logic_error("not a UTF-8 string");
        }
        for (size_t j = 0; j < todo; ++j)
        {
            unsigned char ch = utf8[i++];
            if (ch < 0x80 || ch > 0xBF)
                throw std::logic_error("not a UTF-8 string");
            unicode <<= 6;
            unicode += ch & 0x3F;
        }
        if (unicode >= 0xD800 && unicode <= 0xDFFF)
            throw std::logic_error("not a UTF-8 string");
        if (unicode > 0x10FFFF)
            throw std::logic_error("not a UTF-8 string");

        return unicode;
    }
}

namespace MWGui
{

    FontLoader::FontLoader(ToUTF8::FromType encoding)
    {
        if (encoding == ToUTF8::WINDOWS_1252)
            mEncoding = ToUTF8::CP437;
        else
            mEncoding = encoding;
    }

    void FontLoader::loadAllFonts()
    {
        Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingleton().getResourceGroups ();
        for (Ogre::StringVector::iterator it = groups.begin(); it != groups.end(); ++it)
        {
            Ogre::StringVectorPtr resourcesInThisGroup = Ogre::ResourceGroupManager::getSingleton ().findResourceNames (*it, "*.fnt");
            for (Ogre::StringVector::iterator resource = resourcesInThisGroup->begin(); resource != resourcesInThisGroup->end(); ++resource)
            {
                loadFont(*resource);
            }
        }
    }


    typedef struct
    {
        float x;
        float y;
    } Point;

    typedef struct
    {
        float u1; // appears unused, always 0
        Point top_left;
        Point top_right;
        Point bottom_left;
        Point bottom_right;
        float width;
        float height;
        float u2; // appears unused, always 0
        float kerning;
        float ascent;
    } GlyphInfo;

    void FontLoader::loadFont(const std::string &fileName)
    {
        Ogre::DataStreamPtr file = Ogre::ResourceGroupManager::getSingleton().openResource(fileName);

        float fontSize;
        int one;
        file->read(&fontSize, sizeof(fontSize));

        file->read(&one, sizeof(int));
        assert(one == 1);
        file->read(&one, sizeof(int));
        assert(one == 1);

        char name_[284];
        file->read(name_, sizeof(name_));
        std::string name(name_);

        GlyphInfo data[256];
        file->read(data, sizeof(data));
        file->close();

        // Create the font texture
        std::string bitmapFilename = "Fonts/" + std::string(name) + ".tex";
        Ogre::DataStreamPtr bitmapFile = Ogre::ResourceGroupManager::getSingleton().openResource(bitmapFilename);

        int width, height;
        bitmapFile->read(&width, sizeof(int));
        bitmapFile->read(&height, sizeof(int));

        std::vector<Ogre::uchar> textureData;
        textureData.resize(width*height*4);
        bitmapFile->read(&textureData[0], width*height*4);
        bitmapFile->close();

        std::string textureName = name;
        Ogre::Image image;
        image.loadDynamicImage(&textureData[0], width, height, Ogre::PF_BYTE_RGBA);
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(textureName,
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            width, height, 0, Ogre::PF_BYTE_RGBA);
        texture->loadImage(image);

        // Register the font with MyGUI
        MyGUI::ResourceManualFont* font = static_cast<MyGUI::ResourceManualFont*>(
                    MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceManualFont"));
        // We need to emulate loading from XML because the data members are private as of mygui 3.2.0
        MyGUI::xml::Document xmlDocument;
        MyGUI::xml::ElementPtr root = xmlDocument.createRoot("ResourceManualFont");

        if (name.size() >= 5 && Misc::StringUtils::ciEqual(name.substr(0, 5), "magic"))
            root->addAttribute("name", "Magic Cards");
        else if (name.size() >= 7 && Misc::StringUtils::ciEqual(name.substr(0, 7), "century"))
            root->addAttribute("name", "Century Gothic");
        else if (name.size() >= 7 && Misc::StringUtils::ciEqual(name.substr(0, 7), "daedric"))
            root->addAttribute("name", "Daedric");
        else
            return; // no point in loading it, since there is no way of using additional fonts

        MyGUI::xml::ElementPtr defaultHeight = root->createChild("Property");
        defaultHeight->addAttribute("key", "DefaultHeight");
        defaultHeight->addAttribute("value", fontSize);
        MyGUI::xml::ElementPtr source = root->createChild("Property");
        source->addAttribute("key", "Source");
        source->addAttribute("value", std::string(textureName));
        MyGUI::xml::ElementPtr codes = root->createChild("Codes");

        for(int i = 0; i < 256; i++)
        {
            int x1 = data[i].top_left.x*width;
            int y1 = data[i].top_left.y*height;
            int w  = data[i].top_right.x*width - x1;
            int h  = data[i].bottom_left.y*height - y1;

            ToUTF8::Utf8Encoder encoder(mEncoding);
            unsigned long unicodeVal = utf8ToUnicode(encoder.getUtf8(std::string(1, (unsigned char)(i))));

            MyGUI::xml::ElementPtr code = codes->createChild("Code");
            code->addAttribute("index", unicodeVal);
            code->addAttribute("coord", MyGUI::utility::toString(x1) + " "
                                        + MyGUI::utility::toString(y1) + " "
                                        + MyGUI::utility::toString(w) + " "
                                        + MyGUI::utility::toString(h));
            code->addAttribute("advance", data[i].width);
            code->addAttribute("bearing", MyGUI::utility::toString(data[i].kerning) + " "
                               + MyGUI::utility::toString((fontSize-data[i].ascent)));

            // ASCII vertical bar, use this as text input cursor
            if (i == 124)
            {
                MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
                cursorCode->addAttribute("index", MyGUI::FontCodeType::Cursor);
                cursorCode->addAttribute("coord", MyGUI::utility::toString(x1) + " "
                                            + MyGUI::utility::toString(y1) + " "
                                            + MyGUI::utility::toString(w) + " "
                                            + MyGUI::utility::toString(h));
                cursorCode->addAttribute("advance", data[i].width);
                cursorCode->addAttribute("bearing", MyGUI::utility::toString(data[i].kerning) + " "
                                   + MyGUI::utility::toString((fontSize-data[i].ascent)));
            }
        }

        // These are required as well, but the fonts don't provide them
        for (int i=0; i<3; ++i)
        {
            MyGUI::FontCodeType::Enum type;
            if(i == 0)
                type = MyGUI::FontCodeType::Selected;
            else if (i == 1)
                type = MyGUI::FontCodeType::SelectedBack;
            else if (i == 2)
                type = MyGUI::FontCodeType::NotDefined;

            MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
            cursorCode->addAttribute("index", type);
            cursorCode->addAttribute("coord", "0 0 0 0");
            cursorCode->addAttribute("advance", "0");
            cursorCode->addAttribute("bearing", "0 0");

        }

        font->deserialization(root, MyGUI::Version(3,2,0));

        MyGUI::ResourceManager::getInstance().addResource(font);
    }

}
