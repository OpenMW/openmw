#include "fontloader.hpp"

#include <array>
#include <stdexcept>
#include <string_view>

#include <osg/Image>

#include <osgDB/WriteFile>

#include <MyGUI_FactoryManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_ResourceManager.h>
#include <MyGUI_ResourceManualFont.h>
#include <MyGUI_ResourceTrueTypeFont.h>
#include <MyGUI_XmlDocument.h>

#include <components/debug/debuglog.hpp>

#include <components/fallback/fallback.hpp>

#include <components/vfs/manager.hpp>

#include <components/misc/strings/algorithm.hpp>

#include <components/myguiplatform/scalinglayer.hpp>

#include <components/settings/values.hpp>

namespace
{
    MyGUI::xml::ElementPtr getProperty(MyGUI::xml::ElementPtr resourceNode, std::string_view propertyName)
    {
        MyGUI::xml::ElementPtr propertyNode = nullptr;
        MyGUI::xml::ElementEnumerator propertyIterator = resourceNode->getElementEnumerator();
        while (propertyIterator.next("Property"))
        {
            if (propertyIterator->findAttribute("key") == propertyName)
            {
                propertyNode = propertyIterator.current();
                break;
            }
        }

        return propertyNode;
    }

    MyGUI::IntSize getBookSize(MyGUI::IDataStream* layersStream)
    {
        MyGUI::xml::Document xmlDocument;
        xmlDocument.open(layersStream);
        MyGUI::xml::ElementPtr root = xmlDocument.getRoot();
        MyGUI::xml::ElementEnumerator layersIterator = root->getElementEnumerator();
        while (layersIterator.next("Layer"))
        {
            if (layersIterator->findAttribute("name") == "JournalBooks")
            {
                MyGUI::xml::ElementPtr sizeProperty = getProperty(layersIterator.current(), "Size");
                if (sizeProperty != nullptr)
                {
                    auto sizeValue = sizeProperty->findAttribute("value");
                    if (!sizeValue.empty())
                        return MyGUI::IntSize::parse(sizeValue);
                }
            }
        }

        return MyGUI::RenderManager::getInstance().getViewSize();
    }

    unsigned long utf8ToUnicode(std::string_view utf8)
    {
        if (utf8.empty())
            return 0;
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
            unicode = ch & 0x1F;
            numbytes = 1;
        }
        else if (ch <= 0xEF)
        {
            unicode = ch & 0x0F;
            numbytes = 2;
        }
        else if (ch <= 0xF7)
        {
            unicode = ch & 0x07;
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

    /// This is a hack for Polish font
    unsigned char mapUtf8Char(unsigned char c)
    {
        switch (c)
        {
            case 0x80:
                return 0xc6;
            case 0x81:
                return 0x9c;
            case 0x82:
                return 0xe6;
            case 0x83:
                return 0xb3;
            case 0x84:
                return 0xf1;
            case 0x85:
                return 0xb9;
            case 0x86:
                return 0xbf;
            case 0x87:
                return 0x9f;
            case 0x88:
                return 0xea;
            case 0x89:
                return 0xea;
            case 0x8a:
                return 0x00; // not contained in win1250
            case 0x8b:
                return 0x00; // not contained in win1250
            case 0x8c:
                return 0x8f;
            case 0x8d:
                return 0xaf;
            case 0x8e:
                return 0xa5;
            case 0x8f:
                return 0x8c;
            case 0x90:
                return 0xca;
            case 0x93:
                return 0xa3;
            case 0x94:
                return 0xf6;
            case 0x95:
                return 0xf3;
            case 0x96:
                return 0xaf;
            case 0x97:
                return 0x8f;
            case 0x99:
                return 0xd3;
            case 0x9a:
                return 0xd1;
            case 0x9c:
                return 0x00; // not contained in win1250
            case 0xa0:
                return 0xb9;
            case 0xa1:
                return 0xaf;
            case 0xa2:
                return 0xf3;
            case 0xa3:
                return 0xbf;
            case 0xa4:
                return 0x00; // not contained in win1250
            case 0xe1:
                return 0x8c;
            case 0xe3:
                return 0x00; // not contained in win1250
            case 0xf5:
                return 0x00; // not contained in win1250
            default:
                return c;
        }
    }

    // getUnicode includes various hacks for dealing with Morrowind's .fnt files that are *mostly*
    // in the expected win12XX encoding, but also have randomly swapped characters sometimes.
    // Looks like the Morrowind developers found standard encodings too boring and threw in some twists for fun.
    unsigned long getUnicode(unsigned char c, ToUTF8::Utf8Encoder& encoder, ToUTF8::FromType encoding)
    {
        if (encoding == ToUTF8::WINDOWS_1250) // Hack for polish font
        {
            const std::array<char, 2> str{ static_cast<char>(mapUtf8Char(c)), '\0' };
            return utf8ToUnicode(encoder.getUtf8(std::string_view(str.data(), 1)));
        }
        else
        {
            const std::array<char, 2> str{ static_cast<char>(c), '\0' };
            return utf8ToUnicode(encoder.getUtf8(std::string_view(str.data(), 1)));
        }
    }

    [[noreturn]] void fail(std::istream& stream, std::string_view fileName, std::string_view message)
    {
        std::stringstream error;
        error << "Font loading error: " << message;
        error << "\n  File: " << fileName;
        error << "\n  Offset: 0x" << std::hex << stream.tellg();
        throw std::runtime_error(error.str());
    }

}

namespace Gui
{

