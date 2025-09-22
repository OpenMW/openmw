#include "bookpage.hpp"

#include <optional>

#include "MyGUI_FactoryManager.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_TextureUtility.h"

#include <components/misc/utf8stream.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/settings/values.hpp>

namespace
{
    std::optional<MyGUI::GlyphInfo> getGlyphInfo(MyGUI::IFont* font, MyGUI::Char ch)
    {
        const MyGUI::GlyphInfo* gi = font->getGlyphInfo(ch);
        if (!gi)
            return {};
        const float scale = font->getDefaultHeight() / static_cast<float>(Settings::gui().mFontSize);
        MyGUI::GlyphInfo info = *gi;
        info.bearingX /= scale;
        info.bearingY /= scale;
        info.width /= scale;
        info.height /= scale;
        info.advance /= scale;
        return info;
    }

    bool ucsLineBreak(Utf8Stream::UnicodeChar codePoint)
    {
        return codePoint == '\n';
    }

    bool ucsCarriageReturn(Utf8Stream::UnicodeChar codePoint)
    {
        return codePoint == '\r';
    }

    // Normal no-break space (0x00A0) is ignored here
    // because Morrowind compatibility requires us to render its glyph
    bool ucsSpace(Utf8Stream::UnicodeChar codePoint)
    {
        switch (codePoint)
        {
            case 0x0020: // SPACE
            case 0x1680: // OGHAM SPACE MARK
            case 0x180E: // MONGOLIAN VOWEL SEPARATOR
            case 0x2000: // EN QUAD
            case 0x2001: // EM QUAD
            case 0x2002: // EN SPACE
            case 0x2003: // EM SPACE
            case 0x2004: // THREE-PER-EM SPACE
            case 0x2005: // FOUR-PER-EM SPACE
            case 0x2006: // SIX-PER-EM SPACE
            case 0x2007: // FIGURE SPACE
            case 0x2008: // PUNCTUATION SPACE
            case 0x2009: // THIN SPACE
            case 0x200A: // HAIR SPACE
            case 0x200B: // ZERO WIDTH SPACE
            case 0x202F: // NARROW NO-BREAK SPACE
            case 0x205F: // MEDIUM MATHEMATICAL SPACE
            case 0x3000: // IDEOGRAPHIC SPACE
            case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
                return true;
            default:
                return false;
        }
    }

    // No-break spaces (0x00A0, 0x202F, 0xFEFF - normal, narrow, zero width)
    // are ignored here for obvious reasons
    // Figure space (0x2007) is not a breaking space either
    bool ucsBreakingSpace(int codePoint)
    {
        switch (codePoint)
        {
            case 0x0020: // SPACE
            case 0x1680: // OGHAM SPACE MARK
            case 0x180E: // MONGOLIAN VOWEL SEPARATOR
            case 0x2000: // EN QUAD
            case 0x2001: // EM QUAD
            case 0x2002: // EN SPACE
            case 0x2003: // EM SPACE
            case 0x2004: // THREE-PER-EM SPACE
            case 0x2005: // FOUR-PER-EM SPACE
            case 0x2006: // SIX-PER-EM SPACE
            case 0x2008: // PUNCTUATION SPACE
            case 0x2009: // THIN SPACE
            case 0x200A: // HAIR SPACE
            case 0x200B: // ZERO WIDTH SPACE
            case 0x205F: // MEDIUM MATHEMATICAL SPACE
            case 0x3000: // IDEOGRAPHIC SPACE
                return true;
            default:
                return false;
        }
    }
}

namespace MWGui
{
    struct TypesetBookImpl;
    class PageDisplay;
    class BookPageImpl;

    struct BookTypesetter::Style
    {
        virtual ~Style() = default;
    };

    struct TypesetBookImpl : TypesetBook
    {
        typedef std::pair<Utf8Stream::Point, Utf8Stream::Point> Range;

        struct StyleImpl : BookTypesetter::Style
        {
            MyGUI::IFont* mFont;
            MyGUI::Colour mHotColour;
            MyGUI::Colour mActiveColour;
            MyGUI::Colour mNormalColour;
            InteractiveId mInteractiveId;

