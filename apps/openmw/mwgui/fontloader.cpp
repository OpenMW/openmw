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

    // getUtf8, aka the worst function ever written.
    // This includes various hacks for dealing with Morrowind's .fnt files that are *mostly*
    // in the expected win12XX encoding, but also have randomly swapped characters sometimes.
    // Looks like the Morrowind developers found standard encodings too boring and threw in some twists for fun.
    std::string getUtf8 (unsigned char c, ToUTF8::Utf8Encoder& encoder, ToUTF8::FromType encoding)
    {
        if (encoding == ToUTF8::WINDOWS_1250)
        {
            // Hacks for polish font
            unsigned char win1250;
            std::map<unsigned char, unsigned char> conv;
            conv[0x80] = 0xc6;
            conv[0x81] = 0x9c;
            conv[0x82] = 0xe6;
            conv[0x83] = 0xb3;
            conv[0x84] = 0xf1;
            conv[0x85] = 0xb9;
            conv[0x86] = 0xbf;
            conv[0x87] = 0x9f;
            conv[0x88] = 0xea;
            conv[0x89] = 0xea;
            conv[0x8a] = 0x0; // not contained in win1250
            conv[0x8b] = 0x0; // not contained in win1250
            conv[0x8c] = 0x8f;
            conv[0x8d] = 0xaf;
            conv[0x8e] = 0xa5;
            conv[0x8f] = 0x8c;
            conv[0x90] = 0xca;
            conv[0x93] = 0xa3;
            conv[0x94] = 0xf6;
            conv[0x95] = 0xf3;
            conv[0x96] = 0xaf;
            conv[0x97] = 0x8f;
            conv[0x99] = 0xd3;
            conv[0x9a] = 0xd1;
            conv[0x9c] = 0x0; // not contained in win1250
            conv[0xa0] = 0xb9;
            conv[0xa1] = 0xaf;
            conv[0xa2] = 0xf3;
            conv[0xa3] = 0xbf;
            conv[0xa4] = 0x0; // not contained in win1250
            conv[0xe1] = 0x8c;
            // Can't remember if this was supposed to read 0xe2, or is it just an extraneous copypaste?
            //conv[0xe1] = 0x8c;
            conv[0xe3] = 0x0; // not contained in win1250
            conv[0xf5] = 0x0; // not contained in win1250

            if (conv.find(c) != conv.end())
                win1250 = conv[c];
            else
                win1250 = c;
            return encoder.getUtf8(std::string(1, win1250));
        }
        else
            return encoder.getUtf8(std::string(1, c));
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

        std::string resourceName;
        if (name.size() >= 5 && Misc::StringUtils::ciEqual(name.substr(0, 5), "magic"))
            resourceName = "Magic Cards";
        else if (name.size() >= 7 && Misc::StringUtils::ciEqual(name.substr(0, 7), "century"))
            resourceName = "Century Gothic";
        else if (name.size() >= 7 && Misc::StringUtils::ciEqual(name.substr(0, 7), "daedric"))
            resourceName = "Daedric";
        else
            return; // no point in loading it, since there is no way of using additional fonts

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
        root->addAttribute("name", resourceName);

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
            unsigned long unicodeVal = utf8ToUnicode(getUtf8(i, encoder, mEncoding));

            MyGUI::xml::ElementPtr code = codes->createChild("Code");
            code->addAttribute("index", unicodeVal);
            code->addAttribute("coord", MyGUI::utility::toString(x1) + " "
                                        + MyGUI::utility::toString(y1) + " "
                                        + MyGUI::utility::toString(w) + " "
                                        + MyGUI::utility::toString(h));
            code->addAttribute("advance", data[i].width);
            code->addAttribute("bearing", MyGUI::utility::toString(data[i].kerning) + " "
                               + MyGUI::utility::toString((fontSize-data[i].ascent)));

            // More hacks! The french game uses several win1252 characters that are not included
            // in the cp437 encoding of the font. Fall back to similar available characters.
            if (mEncoding == ToUTF8::CP437)
            {
                std::multimap<int, int> additional;
                additional.insert(std::make_pair(39, 0x2019)); // apostrophe
                additional.insert(std::make_pair(45, 0x2013)); // dash
                additional.insert(std::make_pair(34, 0x201D)); // right double quotation mark
                additional.insert(std::make_pair(34, 0x201C)); // left double quotation mark
                for (std::multimap<int, int>::iterator it = additional.begin(); it != additional.end(); ++it)
                {
                    if (it->first != i)
                        continue;

                    MyGUI::xml::ElementPtr code = codes->createChild("Code");
                    code->addAttribute("index", it->second);
                    code->addAttribute("coord", MyGUI::utility::toString(x1) + " "
                                                + MyGUI::utility::toString(y1) + " "
                                                + MyGUI::utility::toString(w) + " "
                                                + MyGUI::utility::toString(h));
                    code->addAttribute("advance", data[i].width);
                    code->addAttribute("bearing", MyGUI::utility::toString(data[i].kerning) + " "
                                       + MyGUI::utility::toString((fontSize-data[i].ascent)));
                }
            }

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

            // Question mark, use for NotDefined marker (used for glyphs not existing in the font)
            if (i == 63)
            {
                MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
                cursorCode->addAttribute("index", MyGUI::FontCodeType::NotDefined);
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
        for (int i=0; i<2; ++i)
        {
            MyGUI::FontCodeType::Enum type;
            if(i == 0)
                type = MyGUI::FontCodeType::Selected;
            else if (i == 1)
                type = MyGUI::FontCodeType::SelectedBack;

            MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
            cursorCode->addAttribute("index", type);
            cursorCode->addAttribute("coord", "0 0 0 0");
            cursorCode->addAttribute("advance", "0");
            cursorCode->addAttribute("bearing", "0 0");

        }

        font->deserialization(root, MyGUI::Version(3,2,0));

        MyGUI::ResourceManager::getInstance().removeByName(font->getResourceName());
        MyGUI::ResourceManager::getInstance().addResource(font);
    }

}
