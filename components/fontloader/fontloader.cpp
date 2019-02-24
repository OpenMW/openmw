#include "fontloader.hpp"

#include <stdexcept>

#include <osg/Image>

#include <osgDB/WriteFile>

#include <MyGUI_ResourceManager.h>
#include <MyGUI_FontManager.h>
#include <MyGUI_ResourceManualFont.h>
#include <MyGUI_XmlDocument.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_RenderManager.h>

#include <components/debug/debuglog.hpp>

#include <components/vfs/manager.hpp>

#include <components/misc/stringops.hpp>

#include <components/myguiplatform/myguitexture.hpp>

namespace
{
    unsigned long utf8ToUnicode(const std::string& utf8)
    {
        size_t i = 0;
        unsigned long unicode;
        size_t numbytes;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F)
        {
            unicode = ch;
            numbytes = 0;
        }
        else if (ch <= 0xBF)
        {
            throw std::logic_error("not a UTF-8 string");
        }
        else if (ch <= 0xDF)
        {
            unicode = ch&0x1F;
            numbytes = 1;
        }
        else if (ch <= 0xEF)
        {
            unicode = ch&0x0F;
            numbytes = 2;
        }
        else if (ch <= 0xF7)
        {
            unicode = ch&0x07;
            numbytes = 3;
        }
        else
        {
            throw std::logic_error("not a UTF-8 string");
        }
        for (size_t j = 0; j < numbytes; ++j)
        {
            ch = utf8[i++];
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

    void fail (Files::IStreamPtr file, const std::string& fileName, const std::string& message)
    {
        std::stringstream error;
        error << "Font loading error: " << message;
        error << "\n  File: " << fileName;
        error << "\n  Offset: 0x" << std::hex << file->tellg();
        throw std::runtime_error(error.str());
    }

}

namespace Gui
{

    FontLoader::FontLoader(ToUTF8::FromType encoding, const VFS::Manager* vfs, const std::string& userDataPath)
        : mVFS(vfs)
        , mUserDataPath(userDataPath)
    {
        if (encoding == ToUTF8::WINDOWS_1252)
            mEncoding = ToUTF8::CP437;
        else
            mEncoding = encoding;
    }

    FontLoader::~FontLoader()
    {
        for (std::vector<MyGUI::ITexture*>::iterator it = mTextures.begin(); it != mTextures.end(); ++it)
            delete *it;
        mTextures.clear();

        for (std::vector<MyGUI::ResourceManualFont*>::iterator it = mFonts.begin(); it != mFonts.end(); ++it)
        {
            try
            {
                MyGUI::ResourceManager::getInstance().removeByName((*it)->getResourceName());
            }
            catch(const MyGUI::Exception& e)
            {
                Log(Debug::Error) << "Error in the destructor: " << e.what();
            }
        }

        mFonts.clear();
    }

    void FontLoader::loadBitmapFonts(bool exportToFile)
    {
        const std::map<std::string, VFS::File*>& index = mVFS->getIndex();

        std::string pattern = "Fonts/";
        mVFS->normalizeFilename(pattern);

        std::map<std::string, VFS::File*>::const_iterator found = index.lower_bound(pattern);
        while (found != index.end())
        {
            const std::string& name = found->first;
            if (name.size() >= pattern.size() && name.substr(0, pattern.size()) == pattern)
            {
                size_t pos = name.find_last_of('.');
                if (pos != std::string::npos && name.compare(pos, name.size()-pos, ".fnt") == 0)
                    loadFont(name, exportToFile);
            }
            else
                break;
            ++found;
        }
    }