            bool match(MyGUI::IFont* tstFont, const MyGUI::Colour& tstHotColour, const MyGUI::Colour& tstActiveColour,
                const MyGUI::Colour& tstNormalColour, InteractiveId tstInteractiveId) const
            {
                return (mFont == tstFont)
                    && partialMatch(tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
            }

            bool match(std::string_view tstFont, const MyGUI::Colour& tstHotColour,
                const MyGUI::Colour& tstActiveColour, const MyGUI::Colour& tstNormalColour,
                InteractiveId tstInteractiveId) const
            {
                return (mFont->getResourceName() == tstFont)
                    && partialMatch(tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
            }

            bool partialMatch(const MyGUI::Colour& tstHotColour, const MyGUI::Colour& tstActiveColour,
                const MyGUI::Colour& tstNormalColour, InteractiveId tstInteractiveId) const
            {
                return (mHotColour == tstHotColour) && (mActiveColour == tstActiveColour)
                    && (mNormalColour == tstNormalColour) && (mInteractiveId == tstInteractiveId);
            }
        };

        struct Run
        {
            StyleImpl* mStyle;
            Range mRange;
            int mLeft, mRight;
            int mPrintableChars;
        };

        struct Line
        {
            std::vector<Run> mRuns;
            MyGUI::IntRect mRect;
        };

        struct Section
        {
            std::vector<Line> mLines;
            MyGUI::IntRect mRect;
        };

        typedef std::vector<Section> Sections;

        // Holds "top" and "bottom" vertical coordinates in the source text.
        // A page is basically a "window" into a portion of the source text, similar to a ScrollView.
        typedef std::pair<int, int> Page;

        std::vector<Page> mPages;
        Sections mSections;
        std::list<Content> mContents;
        std::list<StyleImpl> mStyles;
        MyGUI::IntRect mRect;

        void setColour(size_t section, size_t line, size_t run, const MyGUI::Colour& colour) const override
        {
            if (section >= mSections.size())
                return;
            if (line >= mSections[section].mLines.size())
                return;
            if (run >= mSections[section].mLines[line].mRuns.size())
                return;

            mSections[section].mLines[line].mRuns[run].mStyle->mNormalColour = colour;
        }

        virtual ~TypesetBookImpl() = default;

        Range addContent(std::string_view text)
        {
            Content& content = mContents.emplace_back(text.begin(), text.end());

            if (content.empty())
                return Range(nullptr, nullptr);

            return Range(content.data(), content.data() + content.size());
        }

        size_t pageCount() const override { return mPages.size(); }

        std::pair<unsigned int, unsigned int> getSize() const override
        {
            return std::make_pair(mRect.width(), mRect.height());
        }

        template <typename Visitor>
        void visitRuns(int top, int bottom, MyGUI::IFont* font, Visitor const& visitor) const
        {
            for (const Section& section : mSections)
            {
                if (top >= mRect.bottom || bottom <= section.mRect.top)
                    continue;
                for (const Line& line : section.mLines)
                {
                    if (top >= line.mRect.bottom || bottom <= line.mRect.top)
                        continue;
                    for (const Run& run : line.mRuns)
                    {
                        if (!font || run.mStyle->mFont == font)
                            visitor(section, line, run);
                    }
                }
            }
        }

        template <typename Visitor>
        void visitRuns(int top, int bottom, Visitor const& visitor) const
        {
            visitRuns(top, bottom, nullptr, visitor);
        }

        /// hit test with a margin for error. only hits on interactive text fragments are reported.
        StyleImpl* hitTestWithMargin(int left, int top)
        {
            StyleImpl* hit = hitTest(left, top);
            if (hit && hit->mInteractiveId != 0)
                return hit;

            const int maxMargin = 10;
            for (int margin = 1; margin < maxMargin; ++margin)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (i == 0)
                        hit = hitTest(left, top - margin);
                    else if (i == 1)
                        hit = hitTest(left, top + margin);
                    else if (i == 2)
                        hit = hitTest(left - margin, top);
                    else
                        hit = hitTest(left + margin, top);

                    if (hit && hit->mInteractiveId != 0)
                        return hit;
                }
            }
            return nullptr;
        }

        StyleImpl* hitTest(int left, int top) const
        {
            for (const Section& section : mSections)
            {
                if (top < section.mRect.top || top >= section.mRect.bottom)
                    continue;

                int left1 = left - section.mRect.left;

                for (const Line& line : section.mLines)
                {
                    if (top < line.mRect.top || top >= line.mRect.bottom)
                        continue;

                    int left2 = left1 - line.mRect.left;

                    for (const Run& run : line.mRuns)
                    {
                        if (left2 < run.mLeft || left2 >= run.mRight)
                            continue;

                        return run.mStyle;
                    }
                }
            }

            return nullptr;
        }

        MyGUI::IFont* affectedFont(StyleImpl* style)
        {
            for (const StyleImpl& s : mStyles)
                if (&s == style)
                    return s.mFont;
            return nullptr;
        }

        struct Typesetter;
    };

    struct TypesetBookImpl::Typesetter : BookTypesetter
    {
        struct PartialText
        {
            StyleImpl* mStyle;
            Utf8Stream::Point mBegin;
            Utf8Stream::Point mEnd;
            int mWidth;

            PartialText(StyleImpl* style, Utf8Stream::Point begin, Utf8Stream::Point end, int width)
                : mStyle(style)
                , mBegin(begin)
                , mEnd(end)
                , mWidth(width)
            {
            }
        };

        int mPageWidth;
        int mPageHeight;

        std::shared_ptr<TypesetBookImpl> mBook;
        Section* mSection;
        Line* mLine;
        Run* mRun;

        std::vector<Alignment> mSectionAlignment;
        std::vector<PartialText> mPartialWhitespace;
        std::vector<PartialText> mPartialWord;

        TypesetBookImpl::Content const* mCurrentContent;
        Alignment mCurrentAlignment;

        Typesetter(int width, int height)
            : mPageWidth(width)
            , mPageHeight(height)
            , mSection(nullptr)
            , mLine(nullptr)
            , mRun(nullptr)
            , mCurrentContent(nullptr)
            , mCurrentAlignment(AlignLeft)
        {
            mBook = std::make_shared<TypesetBookImpl>();
        }

        virtual ~Typesetter() = default;