    FontLoader::FontLoader(ToUTF8::FromType encoding, const VFS::Manager* vfs, float scalingFactor)
        : mVFS(vfs)
        , mScalingFactor(scalingFactor)
    {
        if (encoding == ToUTF8::WINDOWS_1252)
            mEncoding = ToUTF8::CP437;
        else
            mEncoding = encoding;

        MyGUI::ResourceManager::getInstance().unregisterLoadXmlDelegate("Resource");
        MyGUI::ResourceManager::getInstance().registerLoadXmlDelegate("Resource")
            = MyGUI::newDelegate(this, &FontLoader::overrideLineHeight);

        loadFonts();
    }

    void FontLoader::loadFonts()
    {
        std::string defaultFont{ Fallback::Map::getString("Fonts_Font_0") };
        std::string scrollFont{ Fallback::Map::getString("Fonts_Font_2") };
        loadFont(defaultFont, "DefaultFont");
        loadFont(scrollFont, "ScrollFont");
        loadFont("DejaVuLGCSansMono",
            "MonoFont"); // We need to use a TrueType monospace font to display debug texts properly.

        // Use our TrueType fonts as a fallback.
        if (!MyGUI::ResourceManager::getInstance().isExist("DefaultFont")
            && !Misc::StringUtils::ciEqual(defaultFont, "MysticCards"))
            loadFont("MysticCards", "DefaultFont");
        if (!MyGUI::ResourceManager::getInstance().isExist("ScrollFont")
            && !Misc::StringUtils::ciEqual(scrollFont, "DemonicLetters"))
            loadFont("DemonicLetters", "ScrollFont");
    }

    void FontLoader::loadFont(const std::string& fileName, const std::string& fontId)
    {
        if (mVFS->exists("fonts/" + fileName + ".fnt"))
            loadBitmapFont(fileName + ".fnt", fontId);
        else if (mVFS->exists("fonts/" + fileName + ".omwfont"))
            loadTrueTypeFont(fileName + ".omwfont", fontId);
        else
            Log(Debug::Error) << "Font '" << fileName << "' is not found.";
    }