    void FontLoader::loadTrueTypeFonts()
    {
        osgMyGUI::DataManager* dataManager = dynamic_cast<osgMyGUI::DataManager*>(&osgMyGUI::DataManager::getInstance());
        if (!dataManager)
        {
            Log(Debug::Error) << "Can not load TrueType fonts: osgMyGUI::DataManager is not available.";
            return;
        }

        const std::string cfg = dataManager->getDataPath("");
        const std::string fontFile = mUserDataPath + "/" + "Fonts" + "/" + "openmw_font.xml";
        if (!boost::filesystem::exists(fontFile))
            return;

        dataManager->setResourcePath(mUserDataPath + "/" + "Fonts");
        MyGUI::ResourceManager::getInstance().load("openmw_font.xml");
        dataManager->setResourcePath(cfg);
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

    void FontLoader::loadFont(const std::string &fileName, bool exportToFile)
    {
        Files::IStreamPtr file = mVFS->get(fileName);

        float fontSize;
        file->read((char*)&fontSize, sizeof(fontSize));
        if (!file->good())
            fail(file, fileName, "File too small to be a valid font");

        int one;
        file->read((char*)&one, sizeof(one));
        if (!file->good())
            fail(file, fileName, "File too small to be a valid font");

        if (one != 1)
            fail(file, fileName, "Unexpected value");

        file->read((char*)&one, sizeof(one));
        if (!file->good())
            fail(file, fileName, "File too small to be a valid font");

        if (one != 1)
            fail(file, fileName, "Unexpected value");

        char name_[284];
        file->read(name_, sizeof(name_));
        if (!file->good())
            fail(file, fileName, "File too small to be a valid font");
        std::string name(name_);

        GlyphInfo data[256];
        file->read((char*)data, sizeof(data));
        if (!file->good())
            fail(file, fileName, "File too small to be a valid font");

        file.reset();

        // Create the font texture
        std::string bitmapFilename = "Fonts/" + std::string(name) + ".tex";

        Files::IStreamPtr bitmapFile = mVFS->get(bitmapFilename);

        int width, height;
        bitmapFile->read((char*)&width, sizeof(int));
        bitmapFile->read((char*)&height, sizeof(int));

        if (!bitmapFile->good())
            fail(bitmapFile, bitmapFilename, "File too small to be a valid bitmap");

        if (width <= 0 || height <= 0)
            fail(bitmapFile, bitmapFilename, "Width and height must be positive");

        std::vector<char> textureData;
        textureData.resize(width*height*4);
        bitmapFile->read(&textureData[0], width*height*4);
        if (!bitmapFile->good())
            fail(bitmapFile, bitmapFilename, "File too small to be a valid bitmap");
        bitmapFile.reset();

        std::string resourceName;
        if (name.size() >= 5 && Misc::StringUtils::ciEqual(name.substr(0, 5), "magic"))
            resourceName = "Magic Cards";
        else if (name.size() >= 7 && Misc::StringUtils::ciEqual(name.substr(0, 7), "century"))
            resourceName = "Century Gothic";
        else if (name.size() >= 7 && Misc::StringUtils::ciEqual(name.substr(0, 7), "daedric"))
            resourceName = "Daedric";

        if (exportToFile)
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            assert (image->isDataContiguous());
            memcpy(image->data(), &textureData[0], textureData.size());

            Log(Debug::Info) << "Writing " << resourceName + ".png";
            osgDB::writeImageFile(*image, resourceName + ".png");
        }

        // Register the font with MyGUI
        MyGUI::ResourceManualFont* font = static_cast<MyGUI::ResourceManualFont*>(
                    MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceManualFont"));
        mFonts.push_back(font);

        MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture(bitmapFilename);
        tex->createManual(width, height, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8A8);
        unsigned char* texData = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
        memcpy(texData, &textureData[0], textureData.size());
        tex->unlock();

        // Using ResourceManualFont::setTexture, enable for MyGUI 3.2.3
        /*
        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setImage(image);
        osgMyGUI::OSGTexture* myguiTex = new osgMyGUI::OSGTexture(texture);
        mTextures.push_back(myguiTex);
        font->setTexture(myguiTex);
        */

        // We need to emulate loading from XML because the data members are private as of mygui 3.2.0
        MyGUI::xml::Document xmlDocument;
        MyGUI::xml::ElementPtr root = xmlDocument.createRoot("ResourceManualFont");
        root->addAttribute("name", resourceName);

        MyGUI::xml::ElementPtr defaultHeight = root->createChild("Property");
        defaultHeight->addAttribute("key", "DefaultHeight");
        defaultHeight->addAttribute("value", fontSize);
        MyGUI::xml::ElementPtr source = root->createChild("Property");
        source->addAttribute("key", "Source");
        source->addAttribute("value", std::string(bitmapFilename));
        MyGUI::xml::ElementPtr codes = root->createChild("Codes");

        for(int i = 0; i < 256; i++)
        {
            float x1 = data[i].top_left.x*width;
            float y1 = data[i].top_left.y*height;
            float w  = data[i].top_right.x*width - x1;
            float h  = data[i].bottom_left.y*height - y1;

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
            code->addAttribute("size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));

            // Fall back from unavailable Windows-1252 encoding symbols to similar characters available in the game fonts
            std::multimap<int, int> additional; // fallback glyph index, unicode
            additional.insert(std::make_pair(156, 0x00A2)); // cent sign
            additional.insert(std::make_pair(89, 0x00A5)); // yen sign
            additional.insert(std::make_pair(221, 0x00A6)); // broken bar
            additional.insert(std::make_pair(99, 0x00A9)); // copyright sign
            additional.insert(std::make_pair(97, 0x00AA)); // prima ordinal indicator
            additional.insert(std::make_pair(60, 0x00AB)); // double left-pointing angle quotation mark
            additional.insert(std::make_pair(45, 0x00AD)); // soft hyphen
            additional.insert(std::make_pair(114, 0x00AE)); // registered trademark symbol
            additional.insert(std::make_pair(45, 0x00AF)); // macron
            additional.insert(std::make_pair(241, 0x00B1)); // plus-minus sign
            additional.insert(std::make_pair(50, 0x00B2)); // superscript two
            additional.insert(std::make_pair(51, 0x00B3)); // superscript three
            additional.insert(std::make_pair(44, 0x00B8)); // cedilla
            additional.insert(std::make_pair(49, 0x00B9)); // superscript one
            additional.insert(std::make_pair(111, 0x00BA)); // primo ordinal indicator
            additional.insert(std::make_pair(62, 0x00BB)); // double right-pointing angle quotation mark
            additional.insert(std::make_pair(63, 0x00BF)); // inverted question mark
            additional.insert(std::make_pair(65, 0x00C6)); // latin capital ae ligature
            additional.insert(std::make_pair(79, 0x00D8)); // latin capital o with stroke
            additional.insert(std::make_pair(97, 0x00E6)); // latin small ae ligature
            additional.insert(std::make_pair(111, 0x00F8)); // latin small o with stroke
            additional.insert(std::make_pair(79, 0x0152)); // latin capital oe ligature
            additional.insert(std::make_pair(111, 0x0153)); // latin small oe ligature
            additional.insert(std::make_pair(83, 0x015A)); // latin capital s with caron
            additional.insert(std::make_pair(115, 0x015B)); // latin small s with caron
            additional.insert(std::make_pair(89, 0x0178)); // latin capital y with diaresis
            additional.insert(std::make_pair(90, 0x017D)); // latin capital z with caron
            additional.insert(std::make_pair(122, 0x017E)); // latin small z with caron
            additional.insert(std::make_pair(102, 0x0192)); // latin small f with hook
            additional.insert(std::make_pair(94, 0x02C6)); // circumflex modifier
            additional.insert(std::make_pair(126, 0x02DC)); // small tilde
            additional.insert(std::make_pair(69, 0x0401)); // cyrillic capital io (no diaeresis latin e is available)
            additional.insert(std::make_pair(137, 0x0451)); // cyrillic small io
            additional.insert(std::make_pair(45, 0x2012)); // figure dash
            additional.insert(std::make_pair(45, 0x2013)); // en dash
            additional.insert(std::make_pair(45, 0x2014)); // em dash
            additional.insert(std::make_pair(39, 0x2018)); // left single quotation mark
            additional.insert(std::make_pair(39, 0x2019)); // right single quotation mark
            additional.insert(std::make_pair(44, 0x201A)); // single low quotation mark
            additional.insert(std::make_pair(39, 0x201B)); // single high quotation mark (reversed)
            additional.insert(std::make_pair(34, 0x201C)); // left double quotation mark
            additional.insert(std::make_pair(34, 0x201D)); // right double quotation mark
            additional.insert(std::make_pair(44, 0x201E)); // double low quotation mark
            additional.insert(std::make_pair(34, 0x201F)); // double high quotation mark (reversed)
            additional.insert(std::make_pair(43, 0x2020)); // dagger
            additional.insert(std::make_pair(216, 0x2021)); // double dagger (note: this glyph is not available)
            additional.insert(std::make_pair(46, 0x2026)); // ellipsis
            additional.insert(std::make_pair(37, 0x2030)); // per mille sign
            additional.insert(std::make_pair(60, 0x2039)); // single left-pointing angle quotation mark
            additional.insert(std::make_pair(62, 0x203A)); // single right-pointing angle quotation mark
            additional.insert(std::make_pair(101, 0x20AC)); // euro sign
            additional.insert(std::make_pair(84, 0x2122)); // trademark sign
            additional.insert(std::make_pair(45, 0x2212)); // minus sign

            for (std::multimap<int, int>::iterator it = additional.begin(); it != additional.end(); ++it)
            {
                if (it->first != i)
                    continue;
                code = codes->createChild("Code");
                code->addAttribute("index", it->second);
                code->addAttribute("coord", MyGUI::utility::toString(x1) + " "
                                            + MyGUI::utility::toString(y1) + " "
                                            + MyGUI::utility::toString(w) + " "
                                            + MyGUI::utility::toString(h));
                code->addAttribute("advance", data[i].width);
                code->addAttribute("bearing", MyGUI::utility::toString(data[i].kerning) + " "
                                   + MyGUI::utility::toString((fontSize-data[i].ascent)));
                code->addAttribute("size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));
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
                cursorCode->addAttribute("size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));
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
                cursorCode->addAttribute("size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));
            }
        }

        // These are required as well, but the fonts don't provide them
        for (int i=0; i<2; ++i)
        {
            MyGUI::FontCodeType::Enum type;
            if(i == 0)
                type = MyGUI::FontCodeType::Selected;
            else // if (i == 1)
                type = MyGUI::FontCodeType::SelectedBack;

            MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
            cursorCode->addAttribute("index", type);
            cursorCode->addAttribute("coord", "0 0 0 0");
            cursorCode->addAttribute("advance", "0");
            cursorCode->addAttribute("bearing", "0 0");
            cursorCode->addAttribute("size", "0 0");
        }

        if (exportToFile)
        {
            Log(Debug::Info) << "Writing " << resourceName + ".xml";
            xmlDocument.createDeclaration();
            xmlDocument.save(resourceName + ".xml");
        }

        font->deserialization(root, MyGUI::Version(3,2,0));

        // Setup "book" version of font as fallback if we will not use TrueType fonts
        MyGUI::ResourceManualFont* bookFont = static_cast<MyGUI::ResourceManualFont*>(
                    MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceManualFont"));
        mFonts.push_back(bookFont);
        bookFont->deserialization(root, MyGUI::Version(3,2,0));
        bookFont->setResourceName("Journalbook " + resourceName);

        // Remove automatically registered fonts
        for (std::vector<MyGUI::ResourceManualFont*>::iterator it = mFonts.begin(); it != mFonts.end();)
        {
            if ((*it)->getResourceName() == font->getResourceName())
            {
                MyGUI::ResourceManager::getInstance().removeByName(font->getResourceName());
                it = mFonts.erase(it);
            }
            else if ((*it)->getResourceName() == bookFont->getResourceName())
            {
                MyGUI::ResourceManager::getInstance().removeByName(bookFont->getResourceName());
                it = mFonts.erase(it);
            }
            else
                ++it;
        }

        MyGUI::ResourceManager::getInstance().addResource(font);
        MyGUI::ResourceManager::getInstance().addResource(bookFont);
    }

}