        Style* createStyle(const std::string& fontName, const MyGUI::Colour& fontColour, bool useBookFont) override
        {
            std::string fullFontName;
            if (fontName.empty())
                fullFontName = MyGUI::FontManager::getInstance().getDefaultFont();
            else
                fullFontName = fontName;

            if (useBookFont)
                fullFontName = "Journalbook " + fullFontName;

            for (StyleImpl& style : mBook->mStyles)
                if (style.match(fullFontName, fontColour, fontColour, fontColour, 0))
                    return &style;

            MyGUI::IFont* font = MyGUI::FontManager::getInstance().getByName(fullFontName);
            if (!font)
                throw std::runtime_error(std::string("can't find font ") + fullFontName);

            StyleImpl& style = *mBook->mStyles.insert(mBook->mStyles.end(), StyleImpl());
            style.mFont = font;
            style.mHotColour = fontColour;
            style.mActiveColour = fontColour;
            style.mNormalColour = fontColour;
            style.mInteractiveId = 0;

            return &style;
        }

        Style* createHotStyle(Style* baseStyle, const MyGUI::Colour& normalColour, const MyGUI::Colour& hoverColour,
            const MyGUI::Colour& activeColour, InteractiveId id, bool unique) override
        {
            StyleImpl* const baseStyleImpl = static_cast<StyleImpl*>(baseStyle);

            if (!unique)
                for (StyleImpl& style : mBook->mStyles)
                    if (style.match(baseStyleImpl->mFont, hoverColour, activeColour, normalColour, id))
                        return &style;

            StyleImpl& style = *mBook->mStyles.insert(mBook->mStyles.end(), StyleImpl());

            style.mFont = baseStyleImpl->mFont;
            style.mHotColour = hoverColour;
            style.mActiveColour = activeColour;
            style.mNormalColour = normalColour;
            style.mInteractiveId = id;

            return &style;
        }

        void write(Style* style, std::string_view text) override
        {
            Range range = mBook->addContent(text);

            writeImpl(static_cast<StyleImpl*>(style), Utf8Stream(range.first, range.second));
        }

        const Content* addContent(std::string_view text, bool select) override
        {
            add_partial_text();

            Content& content = mBook->mContents.emplace_back(text.begin(), text.end());

            if (select)
                mCurrentContent = &content;

            return &content;
        }

        void selectContent(const Content* contentHandle) override
        {
            add_partial_text();

            mCurrentContent = contentHandle;
        }

        void write(Style* style, size_t begin, size_t end) override
        {
            assert(mCurrentContent != nullptr);
            assert(end <= mCurrentContent->size());
            assert(begin <= mCurrentContent->size());

            const Utf8Stream::Point contentBegin = mCurrentContent->data() + begin;
            const Utf8Stream::Point contentEnd = mCurrentContent->data() + end;

            writeImpl(static_cast<StyleImpl*>(style), Utf8Stream(contentBegin, contentEnd));
        }

        void lineBreak(float margin) override
        {
            assert(margin == 0); // TODO: figure out proper behavior here...

            add_partial_text();

            mRun = nullptr;
            mLine = nullptr;
        }

        void sectionBreak(int margin) override
        {
            add_partial_text();

            if (mBook->mSections.size() > 0)
            {
                mRun = nullptr;
                mLine = nullptr;
                mSection = nullptr;

                if (mBook->mRect.bottom < (mBook->mSections.back().mRect.bottom + margin))
                    mBook->mRect.bottom = (mBook->mSections.back().mRect.bottom + margin);
            }
        }

        void setSectionAlignment(Alignment sectionAlignment) override
        {
            add_partial_text();

            if (mSection != nullptr)
                mSectionAlignment.back() = sectionAlignment;
            mCurrentAlignment = sectionAlignment;
        }

        std::shared_ptr<TypesetBook> complete() override
        {
            int curPageStart = 0;
            int curPageStop = 0;

            add_partial_text();

            std::vector<Alignment>::iterator sa = mSectionAlignment.begin();
            for (Sections::iterator i = mBook->mSections.begin(); i != mBook->mSections.end(); ++i, ++sa)
            {
                // apply alignment to individual lines...
                for (Line& line : i->mLines)
                {
                    int width = line.mRect.width();
                    int excess = mPageWidth - width;

                    switch (*sa)
                    {
                        default:
                        case AlignLeft:
                            line.mRect.left = 0;
                            break;
                        case AlignCenter:
                            line.mRect.left = excess / 2;
                            break;
                        case AlignRight:
                            line.mRect.left = excess;
                            break;
                    }

                    line.mRect.right = line.mRect.left + width;
                }

                if (curPageStop == curPageStart)
                {
                    curPageStart = i->mRect.top;
                    curPageStop = i->mRect.top;
                }

                int spaceLeft = mPageHeight - (curPageStop - curPageStart);
                int sectionHeight = i->mRect.height();

                // This is NOT equal to i->mRect.height(), which doesn't account for section breaks.
                int spaceRequired = (i->mRect.bottom - curPageStop);
                if (curPageStart == curPageStop) // If this is a new page, the section break is not needed
                    spaceRequired = i->mRect.height();

                if (spaceRequired <= mPageHeight)
                {
                    if (spaceRequired > spaceLeft)
                    {
                        // The section won't completely fit on the current page. Finish the current page and start a new
                        // one.
                        assert(curPageStart != curPageStop);

                        mBook->mPages.emplace_back(curPageStart, curPageStop);

                        curPageStart = i->mRect.top;
                        curPageStop = i->mRect.bottom;
                    }
                    else
                        curPageStop = i->mRect.bottom;
                }
                else
                {
                    // The section won't completely fit on the current page. Finish the current page and start a new
                    // one.
                    mBook->mPages.emplace_back(curPageStart, curPageStop);

                    curPageStart = i->mRect.top;
                    curPageStop = i->mRect.bottom;

                    // split section
                    int sectionHeightLeft = sectionHeight;
                    while (sectionHeightLeft >= mPageHeight)
                    {
                        // Adjust to the top of the first line that does not fit on the current page anymore
                        int splitPos = curPageStop;
                        for (const Line& line : i->mLines)
                        {
                            if (line.mRect.bottom > curPageStart + mPageHeight)
                            {
                                splitPos = line.mRect.top;
                                break;
                            }
                        }

                        mBook->mPages.emplace_back(curPageStart, splitPos);
                        curPageStart = splitPos;
                        curPageStop = splitPos;

                        sectionHeightLeft = (i->mRect.bottom - splitPos);
                    }
                    curPageStop = i->mRect.bottom;
                }
            }

            if (curPageStart != curPageStop)
                mBook->mPages.emplace_back(curPageStart, curPageStop);

            return mBook;
        }