    void FontLoader::loadTrueTypeFont(const std::string& fileName, const std::string& fontId)
    {
        Log(Debug::Info) << "Loading font file " << fileName;

        osgMyGUI::DataManager* dataManager
            = dynamic_cast<osgMyGUI::DataManager*>(&osgMyGUI::DataManager::getInstance());
        if (!dataManager)
        {
            Log(Debug::Error) << "Can not load TrueType font " << fontId << ": osgMyGUI::DataManager is not available.";
            return;
        }

        // TODO: it may be worth to take in account resolution change, but it is not safe to replace used assets
        std::unique_ptr<MyGUI::IDataStream> layersStream(dataManager->getData("openmw_layers.xml"));
        MyGUI::IntSize bookSize = getBookSize(layersStream.get());
        float bookScale = osgMyGUI::ScalingLayer::getScaleFactor(bookSize);

        const auto oldDataPath = dataManager->getDataPath({});
        dataManager->setResourcePath("fonts");
        std::unique_ptr<MyGUI::IDataStream> dataStream(dataManager->getData(fileName));

        MyGUI::xml::Document xmlDocument;
        xmlDocument.open(dataStream.get());
        MyGUI::xml::ElementPtr root = xmlDocument.getRoot();

        MyGUI::xml::ElementEnumerator resourceNode = root->getElementEnumerator();
        bool valid = false;
        if (resourceNode.next("Resource"))
        {
            valid = resourceNode->findAttribute("type") == "ResourceTrueTypeFont";
        }

        if (valid == false)
        {
            dataManager->setResourcePath(oldDataPath);
            Log(Debug::Error) << "Can not load TrueType font " << fontId << ": " << fileName << " is invalid.";
            return;
        }

        int resolution = 70;
        MyGUI::xml::ElementPtr resolutionNode = getProperty(resourceNode.current(), "Resolution");
        if (resolutionNode == nullptr)
        {
            resolutionNode = resourceNode->createChild("Property");
            resolutionNode->addAttribute("key", "Resolution");
        }
        else
            resolution = MyGUI::utility::parseInt(resolutionNode->findAttribute("value"));

        resolutionNode->setAttribute("value", MyGUI::utility::toString(resolution * std::ceil(mScalingFactor)));

        MyGUI::xml::ElementPtr sizeNode = resourceNode->createChild("Property");
        sizeNode->addAttribute("key", "Size");
        sizeNode->addAttribute("value", std::to_string(Settings::gui().mFontSize));

        MyGUI::ResourceTrueTypeFont* font = static_cast<MyGUI::ResourceTrueTypeFont*>(
            MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceTrueTypeFont"));
        font->deserialization(resourceNode.current(), MyGUI::Version(3, 2, 0));
        font->setResourceName(fontId);
        MyGUI::ResourceManager::getInstance().addResource(font);

        resolutionNode->setAttribute(
            "value", MyGUI::utility::toString(static_cast<int>(resolution * bookScale * mScalingFactor)));

        MyGUI::ResourceTrueTypeFont* bookFont = static_cast<MyGUI::ResourceTrueTypeFont*>(
            MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceTrueTypeFont"));
        bookFont->deserialization(resourceNode.current(), MyGUI::Version(3, 2, 0));
        bookFont->setResourceName("Journalbook " + fontId);
        MyGUI::ResourceManager::getInstance().addResource(bookFont);

        dataManager->setResourcePath(oldDataPath);

        if (resourceNode.next("Resource"))
            Log(Debug::Warning) << "Font file " << fileName
                                << " contains multiple Resource entries, only first one will be used.";
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

    void FontLoader::loadBitmapFont(const std::string& fileName, const std::string& fontId)
    {
        Log(Debug::Info) << "Loading font file " << fileName;

        Files::IStreamPtr file = mVFS->get("fonts/" + fileName);

        float fontSize;
        file->read((char*)&fontSize, sizeof(fontSize));
        if (!file->good())
            fail(*file, fileName, "File too small to be a valid font");

        int one;
        file->read((char*)&one, sizeof(one));
        if (!file->good())
            fail(*file, fileName, "File too small to be a valid font");

        if (one != 1)
            fail(*file, fileName, "Unexpected value");

        file->read((char*)&one, sizeof(one));
        if (!file->good())
            fail(*file, fileName, "File too small to be a valid font");

        if (one != 1)
            fail(*file, fileName, "Unexpected value");

        char name_[284];
        file->read(name_, sizeof(name_));
        if (!file->good())
            fail(*file, fileName, "File too small to be a valid font");

        GlyphInfo data[256];
        file->read((char*)data, sizeof(data));
        if (!file->good())
            fail(*file, fileName, "File too small to be a valid font");

        file.reset();

        // Create the font texture
        std::string bitmapFilename = "fonts/" + std::string(name_) + ".tex";

        Files::IStreamPtr bitmapFile = mVFS->get(bitmapFilename);

        int width, height;
        bitmapFile->read((char*)&width, sizeof(int));
        bitmapFile->read((char*)&height, sizeof(int));

        if (!bitmapFile->good())
            fail(*bitmapFile, bitmapFilename, "File too small to be a valid bitmap");

        if (width <= 0 || height <= 0)
            fail(*bitmapFile, bitmapFilename, "Width and height must be positive");

        std::vector<char> textureData;
        textureData.resize(width * height * 4);
        bitmapFile->read(textureData.data(), width * height * 4);
        if (!bitmapFile->good())
            Log(Debug::Warning) << "Font bitmap " << bitmapFilename << " ended prematurely, using partial data ("
                                << bitmapFile->gcount() << "/" << (width * height * 4) << " bytes)";
        bitmapFile.reset();

        MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture(bitmapFilename);
        tex->createManual(width, height, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8A8);
        unsigned char* texData = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
        memcpy(texData, textureData.data(), textureData.size());
        tex->unlock();

        // We need to emulate loading from XML because the data members are private as of mygui 3.2.0
        MyGUI::xml::Document xmlDocument;
        MyGUI::xml::ElementPtr root = xmlDocument.createRoot("ResourceManualFont");

        root->addAttribute("name", fontId);

        MyGUI::xml::ElementPtr defaultHeight = root->createChild("Property");
        defaultHeight->addAttribute("key", "DefaultHeight");
        defaultHeight->addAttribute("value", fontSize);
        MyGUI::xml::ElementPtr source = root->createChild("Property");
        source->addAttribute("key", "Source");
        source->addAttribute("value", bitmapFilename);
        MyGUI::xml::ElementPtr codes = root->createChild("Codes");

        // Fall back from unavailable Win-1252 encoding symbols to similar characters available in CP-437 game fonts
        // Generally, that is, as this tries to follow Morrowind's interesting substitution logic

        using FallbackIndex = unsigned int;
        using UnicodeIndex = unsigned int;
        std::multimap<FallbackIndex, UnicodeIndex> additional;

        // Some symbols must not be shown altogether
        std::vector<UnicodeIndex> omitted;

        // Ignored madness: caret ^ (available in the font!) uses percent sign % glyph
        // We do not differentiate between glyphs that are always considered missing and glyphs that are not replaced
        // because there's (probably?) no point

        // € (Euro Sign, 0x80/U+20AC) is replaced with underscore
        // 0x81 (unused) is replaced with underscore
        additional.emplace(44, 0x201A); // ‚ (Single Low-9 Quotation Mark, 0x82) => , (comma)
        // ƒ (Latin Small Letter F with Hook, 0x83) is unavailable, not replaced
        additional.emplace(44, 0x201E); // „ (Double Low-9 Quotation Mark, 0x84) => , (comma)
        additional.emplace(46, 0x2026); // … (Horizontal Ellipsis, 0x85) => . (period)
        additional.emplace(43, 0x2020); // † (Dagger, 0x86) => + (plus sign)
        additional.emplace(216, 0x2021); // ‡ (Double Dagger, 0x87) => ╫
        additional.emplace(94, 0x02C6); // ˆ (Modifier Letter Circumflex Accent, 0x88) => ^ (caret)
        additional.emplace(37, 0x2030); // ‰ (Per Mille Sign, 0x89) => % (percent sign)
        additional.emplace(83, 0x0160); // Š (Latin Capital Letter S with Caron, 0x8A) => S (latin capital S)
        additional.emplace(60, 0x2039); // ‹ (Single Left-Pointing Angle Quotation Mark, 0x8B) => < (less-than)
        additional.emplace(79, 0x0152); // Œ (Latin Capital Ligature Oe, 0x8C) => O (latin capital O)
        // 0x8D (unused) is replaced with underscore
        additional.emplace(90, 0x017D); // Ž (Latin Capital Letter Z with Caron, 0x8E) => Z (latin capital Z)
        // 0x8F (unused) is replaced with underscore
        // 0x90 (unused) is replaced with underscore
        additional.emplace(39, 0x2018); // ‘ (Left Single Quotation Mark, 0x91) => ' (apostrophe) (MW: backtick)
        additional.emplace(39, 0x2019); // ’ (Right Single Quotation Mark, 0x92) => ' (apostrophe)
        additional.emplace(34, 0x201C); // “ (Left Double Quotation Mark, 0x93) => " (double quote mark)
        additional.emplace(34, 0x201D); // ” (Right Double Quotation Mark, 0x94) => " (double quote mark)
        omitted.push_back(0x2022); // • (Bullet, 0x95)
        additional.emplace(45, 0x2013); // – (En Dash, 0x96) => - (hyphen)
        additional.emplace(45, 0x2014); // — (Em Dash, 0x97) => - (hyphen)
        additional.emplace(126, 0x02DC); // ˜ (Small Tilde, 0x98) => ~ (tilde)
        additional.emplace(84, 0x2122); // ™ (Trade Mark Sign, 0x99) => T (latin capital T)
        additional.emplace(115, 0x0161); // š (Latin Small Letter S with Caron, 0x9A) => s (latin small S)
        additional.emplace(62, 0x203A); // › (Single Right-Pointing Angle Quotation Mark, 0x9B) => > (greater-than)
        additional.emplace(111, 0x0153); // œ (Latin Small Ligature Oe, 0x9C) => o (latin small O)
        // 0x9D (unused) is replaced with underscore
        additional.emplace(122, 0x017E); // ž (Latin Small Letter Z with Caron, 0x9E) => z (latin small Z)
        additional.emplace(89, 0x0178); // Ÿ (Latin Capital Letter Y with Diaeresis, 0x9F) => Y (latin capital Y)
        // Nbsp (No-Break Space, 0xA0) is unavailable, not replaced
        // ¡ (Inverted Exclamation Mark, 0xA1) is unavailable, not replaced
        // ¢ (Cent Sign, 0xA2) is unavailable, not replaced
        // £ (Pound Sign, 0xA3) is available but its glyph looks like œ (small oe ligature)
        omitted.push_back(0x00A4); // ¤ (Currency Sign)
        // ¥ (Yen Sign, 0xA5) is unavailable, not replaced
        additional.emplace(221, 0x00A6); // ¦ (Broken Bar, 0xA6) => ▌
        omitted.push_back(0x00A7); // § (Section Sign)
        additional.emplace(34, 0x00A8); // ¨ (Diaeresis) => " (double quote mark)
        additional.emplace(99, 0x00A9); // © (Copyright Sign) => c (latin small C)
        // ª (Feminine Ordinal Indicator, 0xAA) is unavailable, not replaced
        // « (Left-Pointing Double Angle Quotation Mark, 0xAB) is unavailable, not replaced
        // ¬ (Not Sign, 0xAC) is unavailable, not replaced
        additional.emplace(45, 0x00AD); // Soft Hyphen => - (hyphen)
        additional.emplace(114, 0x00AE); // ® (Registered Sign) => r (latin small R)
        additional.emplace(95, 0x00AF); // ¯ (Macron) => _ (underscore)
        // ° (Degree Sign, 0xB0) is unavailable, not replaced
        // ± (Plus-Minus Sign, 0xB1) is unavailable, not replaced
        // ² (Superscript Two, 0xB2) is unavailable, not replaced
        additional.emplace(51, 0x00B3); // ³ (Superscript Three) => 3 (three digit)
        additional.emplace(39, 0x00B4); // ´ (Acute Accent) => ' (apostrophe)
        // µ (Micro Sign, 0xB5) is unavailable, not replaced
        omitted.push_back(0x00B6); // ¶ (Pilcrow Sign)
        // · (Middle Dot, 0xB7) is unavailable, not replaced
        additional.emplace(44, 0x00B8); // ¸ (Cedilla) => , (comma)
        additional.emplace(49, 0x00B9); // ¹ (Superscript One) => 1 (one digit)
        // º (Masculine Ordinal Indicator, 0xBA) is unavailable, not replaced
        // » (Right-Pointing Double Angle Quotation Mark, 0xBB) is unavailable, not replaced
        // ¼ (Vulgar Fraction One Quarter, 0xBC) is unavailable, not replaced
        // ½ (Vulgar Fraction One Half, 0xBD) is unavailable, not replaced
        // ¾ (Vulgar Fraction Three Quarters, 0xBE) is replaced with underscore
        // ¿ (Inverted Question Mark, 0xBF) is unavailable, not replaced
        additional.emplace(65, 0x00C0); // À (Latin Capital Letter A with Grave) => A (latin capital A)
        additional.emplace(65, 0x00C1); // Á (Latin Capital Letter A with Acute) => A (latin capital A)
        additional.emplace(65, 0x00C2); // Â (Latin Capital Letter A with Circumflex) => A (latin capital A)
        additional.emplace(65, 0x00C3); // Ã (Latin Capital Letter A with Tilde) => A (latin capital A)
        // Ä (Latin Capital Letter A with Diaeresis, 0xC4) is available
        // Å (Latin Capital Letter A with Ring Above, 0xC5) is available
        // Æ (Latin Capital Letter Ae, 0xC6) is unavailable, not replaced
        // Ç (Latin Capital Letter C with Cedilla, 0xC7) is available
        additional.emplace(69, 0x00C8); // È (Latin Capital Letter E with Grave) => E (latin capital E)
        // É (Latin Capital Letter E with Acute, 0xC9) is available
        additional.emplace(69, 0x00CA); // Ê (Latin Capital Letter E with Circumflex) => E (latin capital E)
        additional.emplace(69, 0x00CB); // Ë (Latin Capital Letter E with Diaeresis) => E (latin capital E)
        additional.emplace(73, 0x00CC); // Ì (Latin Capital Letter I with Grave) => I (latin capital I)
        additional.emplace(73, 0x00CD); // Í (Latin Capital Letter I with Acute) => I (latin capital I)
        additional.emplace(73, 0x00CE); // Î (Latin Capital Letter I with Circumflex) => I (latin capital I)
        additional.emplace(73, 0x00CF); // Ï (Latin Capital Letter I with Diaeresis) => I (latin capital I)
        additional.emplace(68, 0x00D0); // Ð (Latin Capital Letter Eth) => D (latin capital D)
        // Ñ (Latin Capital Letter N with Tilde, 0xD1) is unavailable, not replaced
        additional.emplace(79, 0x00D2); // Ò (Latin Capital Letter O with Grave) => O (latin capital O)
        additional.emplace(79, 0x00D3); // Ó (Latin Capital Letter O with Acute) => O (latin capital O)
        additional.emplace(79, 0x00D4); // Ô (Latin Capital Letter O with Circumflex) => O (latin capital O)
        additional.emplace(79, 0x00D5); // Õ (Latin Capital Letter O with Tilde) => O (latin capital O)
        // Ö (Latin Capital Letter O with Diaeresis, 0xD6) is available
        additional.emplace(120, 0x00D7); // × (Multiplication Sign) => x (latin small X)
        additional.emplace(79, 0x00D8); // Ø (Latin Capital Letter O with Stroke) => O (latin capital O)
        additional.emplace(85, 0x00D9); // Ù (Latin Capital Letter U with Grave) => U (latin capital U)
        additional.emplace(85, 0x00DA); // Ú (Latin Capital Letter U with Acute) => U (latin capital U)
        additional.emplace(85, 0x00DB); // Û (Latin Capital Letter U with Circumflex) => U (latin capital U)
        // Ü (Latin Capital Letter U with Diaeresis, 0xDC) is available
        additional.emplace(89, 0x00DD); // Ý (Latin Capital Letter Y with Acute) => Y (latin capital Y)
        // 0xDE to 0xFF are generally not replaced with certain exceptions
        additional.emplace(97, 0x00E3); // ã (Latin Small Letter A with Tilde) => a (latin small A)
        additional.emplace(100, 0x00F0); // ð (Latin Small Letter Eth) => d (latin small D)
        additional.emplace(111, 0x00F5); // õ (Latin Small Letter O with Tilde) => o (latin small O)
        additional.emplace(111, 0x00F8); // ø (Latin Small Letter O with Stroke) => o (latin small O)
        additional.emplace(121, 0x00FD); // ý (Latin Small Letter Y with Acute) => y (latin small Y)

        // Russian Morrowind which uses Win-1251 encoding only does equivalent (often garbage) Win-1252 replacements
        // However, we'll provide custom replacements for Cyrillic io letters
        // These letters aren't a part of any content but they are printable
        additional.emplace(69, 0x0401); // Ё (Cyrillic Capital Letter Io) => E (latin capital E)
        additional.emplace(137, 0x0451); // ё (Cyrillic Small Letter Io) => ë (latin small E-diaeresis)

        for (int i = 0; i < 256; i++)
        {
            float x1 = data[i].top_left.x * width;
            float y1 = data[i].top_left.y * height;
            float w = data[i].top_right.x * width - x1;
            float h = data[i].bottom_left.y * height - y1;

            ToUTF8::Utf8Encoder encoder(mEncoding);
            unsigned long unicodeVal = getUnicode(i, encoder, mEncoding);

            MyGUI::xml::ElementPtr code = codes->createChild("Code");
            code->addAttribute("index", unicodeVal);
            code->addAttribute("coord",
                MyGUI::utility::toString(x1) + " " + MyGUI::utility::toString(y1) + " " + MyGUI::utility::toString(w)
                    + " " + MyGUI::utility::toString(h));
            code->addAttribute("advance", data[i].width);
            code->addAttribute("bearing",
                MyGUI::utility::toString(data[i].kerning) + " "
                    + MyGUI::utility::toString((fontSize - data[i].ascent)));
            code->addAttribute(
                "size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));

            for (auto [it, end] = additional.equal_range(i); it != end; ++it)
            {
                code = codes->createChild("Code");
                code->addAttribute("index", it->second);
                code->addAttribute("coord",
                    MyGUI::utility::toString(x1) + " " + MyGUI::utility::toString(y1) + " "
                        + MyGUI::utility::toString(w) + " " + MyGUI::utility::toString(h));
                code->addAttribute("advance", data[i].width);
                code->addAttribute("bearing",
                    MyGUI::utility::toString(data[i].kerning) + " "
                        + MyGUI::utility::toString((fontSize - data[i].ascent)));
                code->addAttribute(
                    "size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));
            }

            // ASCII vertical bar, use this as text input cursor
            if (i == 124)
            {
                MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
                cursorCode->addAttribute("index", MyGUI::FontCodeType::Cursor);
                cursorCode->addAttribute("coord",
                    MyGUI::utility::toString(x1) + " " + MyGUI::utility::toString(y1) + " "
                        + MyGUI::utility::toString(w) + " " + MyGUI::utility::toString(h));
                cursorCode->addAttribute("advance", data[i].width);
                cursorCode->addAttribute("bearing",
                    MyGUI::utility::toString(data[i].kerning) + " "
                        + MyGUI::utility::toString((fontSize - data[i].ascent)));
                cursorCode->addAttribute(
                    "size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));
            }

            // Underscore, use for NotDefined marker (used for glyphs not existing in the font)
            if (i == 95)
            {
                MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
                cursorCode->addAttribute("index", MyGUI::FontCodeType::NotDefined);
                cursorCode->addAttribute("coord",
                    MyGUI::utility::toString(x1) + " " + MyGUI::utility::toString(y1) + " "
                        + MyGUI::utility::toString(w) + " " + MyGUI::utility::toString(h));
                cursorCode->addAttribute("advance", data[i].width);
                cursorCode->addAttribute("bearing",
                    MyGUI::utility::toString(data[i].kerning) + " "
                        + MyGUI::utility::toString((fontSize - data[i].ascent)));
                cursorCode->addAttribute(
                    "size", MyGUI::IntSize(static_cast<int>(data[i].width), static_cast<int>(data[i].height)));
            }
        }

        // These are required as well, but the fonts don't provide them
        omitted.push_back(MyGUI::FontCodeType::Selected);
        omitted.push_back(MyGUI::FontCodeType::SelectedBack);
        for (const UnicodeIndex index : omitted)
        {
            MyGUI::xml::ElementPtr cursorCode = codes->createChild("Code");
            cursorCode->addAttribute("index", index);
            cursorCode->addAttribute("coord", "0 0 0 0");
            cursorCode->addAttribute("advance", "0");
            cursorCode->addAttribute("bearing", "0 0");
            cursorCode->addAttribute("size", "0 0");
        }

        // Register the font with MyGUI
        MyGUI::ResourceManualFont* font = static_cast<MyGUI::ResourceManualFont*>(
            MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceManualFont"));
        font->deserialization(root, MyGUI::Version(3, 2, 0));

        MyGUI::ResourceManualFont* bookFont = static_cast<MyGUI::ResourceManualFont*>(
            MyGUI::FactoryManager::getInstance().createObject("Resource", "ResourceManualFont"));
        bookFont->deserialization(root, MyGUI::Version(3, 2, 0));
        bookFont->setResourceName("Journalbook " + fontId);

        MyGUI::ResourceManager::getInstance().addResource(font);
        MyGUI::ResourceManager::getInstance().addResource(bookFont);
    }

    void FontLoader::overrideLineHeight(MyGUI::xml::ElementPtr _node, std::string_view _file, MyGUI::Version _version)
    {
        // We should adjust line height for MyGUI widgets depending on font size
        MyGUI::xml::ElementEnumerator resourceNode = _node->getElementEnumerator();
        while (resourceNode.next("Resource"))
        {
            auto type = resourceNode->findAttribute("type");

            if (Misc::StringUtils::ciEqual(type, "ResourceLayout"))
            {
                MyGUI::xml::ElementEnumerator resourceRootNode = resourceNode->getElementEnumerator();
                while (resourceRootNode.next("Widget"))
                {
                    if (resourceRootNode->findAttribute("name") != "Root")
                        continue;

                    MyGUI::xml::ElementPtr heightNode = resourceRootNode->createChild("UserString");
                    heightNode->addAttribute("key", "HeightLine");
                    heightNode->addAttribute("value", std::to_string(Settings::gui().mFontSize + 2));

                    break;
                }
            }
        }

        MyGUI::ResourceManager::getInstance().loadFromXmlNode(_node, _file, _version);
    }

    std::string_view FontLoader::getFontForFace(std::string_view face)
    {
        if (Misc::StringUtils::ciEqual(face, "daedric"))
            return "ScrollFont";

        return "DefaultFont";
    }
}