        void writeImpl(StyleImpl* style, Utf8Stream&& stream)
        {
            while (!stream.eof())
            {
                if (ucsLineBreak(stream.peek()))
                {
                    add_partial_text();
                    stream.consume();
                    mLine = nullptr;
                    mRun = nullptr;
                    continue;
                }

                if (ucsBreakingSpace(stream.peek()) && !mPartialWord.empty())
                    add_partial_text();

                int wordWidth = 0;
                int spaceWidth = 0;

                Utf8Stream::Point lead = stream.current();

                while (!stream.eof() && !ucsLineBreak(stream.peek()) && ucsBreakingSpace(stream.peek()))
                {
                    std::optional<MyGUI::GlyphInfo> info = getGlyphInfo(style->mFont, stream.peek());
                    if (info)
                        spaceWidth += static_cast<int>(info->advance + info->bearingX);
                    stream.consume();
                }

                Utf8Stream::Point origin = stream.current();

                while (!stream.eof() && !ucsLineBreak(stream.peek()) && !ucsBreakingSpace(stream.peek()))
                {
                    std::optional<MyGUI::GlyphInfo> info = getGlyphInfo(style->mFont, stream.peek());
                    if (info)
                        wordWidth += static_cast<int>(info->advance + info->bearingX);
                    stream.consume();
                }

                Utf8Stream::Point extent = stream.current();

                if (lead == extent)
                    break;

                if (lead != origin)
                    mPartialWhitespace.emplace_back(style, lead, origin, spaceWidth);
                if (origin != extent)
                    mPartialWord.emplace_back(style, origin, extent, wordWidth);
            }
        }

        void add_partial_text()
        {
            if (mPartialWhitespace.empty() && mPartialWord.empty())
                return;

            const int fontHeight = Settings::gui().mFontSize;
            int spaceWidth = 0;
            int wordWidth = 0;

            for (const PartialText& partialText : mPartialWhitespace)
                spaceWidth += partialText.mWidth;
            for (const PartialText& partialText : mPartialWord)
                wordWidth += partialText.mWidth;

            int left = mLine ? mLine->mRect.right : 0;

            if (left + spaceWidth + wordWidth > mPageWidth)
            {
                mLine = nullptr;
                mRun = nullptr;
                left = 0;
            }
            else
            {
                for (const PartialText& partialText : mPartialWhitespace)
                {
                    int top = mLine ? mLine->mRect.top : mBook->mRect.bottom;

                    appendRun(partialText.mStyle, partialText.mBegin, partialText.mEnd, 0, left + partialText.mWidth,
                        top + fontHeight);

                    left = mLine->mRect.right;
                }
            }

            for (const PartialText& partialText : mPartialWord)
            {
                int top = mLine ? mLine->mRect.top : mBook->mRect.bottom;
                const int numChars = static_cast<int>(partialText.mEnd - partialText.mBegin);
                appendRun(partialText.mStyle, partialText.mBegin, partialText.mEnd, numChars, left + partialText.mWidth,
                    top + fontHeight);

                left = mLine->mRect.right;
            }

            mPartialWhitespace.clear();
            mPartialWord.clear();
        }

        void appendRun(StyleImpl* style, Utf8Stream::Point begin, Utf8Stream::Point end, int pc, int right, int bottom)
        {
            if (mSection == nullptr)
            {
                mBook->mSections.push_back(Section());
                mSection = &mBook->mSections.back();
                mSection->mRect = MyGUI::IntRect(0, mBook->mRect.bottom, 0, mBook->mRect.bottom);
                mSectionAlignment.push_back(mCurrentAlignment);
            }

            if (mLine == nullptr)
            {
                mSection->mLines.push_back(Line());
                mLine = &mSection->mLines.back();
                mLine->mRect = MyGUI::IntRect(0, mSection->mRect.bottom, 0, mBook->mRect.bottom);
            }

            if (mBook->mRect.right < right)
                mBook->mRect.right = right;

            if (mBook->mRect.bottom < bottom)
                mBook->mRect.bottom = bottom;

            if (mSection->mRect.right < right)
                mSection->mRect.right = right;

            if (mSection->mRect.bottom < bottom)
                mSection->mRect.bottom = bottom;

            if (mLine->mRect.right < right)
                mLine->mRect.right = right;

            if (mLine->mRect.bottom < bottom)
                mLine->mRect.bottom = bottom;

            if (mRun == nullptr || mRun->mStyle != style || mRun->mRange.second != begin)
            {
                int left = mRun ? mRun->mRight : mLine->mRect.left;

                mLine->mRuns.push_back(Run());
                mRun = &mLine->mRuns.back();
                mRun->mStyle = style;
                mRun->mLeft = left;
                mRun->mRight = right;
                mRun->mRange.first = begin;
                mRun->mRange.second = end;
                mRun->mPrintableChars = pc;
                // Run->Locale = Locale;
            }
            else
            {
                mRun->mRight = right;
                mRun->mRange.second = end;
                mRun->mPrintableChars += pc;
            }
        }
    };

    std::shared_ptr<BookTypesetter> BookTypesetter::create(int pageWidth, int pageHeight)
    {
        return std::make_shared<TypesetBookImpl::Typesetter>(pageWidth, pageHeight);
    }

    namespace
    {
        struct RenderXform
        {
        public:
            float clipTop;
            float clipLeft;
            float clipRight;
            float clipBottom;

            float absoluteLeft;
            float absoluteTop;
            float leftOffset;
            float topOffset;

            float pixScaleX;
            float pixScaleY;
            float hOffset;
            float vOffset;

            RenderXform(MyGUI::ICroppedRectangle* croppedParent, MyGUI::RenderTargetInfo const& renderTargetInfo)
            {
                clipTop = static_cast<float>(croppedParent->_getMarginTop());
                clipLeft = static_cast<float>(croppedParent->_getMarginLeft());
                clipRight = static_cast<float>(croppedParent->getWidth() - croppedParent->_getMarginRight());
                clipBottom = static_cast<float>(croppedParent->getHeight() - croppedParent->_getMarginBottom());

                absoluteLeft = static_cast<float>(croppedParent->getAbsoluteLeft());
                absoluteTop = static_cast<float>(croppedParent->getAbsoluteTop());
                leftOffset = static_cast<float>(renderTargetInfo.leftOffset);
                topOffset = static_cast<float>(renderTargetInfo.topOffset);

                pixScaleX = renderTargetInfo.pixScaleX;
                pixScaleY = renderTargetInfo.pixScaleY;
                hOffset = renderTargetInfo.hOffset;
                vOffset = renderTargetInfo.vOffset;
            }

            bool clip(MyGUI::FloatRect& vr, MyGUI::FloatRect& tr)
            {
                if (vr.bottom <= clipTop || vr.right <= clipLeft || vr.left >= clipRight || vr.top >= clipBottom)
                    return false;

                if (vr.top < clipTop)
                {
                    tr.top += tr.height() * (clipTop - vr.top) / vr.height();
                    vr.top = clipTop;
                }

                if (vr.left < clipLeft)
                {
                    tr.left += tr.width() * (clipLeft - vr.left) / vr.width();
                    vr.left = clipLeft;
                }

                if (vr.right > clipRight)
                {
                    tr.right -= tr.width() * (vr.right - clipRight) / vr.width();
                    vr.right = clipRight;
                }

                if (vr.bottom > clipBottom)
                {
                    tr.bottom -= tr.height() * (vr.bottom - clipBottom) / vr.height();
                    vr.bottom = clipBottom;
                }

                return true;
            }

            MyGUI::FloatPoint operator()(MyGUI::FloatPoint pt)
            {
                pt.left = absoluteLeft - leftOffset + pt.left;
                pt.top = absoluteTop - topOffset + pt.top;

                pt.left = +(((pixScaleX * pt.left + hOffset) * 2.0f) - 1.0f);
                pt.top = -(((pixScaleY * pt.top + vOffset) * 2.0f) - 1.0f);

                return pt;
            }
        };

        struct GlyphStream
        {
            float mZ;
            uint32_t mC;
            MyGUI::IFont* mFont;
            MyGUI::FloatPoint mOrigin;
            MyGUI::FloatPoint mCursor;
            MyGUI::Vertex* mVertices;
            RenderXform mRenderXform;
            MyGUI::VertexColourType mVertexColourType;

            explicit GlyphStream(MyGUI::IFont* font, float left, float top, float z, MyGUI::Vertex* vertices,
                RenderXform const& renderXform)
                : mZ(z)
                , mC(0)
                , mFont(font)
                , mOrigin(left, top)
                , mVertices(vertices)
                , mRenderXform(renderXform)
            {
                assert(font != nullptr);
                mVertexColourType = MyGUI::RenderManager::getInstance().getVertexFormat();
            }

            ~GlyphStream() = default;

            MyGUI::Vertex* end() const { return mVertices; }

            void reset(float left, float top, MyGUI::Colour colour)
            {
                mC = MyGUI::texture_utility::toNativeColour(colour, MyGUI::VertexColourType::ColourARGB) | 0xFF000000;
                MyGUI::texture_utility::convertColour(mC, mVertexColourType);

                mCursor.left = mOrigin.left + left;
                mCursor.top = mOrigin.top + top;
            }

            void emitGlyph(MyGUI::Char ch)
            {
                std::optional<MyGUI::GlyphInfo> info = getGlyphInfo(mFont, ch);

                if (!info)
                    return;

                MyGUI::FloatRect vr;

                vr.left = mCursor.left + info->bearingX;
                vr.top = mCursor.top + info->bearingY;
                vr.right = vr.left + info->width;
                vr.bottom = vr.top + info->height;

                MyGUI::FloatRect tr = info->uvRect;

                if (mRenderXform.clip(vr, tr))
                    quad(vr, tr);

                mCursor.left += static_cast<int>(info->bearingX + info->advance);
            }

            void emitSpace(MyGUI::Char ch)
            {
                std::optional<MyGUI::GlyphInfo> info = getGlyphInfo(mFont, ch);

                if (info)
                    mCursor.left += static_cast<int>(info->bearingX + info->advance);
            }

        private:
            void quad(const MyGUI::FloatRect& vr, const MyGUI::FloatRect& tr)
            {
                vertex(vr.left, vr.top, tr.left, tr.top);
                vertex(vr.right, vr.top, tr.right, tr.top);
                vertex(vr.left, vr.bottom, tr.left, tr.bottom);
                vertex(vr.right, vr.top, tr.right, tr.top);
                vertex(vr.left, vr.bottom, tr.left, tr.bottom);
                vertex(vr.right, vr.bottom, tr.right, tr.bottom);
            }

            void vertex(float x, float y, float u, float v)
            {
                MyGUI::FloatPoint pt = mRenderXform(MyGUI::FloatPoint(x, y));

                mVertices->x = pt.left;
                mVertices->y = pt.top;
                mVertices->z = mZ;
                mVertices->u = u;
                mVertices->v = v;
                mVertices->colour = mC;

                ++mVertices;
            }
        };
    }

    class PageDisplay final : public MyGUI::ISubWidgetText
    {
        MYGUI_RTTI_DERIVED(PageDisplay)
    protected:
        bool mIsPageReset;
        size_t mPage;

        struct TextFormat : ISubWidget
        {
            MyGUI::IFont* mFont;
            int mCountVertex;
            MyGUI::ITexture* mTexture;
            MyGUI::RenderItem* mRenderItem;
            PageDisplay* mDisplay;

            TextFormat(MyGUI::IFont* id, PageDisplay* display)
                : mFont(id)
                , mCountVertex(0)
                , mTexture(nullptr)
                , mRenderItem(nullptr)
                , mDisplay(display)
            {
            }

            void createDrawItem(MyGUI::ILayerNode* node)
            {
                assert(mRenderItem == nullptr);

                if (mTexture != nullptr)
                {
                    mRenderItem = node->addToRenderItem(mTexture, false, false);
                    mRenderItem->addDrawItem(this, mCountVertex);
                }
            }

            void destroyDrawItem(MyGUI::ILayerNode* node)
            {
                assert(mTexture != nullptr ? mRenderItem != nullptr : mRenderItem == nullptr);

                if (mTexture != nullptr)
                {
                    mRenderItem->removeDrawItem(this);
                    mRenderItem = nullptr;
                }
            }

            void doRender() override { mDisplay->doRender(*this); }

            // this isn't really a sub-widget, its just a "drawitem" which
            // should have its own interface
            void createDrawItem(MyGUI::ITexture* /*texture*/, MyGUI::ILayerNode* /*node*/) override {}
            void destroyDrawItem() override {}
        };

        void resetPage()
        {
            mIsPageReset = true;
            mPage = 0;
        }

        void setPage(size_t page)
        {
            mIsPageReset = false;
            mPage = page;
        }

        bool isPageDifferent(size_t page) { return mIsPageReset || (mPage != page); }

        std::optional<MyGUI::IntPoint> getAdjustedPos(int left, int top, bool move = false)
        {
            if (!mBook)
                return {};

            if (mPage >= mBook->mPages.size())
                return {};

            MyGUI::IntPoint pos(left, top);
            pos.left -= mCroppedParent->getAbsoluteLeft();
            pos.top -= mCroppedParent->getAbsoluteTop();
            pos.top += mViewTop;
            return pos;
        }

    public:
        typedef std::map<MyGUI::IFont*, std::unique_ptr<TextFormat>> ActiveTextFormats;

        int mViewTop;
        int mViewBottom;

        TypesetBookImpl::StyleImpl* mFocusItem;
        bool mItemActive;
        MyGUI::MouseButton mLastDown;
        std::function<void(TypesetBook::InteractiveId)> mLinkClicked;

        std::shared_ptr<TypesetBookImpl> mBook;

        MyGUI::ILayerNode* mNode;
        ActiveTextFormats mActiveTextFormats;

        PageDisplay()
        {
            resetPage();
            mViewTop = 0;
            mViewBottom = 0;
            mFocusItem = nullptr;
            mItemActive = false;
            mNode = nullptr;
        }

        void dirtyFocusItem()
        {
            if (mFocusItem != nullptr)
            {
                MyGUI::IFont* const font = mBook->affectedFont(mFocusItem);

                ActiveTextFormats::iterator i = mActiveTextFormats.find(font);

                if (mNode && i != mActiveTextFormats.end())
                    mNode->outOfDate(i->second->mRenderItem);
            }
        }

        void onMouseLostFocus()
        {
            if (!mBook)
                return;

            if (mPage >= mBook->mPages.size())
                return;

            dirtyFocusItem();

            mFocusItem = nullptr;
            mItemActive = false;
        }

        void onMouseMove(int left, int top)
        {
            TypesetBookImpl::StyleImpl* hit = nullptr;
            if (auto pos = getAdjustedPos(left, top, true))
                if (pos->top <= mViewBottom)
                    hit = mBook->hitTestWithMargin(pos->left, pos->top);

            if (mLastDown == MyGUI::MouseButton::None)
            {
                if (hit != mFocusItem)
                {
                    dirtyFocusItem();

                    mFocusItem = hit;
                    mItemActive = false;

                    dirtyFocusItem();
                }
            }
            else if (mFocusItem != nullptr)
            {
                bool newItemActive = hit == mFocusItem;

                if (newItemActive != mItemActive)
                {
                    mItemActive = newItemActive;

                    dirtyFocusItem();
                }
            }
        }

        void onMouseButtonPressed(int left, int top, MyGUI::MouseButton id)
        {
            auto pos = getAdjustedPos(left, top);

            if (pos && mLastDown == MyGUI::MouseButton::None)
            {
                mFocusItem = pos->top <= mViewBottom ? mBook->hitTestWithMargin(pos->left, pos->top) : nullptr;
                mItemActive = true;

                dirtyFocusItem();

                mLastDown = id;
            }
        }

        void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id)
        {
            auto pos = getAdjustedPos(left, top);

            if (pos && mLastDown == id)
            {
                TypesetBookImpl::StyleImpl* item
                    = pos->top <= mViewBottom ? mBook->hitTestWithMargin(pos->left, pos->top) : nullptr;

                bool clicked = mFocusItem == item;

                mItemActive = false;

                dirtyFocusItem();

                mLastDown = MyGUI::MouseButton::None;

                if (clicked && mLinkClicked && item && item->mInteractiveId != 0)
                    mLinkClicked(item->mInteractiveId);
            }
        }

        void showPage(std::shared_ptr<TypesetBook> book, size_t newPage)
        {
            std::shared_ptr<TypesetBookImpl> newBook = std::dynamic_pointer_cast<TypesetBookImpl>(book);

            if (mBook != newBook)
            {
                mFocusItem = nullptr;
                mItemActive = 0;

                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                {
                    if (mNode != nullptr && i->second != nullptr)
                        i->second->destroyDrawItem(mNode);
                    i->second.reset();
                }

                mActiveTextFormats.clear();

                if (newBook != nullptr)
                {
                    createActiveFormats(newBook);

                    mBook = std::move(newBook);
                    setPage(newPage);

                    if (newPage < mBook->mPages.size())
                    {
                        mViewTop = mBook->mPages[newPage].first;
                        mViewBottom = mBook->mPages[newPage].second;
                    }
                    else
                    {
                        mViewTop = 0;
                        mViewBottom = 0;
                    }
                }
                else
                {
                    mBook.reset();
                    resetPage();
                    mViewTop = 0;
                    mViewBottom = 0;
                }
            }
            else if (mBook && isPageDifferent(newPage))
            {
                if (mNode != nullptr)
                    for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                        mNode->outOfDate(i->second->mRenderItem);

                setPage(newPage);

                if (newPage < mBook->mPages.size())
                {
                    mViewTop = mBook->mPages[newPage].first;
                    mViewBottom = mBook->mPages[newPage].second;
                }
                else
                {
                    mViewTop = 0;
                    mViewBottom = 0;
                }
            }
        }

        struct CreateActiveFormat
        {
            PageDisplay* mPageDisplay;

            explicit CreateActiveFormat(PageDisplay* pageDisplay)
                : mPageDisplay(pageDisplay)
            {
            }

            void operator()(const TypesetBookImpl::Section& section, const TypesetBookImpl::Line& line,
                const TypesetBookImpl::Run& run) const
            {
                MyGUI::IFont* const font = run.mStyle->mFont;

                ActiveTextFormats::iterator j = mPageDisplay->mActiveTextFormats.find(font);

                if (j == mPageDisplay->mActiveTextFormats.end())
                {
                    auto textFormat = std::make_unique<TextFormat>(font, mPageDisplay);

                    textFormat->mTexture = font->getTextureFont();

                    j = mPageDisplay->mActiveTextFormats.insert(std::make_pair(font, std::move(textFormat))).first;
                }

                j->second->mCountVertex += run.mPrintableChars * 6;
            }
        };

        void createActiveFormats(std::shared_ptr<TypesetBookImpl> newBook)
        {
            newBook->visitRuns(0, 0x7FFFFFFF, CreateActiveFormat(this));

            if (mNode != nullptr)
                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                    i->second->createDrawItem(mNode);
        }

        void setVisible(bool newVisible) override
        {
            if (mVisible == newVisible)
                return;

            mVisible = newVisible;

            if (mVisible)
            {
                // reset input state
                mLastDown = MyGUI::MouseButton::None;
                mFocusItem = nullptr;
                mItemActive = 0;
            }

            if (nullptr != mNode)
            {
                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                    mNode->outOfDate(i->second->mRenderItem);
            }
        }

        void createDrawItem(MyGUI::ITexture* texture, MyGUI::ILayerNode* node) override
        {
            mNode = node;

            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                i->second->createDrawItem(node);
        }

        struct RenderRun
        {
            PageDisplay* mPageDisplay;
            GlyphStream& mGlyphStream;

            explicit RenderRun(PageDisplay* pageDisplay, GlyphStream& glyphStream)
                : mPageDisplay(pageDisplay)
                , mGlyphStream(glyphStream)
            {
            }

            void operator()(const TypesetBookImpl::Section& section, const TypesetBookImpl::Line& line,
                const TypesetBookImpl::Run& run) const
            {
                bool isActive = run.mStyle->mInteractiveId && (run.mStyle == mPageDisplay->mFocusItem);

                MyGUI::Colour colour = isActive
                    ? (mPageDisplay->mItemActive ? run.mStyle->mActiveColour : run.mStyle->mHotColour)
                    : run.mStyle->mNormalColour;

                mGlyphStream.reset(static_cast<float>(section.mRect.left + line.mRect.left + run.mLeft),
                    static_cast<float>(line.mRect.top), colour);

                Utf8Stream stream(run.mRange);

                while (!stream.eof())
                {
                    const Utf8Stream::UnicodeChar codePoint = stream.consume();

                    if (ucsCarriageReturn(codePoint))
                        continue;

                    if (!ucsSpace(codePoint))
                        mGlyphStream.emitGlyph(codePoint);
                    else
                        mGlyphStream.emitSpace(codePoint);
                }
            }
        };

        /*
            queue up rendering operations for this text format
        */
        void doRender(TextFormat& textFormat)
        {
            if (!mVisible)
                return;

            MyGUI::Vertex* vertices = textFormat.mRenderItem->getCurrentVertexBuffer();

            RenderXform renderXform(mCroppedParent, textFormat.mRenderItem->getRenderTarget()->getInfo());

            float z = SceneUtil::AutoDepth::isReversed() ? 1.f : -1.f;

            GlyphStream glyphStream(textFormat.mFont, static_cast<float>(mCoord.left),
                static_cast<float>(mCoord.top - mViewTop), z /*mNode->getNodeDepth()*/, vertices, renderXform);

            const int visitTop = std::max(mViewTop, mViewTop + static_cast<int>(renderXform.clipTop));
            const int visitBottom = std::min(mViewBottom, mViewTop + static_cast<int>(renderXform.clipBottom));

            mBook->visitRuns(visitTop, visitBottom, textFormat.mFont, RenderRun(this, glyphStream));

            textFormat.mRenderItem->setLastVertexCount(glyphStream.end() - vertices);
        }

        // ISubWidget should not necessarily be a drawitem
        // in this case, it is not...
        void doRender() override {}

        void _updateView() override
        {
            _checkMargin();

            if (mNode != nullptr)
                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                    mNode->outOfDate(i->second->mRenderItem);
        }

        void _correctView() override
        {
            _checkMargin();

            if (mNode != nullptr)
                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                    mNode->outOfDate(i->second->mRenderItem);
        }

        void destroyDrawItem() override
        {
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin(); i != mActiveTextFormats.end(); ++i)
                i->second->destroyDrawItem(mNode);

            mNode = nullptr;
        }
    };

    class BookPageImpl final : public BookPage
    {
        MYGUI_RTTI_DERIVED(BookPage)
    public:
        BookPageImpl()
            : mPageDisplay(nullptr)
        {
        }

        void showPage(std::shared_ptr<TypesetBook> book, size_t page) override
        {
            mPageDisplay->showPage(std::move(book), page);
        }

        void adviseLinkClicked(std::function<void(TypesetBook::InteractiveId)> linkClicked) override
        {
            mPageDisplay->mLinkClicked = std::move(linkClicked);
        }

        void unadviseLinkClicked() override
        {
            mPageDisplay->mLinkClicked = std::function<void(TypesetBook::InteractiveId)>();
        }

        void setFocusItem(BookTypesetter::Style* itemStyle) override
        {
            mPageDisplay->mFocusItem = static_cast<TypesetBookImpl::StyleImpl*>(itemStyle);
            mPageDisplay->dirtyFocusItem();
        }

    protected:
        void initialiseOverride() override
        {
            Base::initialiseOverride();

            if (getSubWidgetText())
            {
                mPageDisplay = getSubWidgetText()->castType<PageDisplay>();
            }
            else
            {
                throw std::runtime_error("BookPage unable to find page display sub widget");
            }
        }

        void onMouseLostFocus(MyGUI::Widget* /*newWidget*/) override
        {
            // NOTE: MyGUI also fires eventMouseLostFocus for widgets that are about to be destroyed (if they had
            // focus). Child widgets may already be destroyed! So be careful.
            mPageDisplay->onMouseLostFocus();
        }

        void onMouseMove(int left, int top) override { mPageDisplay->onMouseMove(left, top); }

        void onMouseButtonPressed(int left, int top, MyGUI::MouseButton id) override
        {
            mPageDisplay->onMouseButtonPressed(left, top, id);
        }

        void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id) override
        {
            mPageDisplay->onMouseButtonReleased(left, top, id);
        }

        PageDisplay* mPageDisplay;
    };

    void BookPage::registerMyGUIComponents()
    {
        MyGUI::FactoryManager& factory = MyGUI::FactoryManager::getInstance();

        factory.registerFactory<BookPageImpl>("Widget");
        factory.registerFactory<PageDisplay>("BasisSkin");
    }
}
