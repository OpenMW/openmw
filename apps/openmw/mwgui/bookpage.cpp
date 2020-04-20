#include "bookpage.hpp"

#include "MyGUI_RenderItem.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_TextureUtility.h"
#include "MyGUI_FactoryManager.h"

#include <components/misc/utf8stream.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
struct TypesetBookImpl;
class PageDisplay;
class BookPageImpl;

static bool ucsSpace (int codePoint);
static bool ucsLineBreak (int codePoint);
static bool ucsCarriageReturn (int codePoint);
static bool ucsBreakingSpace (int codePoint);

struct BookTypesetter::Style { virtual ~Style () {} };

struct TypesetBookImpl : TypesetBook
{
    typedef std::vector <uint8_t> Content;
    typedef std::list <Content> Contents;
    typedef Utf8Stream::Point Utf8Point;
    typedef std::pair <Utf8Point, Utf8Point> Range;

    struct StyleImpl : BookTypesetter::Style
    {
        MyGUI::IFont*         mFont;
        MyGUI::Colour         mHotColour;
        MyGUI::Colour         mActiveColour;
        MyGUI::Colour         mNormalColour;
        InteractiveId mInteractiveId;

        bool match (MyGUI::IFont* tstFont, const MyGUI::Colour& tstHotColour, const MyGUI::Colour& tstActiveColour,
                    const MyGUI::Colour& tstNormalColour, intptr_t tstInteractiveId)
        {
            return (mFont == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool match (char const * tstFont, const MyGUI::Colour& tstHotColour, const MyGUI::Colour& tstActiveColour,
                    const MyGUI::Colour& tstNormalColour, intptr_t tstInteractiveId)
        {
            return (mFont->getResourceName ()   == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool partal_match (const MyGUI::Colour& tstHotColour, const MyGUI::Colour& tstActiveColour,
                           const MyGUI::Colour& tstNormalColour, intptr_t tstInteractiveId)
        {
            return
                (mHotColour                  == tstHotColour     ) &&
                (mActiveColour               == tstActiveColour  ) &&
                (mNormalColour               == tstNormalColour  ) &&
                (mInteractiveId              == tstInteractiveId ) ;
        }
    };

    typedef std::list <StyleImpl> Styles;

    struct Run
    {
        StyleImpl*  mStyle;
        Range       mRange;
        int         mLeft, mRight;
        int         mPrintableChars;
    };

    typedef std::vector <Run> Runs;

    struct Line
    {
        Runs mRuns;
        MyGUI::IntRect mRect;
    };

    typedef std::vector <Line> Lines;

    struct Section
    {
        Lines mLines;
        MyGUI::IntRect mRect;
    };

    typedef std::vector <Section> Sections;

    // Holds "top" and "bottom" vertical coordinates in the source text.
    // A page is basically a "window" into a portion of the source text, similar to a ScrollView.
    typedef std::pair <int, int> Page;

    typedef std::vector <Page> Pages;

    Pages mPages;
    Sections mSections;
    Contents mContents;
    Styles mStyles;
    MyGUI::IntRect mRect;

    virtual ~TypesetBookImpl () {}

    Range addContent (BookTypesetter::Utf8Span text)
    {
        Contents::iterator i = mContents.insert (mContents.end (), Content (text.first, text.second));

        if (i->empty())
            return Range (Utf8Point (nullptr), Utf8Point (nullptr));

        return Range (i->data(), i->data() + i->size());
    }

    size_t pageCount () const { return mPages.size (); }

    std::pair <unsigned int, unsigned int> getSize () const
    {
        return std::make_pair (mRect.width (), mRect.height ());
    }

    template <typename Visitor>
    void visitRuns (int top, int bottom, MyGUI::IFont* Font, Visitor const & visitor) const
    {
        for (Sections::const_iterator i = mSections.begin (); i != mSections.end (); ++i)
        {
            if (top >= mRect.bottom || bottom <= i->mRect.top)
                continue;

            for (Lines::const_iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
            {
                if (top >= j->mRect.bottom || bottom <= j->mRect.top)
                    continue;

                for (Runs::const_iterator k = j->mRuns.begin (); k != j->mRuns.end (); ++k)
                    if (!Font || k->mStyle->mFont == Font)
                        visitor (*i, *j, *k);
            }
        }
    }

    template <typename Visitor>
    void visitRuns (int top, int bottom, Visitor const & visitor) const
    {
        visitRuns (top, bottom, nullptr, visitor);
    }

    /// hit test with a margin for error. only hits on interactive text fragments are reported.
    StyleImpl * hitTestWithMargin (int left, int top)
    {
        StyleImpl * hit = hitTest(left, top);
        if (hit && hit->mInteractiveId != 0)
            return hit;

        const int maxMargin = 10;
        for (int margin=1; margin < maxMargin; ++margin)
        {
            for (int i=0; i<4; ++i)
            {
                if (i==0)
                    hit = hitTest(left, top-margin);
                else if (i==1)
                    hit = hitTest(left, top+margin);
                else if (i==2)
                    hit = hitTest(left-margin, top);
                else
                    hit = hitTest(left+margin, top);

                if (hit && hit->mInteractiveId != 0)
                    return hit;
            }
        }
        return nullptr;
    }

    StyleImpl * hitTest (int left, int top) const
    {
        for (Sections::const_iterator i = mSections.begin (); i != mSections.end (); ++i)
        {
            if (top < i->mRect.top || top >= i->mRect.bottom)
                continue;

            int left1 = left - i->mRect.left;

            for (Lines::const_iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
            {
                if (top < j->mRect.top || top >= j->mRect.bottom)
                    continue;

                int left2 = left1 - j->mRect.left;

                for (Runs::const_iterator k = j->mRuns.begin (); k != j->mRuns.end (); ++k)
                {
                    if (left2 < k->mLeft || left2 >= k->mRight)
                        continue;

                    return k->mStyle;
                }
            }
        }

        return nullptr;
    }

    MyGUI::IFont* affectedFont (StyleImpl* style)
    {
        for (Styles::iterator i = mStyles.begin (); i != mStyles.end (); ++i)
            if (&*i == style)
                return i->mFont;
        return nullptr;
    }

    struct Typesetter;
};

struct TypesetBookImpl::Typesetter : BookTypesetter
{
    struct PartialText {
        StyleImpl *mStyle;
        Utf8Stream::Point mBegin;
        Utf8Stream::Point mEnd;
        int mWidth;

        PartialText( StyleImpl *style, Utf8Stream::Point begin, Utf8Stream::Point end, int width) :
            mStyle(style), mBegin(begin), mEnd(end), mWidth(width)
        {}
    };

    typedef TypesetBookImpl Book;
    typedef std::shared_ptr <Book> BookPtr;
    typedef std::vector<PartialText>::const_iterator PartialTextConstIterator;

    int mPageWidth;
    int mPageHeight;

    BookPtr mBook;
    Section * mSection;
    Line * mLine;
    Run * mRun;

    std::vector <Alignment> mSectionAlignment;
    std::vector <PartialText> mPartialWhitespace;
    std::vector <PartialText> mPartialWord;

    Book::Content const * mCurrentContent;
    Alignment mCurrentAlignment;

    Typesetter (size_t width, size_t height) :
        mPageWidth (width), mPageHeight(height),
        mSection (nullptr), mLine (nullptr), mRun (nullptr),
        mCurrentContent (nullptr),
        mCurrentAlignment (AlignLeft)
    {
        mBook = std::make_shared <Book> ();
    }

    virtual ~Typesetter ()
    {
    }

    Style * createStyle (const std::string& fontName, const Colour& fontColour, bool useBookFont)
    {
        std::string fullFontName;
        if (fontName.empty())
            fullFontName = MyGUI::FontManager::getInstance().getDefaultFont();
        else
            fullFontName = fontName;

        if (useBookFont)
            fullFontName = "Journalbook " + fullFontName;

        for (Styles::iterator i = mBook->mStyles.begin (); i != mBook->mStyles.end (); ++i)
            if (i->match (fullFontName.c_str(), fontColour, fontColour, fontColour, 0))
                return &*i;

        MyGUI::IFont* font = MyGUI::FontManager::getInstance().getByName(fullFontName);
        if (!font)
            throw std::runtime_error(std::string("can't find font ") + fullFontName);

        StyleImpl & style = *mBook->mStyles.insert (mBook->mStyles.end (), StyleImpl ());
        style.mFont = font;
        style.mHotColour = fontColour;
        style.mActiveColour = fontColour;
        style.mNormalColour = fontColour;
        style.mInteractiveId = 0;

        return &style;
    }

    Style* createHotStyle (Style* baseStyle, const Colour& normalColour, const Colour& hoverColour,
                           const Colour& activeColour, InteractiveId id, bool unique)
    {
        StyleImpl* BaseStyle = static_cast <StyleImpl*> (baseStyle);

        if (!unique)
            for (Styles::iterator i = mBook->mStyles.begin (); i != mBook->mStyles.end (); ++i)
                if (i->match (BaseStyle->mFont, hoverColour, activeColour, normalColour, id))
                    return &*i;

        StyleImpl & style = *mBook->mStyles.insert (mBook->mStyles.end (), StyleImpl ());

        style.mFont = BaseStyle->mFont;
        style.mHotColour = hoverColour;
        style.mActiveColour = activeColour;
        style.mNormalColour = normalColour;
        style.mInteractiveId = id;

        return &style;
    }

    void write (Style * style, Utf8Span text)
    {
        Range range = mBook->addContent (text);

        writeImpl (static_cast <StyleImpl*> (style), range.first, range.second);
    }

    intptr_t addContent (Utf8Span text, bool select)
    {
        add_partial_text();

        Contents::iterator i = mBook->mContents.insert (mBook->mContents.end (), Content (text.first, text.second));

        if (select)
            mCurrentContent = &(*i);

        return reinterpret_cast <intptr_t> (&(*i));
    }

    void selectContent (intptr_t contentHandle)
    {
        add_partial_text();

        mCurrentContent = reinterpret_cast <Content const *> (contentHandle);
    }

    void write (Style * style, size_t begin, size_t end)
    {
        assert (mCurrentContent != nullptr);
        assert (end <= mCurrentContent->size ());
        assert (begin <= mCurrentContent->size ());

        Utf8Point begin_ = mCurrentContent->data() + begin;
        Utf8Point end_   = mCurrentContent->data() + end;

        writeImpl (static_cast <StyleImpl*> (style), begin_, end_);
    }

    void lineBreak (float margin)
    {
        assert (margin == 0); //TODO: figure out proper behavior here...

        add_partial_text();

        mRun = nullptr;
        mLine = nullptr;
    }

    void sectionBreak (int margin)
    {
        add_partial_text();

        if (mBook->mSections.size () > 0)
        {
            mRun = nullptr;
            mLine = nullptr;
            mSection = nullptr;

            if (mBook->mRect.bottom < (mBook->mSections.back ().mRect.bottom + margin))
                mBook->mRect.bottom = (mBook->mSections.back ().mRect.bottom + margin);
        }
    }

    void setSectionAlignment (Alignment sectionAlignment)
    {
        add_partial_text();

        if (mSection != nullptr)
            mSectionAlignment.back () = sectionAlignment;
        mCurrentAlignment = sectionAlignment;
    }

    TypesetBook::Ptr complete ()
    {
        int curPageStart = 0;
        int curPageStop  = 0;

        add_partial_text();

        std::vector <Alignment>::iterator sa = mSectionAlignment.begin ();
        for (Sections::iterator i = mBook->mSections.begin (); i != mBook->mSections.end (); ++i, ++sa)
        {
            // apply alignment to individual lines...
            for (Lines::iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
            {
                int width = j->mRect.width ();
                int excess = mPageWidth - width;

                switch (*sa)
                {
                default:
                case AlignLeft:   j->mRect.left = 0;        break;
                case AlignCenter: j->mRect.left = excess/2; break;
                case AlignRight:  j->mRect.left = excess;   break;
                }

                j->mRect.right = j->mRect.left + width;
            }

            if (curPageStop == curPageStart)
            {
                curPageStart = i->mRect.top;
                curPageStop  = i->mRect.top;
            }

            int spaceLeft = mPageHeight - (curPageStop - curPageStart);
            int sectionHeight = i->mRect.height ();

            // This is NOT equal to i->mRect.height(), which doesn't account for section breaks.
            int spaceRequired = (i->mRect.bottom - curPageStop);
            if (curPageStart == curPageStop) // If this is a new page, the section break is not needed
                spaceRequired = i->mRect.height();

            if (spaceRequired <= mPageHeight)
            {
                if (spaceRequired > spaceLeft)
                {
                    // The section won't completely fit on the current page. Finish the current page and start a new one.
                    assert (curPageStart != curPageStop);

                    mBook->mPages.push_back (Page (curPageStart, curPageStop));

                    curPageStart = i->mRect.top;
                    curPageStop = i->mRect.bottom;
                }
                else
                    curPageStop = i->mRect.bottom;
            }
            else
            {
                // The section won't completely fit on the current page. Finish the current page and start a new one.
                mBook->mPages.push_back (Page (curPageStart, curPageStop));

                curPageStart = i->mRect.top;
                curPageStop = i->mRect.bottom;

                //split section
                int sectionHeightLeft = sectionHeight;
                while (sectionHeightLeft >= mPageHeight)
                {
                    // Adjust to the top of the first line that does not fit on the current page anymore
                    int splitPos = curPageStop;
                    for (Lines::iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
                    {
                        if (j->mRect.bottom > curPageStart + mPageHeight)
                        {
                            splitPos = j->mRect.top;
                            break;
                        }
                    }

                    mBook->mPages.push_back (Page (curPageStart, splitPos));
                    curPageStart = splitPos;
                    curPageStop = splitPos;

                    sectionHeightLeft = (i->mRect.bottom - splitPos);
                }
                curPageStop = i->mRect.bottom;
            }
        }

        if (curPageStart != curPageStop)
            mBook->mPages.push_back (Page (curPageStart, curPageStop));

        return mBook;
    }

    void writeImpl (StyleImpl * style, Utf8Stream::Point _begin, Utf8Stream::Point _end)
    {
        Utf8Stream stream (_begin, _end);

        while (!stream.eof ())
        {
            if (ucsLineBreak (stream.peek ()))
            {
                add_partial_text();
                stream.consume ();
                mLine = nullptr, mRun = nullptr;
                continue;
            }

            if (ucsBreakingSpace (stream.peek ()) && !mPartialWord.empty())
                add_partial_text();

            int word_width = 0;
            int space_width = 0;

            Utf8Stream::Point lead = stream.current ();

            while (!stream.eof () && !ucsLineBreak (stream.peek ()) && ucsBreakingSpace (stream.peek ()))
            {
                MWGui::GlyphInfo info = GlyphInfo(style->mFont, stream.peek());
                if (info.charFound)
                    space_width += static_cast<int>(info.advance + info.bearingX);
                stream.consume ();
            }

            Utf8Stream::Point origin = stream.current ();

            while (!stream.eof () && !ucsLineBreak (stream.peek ()) && !ucsBreakingSpace (stream.peek ()))
            {
                MWGui::GlyphInfo info = GlyphInfo(style->mFont, stream.peek());
                if (info.charFound)
                    word_width += static_cast<int>(info.advance + info.bearingX);
                stream.consume ();
            }

            Utf8Stream::Point extent = stream.current ();

            if (lead == extent)
                break;

            if ( lead != origin )
                mPartialWhitespace.push_back (PartialText (style, lead, origin, space_width));
            if ( origin != extent )
                mPartialWord.push_back (PartialText (style, origin, extent, word_width));
        }
    }

    void add_partial_text ()
    {
        if (mPartialWhitespace.empty() && mPartialWord.empty())
            return;

        int fontHeight = MWBase::Environment::get().getWindowManager()->getFontHeight();
        int space_width = 0;
        int word_width  = 0;

        for (PartialTextConstIterator i = mPartialWhitespace.begin (); i != mPartialWhitespace.end (); ++i)
            space_width += i->mWidth;
        for (PartialTextConstIterator i = mPartialWord.begin (); i != mPartialWord.end (); ++i)
            word_width += i->mWidth;

        int left = mLine ? mLine->mRect.right : 0;

        if (left + space_width + word_width > mPageWidth)
        {
            mLine = nullptr, mRun = nullptr, left = 0;
        }
        else
        {
            for (PartialTextConstIterator i = mPartialWhitespace.begin (); i != mPartialWhitespace.end (); ++i)
            {
                int top = mLine ? mLine->mRect.top : mBook->mRect.bottom;

                append_run ( i->mStyle, i->mBegin, i->mEnd, 0, left + i->mWidth, top + fontHeight);

                left = mLine->mRect.right;
            }
        }

        for (PartialTextConstIterator i = mPartialWord.begin (); i != mPartialWord.end (); ++i)
        {
            int top = mLine ? mLine->mRect.top : mBook->mRect.bottom;

            append_run (i->mStyle, i->mBegin, i->mEnd, i->mEnd - i->mBegin, left + i->mWidth, top + fontHeight);

            left = mLine->mRect.right;
        }

        mPartialWhitespace.clear();
        mPartialWord.clear();
    }

    void append_run (StyleImpl * style, Utf8Stream::Point begin, Utf8Stream::Point end, int pc, int right, int bottom)
    {
        if (mSection == nullptr)
        {
            mBook->mSections.push_back (Section ());
            mSection = &mBook->mSections.back ();
            mSection->mRect = MyGUI::IntRect (0, mBook->mRect.bottom, 0, mBook->mRect.bottom);
            mSectionAlignment.push_back (mCurrentAlignment);
        }

        if (mLine == nullptr)
        {
            mSection->mLines.push_back (Line ());
            mLine = &mSection->mLines.back ();
            mLine->mRect = MyGUI::IntRect (0, mSection->mRect.bottom, 0, mBook->mRect.bottom);
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

            mLine->mRuns.push_back (Run ());
            mRun = &mLine->mRuns.back ();
            mRun->mStyle = style;
            mRun->mLeft = left;
            mRun->mRight = right;
            mRun->mRange.first = begin;
            mRun->mRange.second = end;
            mRun->mPrintableChars = pc;
          //Run->Locale = Locale;
        }
        else
        {
            mRun->mRight = right;
            mRun->mRange.second = end;
            mRun->mPrintableChars += pc;
        }
    }
};

BookTypesetter::Ptr BookTypesetter::create (int pageWidth, int pageHeight)
{
    return std::make_shared <TypesetBookImpl::Typesetter> (pageWidth, pageHeight);
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

        RenderXform (MyGUI::ICroppedRectangle* croppedParent, MyGUI::RenderTargetInfo const & renderTargetInfo)
        {
            clipTop    = static_cast<float>(croppedParent->_getMarginTop());
            clipLeft   = static_cast<float>(croppedParent->_getMarginLeft ());
            clipRight  = static_cast<float>(croppedParent->getWidth () - croppedParent->_getMarginRight ());
            clipBottom = static_cast<float>(croppedParent->getHeight() - croppedParent->_getMarginBottom());

            absoluteLeft = static_cast<float>(croppedParent->getAbsoluteLeft());
            absoluteTop  = static_cast<float>(croppedParent->getAbsoluteTop());
            leftOffset   = static_cast<float>(renderTargetInfo.leftOffset);
            topOffset    = static_cast<float>(renderTargetInfo.topOffset);

            pixScaleX   = renderTargetInfo.pixScaleX;
            pixScaleY   = renderTargetInfo.pixScaleY;
            hOffset     = renderTargetInfo.hOffset;
            vOffset     = renderTargetInfo.vOffset;
        }

        bool clip (MyGUI::FloatRect & vr, MyGUI::FloatRect & tr)
        {
            if (vr.bottom <= clipTop    || vr.right  <= clipLeft   ||
                vr.left   >= clipRight  || vr.top    >= clipBottom )
                return false;

            if (vr.top < clipTop)
            {
                tr.top += tr.height () * (clipTop - vr.top) / vr.height ();
                vr.top = clipTop;
            }

            if (vr.left < clipLeft)
            {
                tr.left += tr.width () * (clipLeft - vr.left) / vr.width ();
                vr.left = clipLeft;
            }

            if (vr.right > clipRight)
            {
                tr.right -= tr.width () * (vr.right - clipRight) / vr.width ();
                vr.right = clipRight;
            }

            if (vr.bottom > clipBottom)
            {
                tr.bottom -= tr.height () * (vr.bottom - clipBottom) / vr.height ();
                vr.bottom = clipBottom;
            }

            return true;
        }

        MyGUI::FloatPoint operator () (MyGUI::FloatPoint pt)
        {
            pt.left = absoluteLeft - leftOffset + pt.left;
            pt.top  = absoluteTop  - topOffset  + pt.top;

            pt.left = +(((pixScaleX * pt.left + hOffset) * 2.0f) - 1.0f);
            pt.top  = -(((pixScaleY * pt.top  + vOffset) * 2.0f) - 1.0f);

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

        GlyphStream (MyGUI::IFont* font, float left, float top, float Z,
                      MyGUI::Vertex* vertices, RenderXform const & renderXform) :
            mZ(Z),
            mC(0), mFont (font), mOrigin (left, top),
            mVertices (vertices),
            mRenderXform (renderXform)
        {
            assert(font != nullptr);
            mVertexColourType = MyGUI::RenderManager::getInstance().getVertexFormat();
        }

        ~GlyphStream ()
        {
        }

        MyGUI::Vertex* end () const { return mVertices; }

        void reset (float left, float top, MyGUI::Colour colour)
        {
            mC = MyGUI::texture_utility::toColourARGB(colour) | 0xFF000000;
            MyGUI::texture_utility::convertColour(mC, mVertexColourType);

            mCursor.left = mOrigin.left + left;
            mCursor.top = mOrigin.top + top;
        }

        void emitGlyph (wchar_t ch)
        {
            MWGui::GlyphInfo info = GlyphInfo(mFont, ch);

            if (!info.charFound)
                return;

            MyGUI::FloatRect vr;

            vr.left = mCursor.left + info.bearingX;
            vr.top = mCursor.top + info.bearingY;
            vr.right = vr.left + info.width;
            vr.bottom = vr.top + info.height;

            MyGUI::FloatRect tr = info.uvRect;

            if (mRenderXform.clip (vr, tr))
                quad (vr, tr);

            mCursor.left += static_cast<int>(info.bearingX + info.advance);
        }

        void emitSpace (wchar_t ch)
        {
            MWGui::GlyphInfo info = GlyphInfo(mFont, ch);

            if (info.charFound)
                mCursor.left += static_cast<int>(info.bearingX + info.advance);
        }

    private:

        void quad (const MyGUI::FloatRect& vr, const MyGUI::FloatRect& tr)
        {
            vertex (vr.left, vr.top, tr.left, tr.top);
            vertex (vr.right, vr.top, tr.right, tr.top);
            vertex (vr.left, vr.bottom, tr.left, tr.bottom);
            vertex (vr.right, vr.top, tr.right, tr.top);
            vertex (vr.left, vr.bottom, tr.left, tr.bottom);
            vertex (vr.right, vr.bottom, tr.right, tr.bottom);
        }

        void vertex (float x, float y, float u, float v)
        {
            MyGUI::FloatPoint pt = mRenderXform (MyGUI::FloatPoint (x, y));

            mVertices->x = pt.left;
            mVertices->y = pt.top ;
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

    typedef TypesetBookImpl::Section Section;
    typedef TypesetBookImpl::Line    Line;
    typedef TypesetBookImpl::Run     Run;
    bool mIsPageReset;
    size_t mPage;

    struct TextFormat : ISubWidget
    {
        typedef MyGUI::IFont* Id;

        Id mFont;
        int mCountVertex;
        MyGUI::ITexture* mTexture;
        MyGUI::RenderItem* mRenderItem;
        PageDisplay * mDisplay;

        TextFormat (MyGUI::IFont* id, PageDisplay * display) :
            mFont (id),
            mCountVertex (0),
            mTexture (nullptr),
            mRenderItem (nullptr),
            mDisplay (display)
        {
        }

        void createDrawItem (MyGUI::ILayerNode* node)
        {
            assert (mRenderItem == nullptr);

            if (mTexture != nullptr)
            {
                mRenderItem = node->addToRenderItem(mTexture, false, false);
                mRenderItem->addDrawItem(this, mCountVertex);
            }
        }

        void destroyDrawItem (MyGUI::ILayerNode* node)
        {
            assert (mTexture != nullptr ? mRenderItem != nullptr : mRenderItem == nullptr);

            if (mTexture != nullptr)
            {
                mRenderItem->removeDrawItem (this);
                mRenderItem = nullptr;
            }
        }

        void doRender() { mDisplay->doRender (*this); }

        // this isn't really a sub-widget, its just a "drawitem" which
        // should have its own interface
        void createDrawItem(MyGUI::ITexture* _texture, MyGUI::ILayerNode* _node) {}
        void destroyDrawItem() {};
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

    bool isPageDifferent(size_t page)
    {
       return mIsPageReset || (mPage != page);
    }

public:

    typedef TypesetBookImpl::StyleImpl Style;
    typedef std::map <TextFormat::Id, std::unique_ptr<TextFormat>> ActiveTextFormats;

    int mViewTop;
    int mViewBottom;

    Style* mFocusItem;
    bool mItemActive;
    MyGUI::MouseButton mLastDown;
    std::function <void (intptr_t)> mLinkClicked;


    std::shared_ptr <TypesetBookImpl> mBook;

    MyGUI::ILayerNode* mNode;
    ActiveTextFormats mActiveTextFormats;

    PageDisplay ()
    {
        resetPage ();
        mViewTop = 0;
        mViewBottom = 0;
        mFocusItem = nullptr;
        mItemActive = false;
        mNode = nullptr;
    }

    void dirtyFocusItem ()
    {
        if (mFocusItem != 0)
        {
            MyGUI::IFont* Font = mBook->affectedFont (mFocusItem);

            ActiveTextFormats::iterator i = mActiveTextFormats.find (Font);

            if (mNode)
                mNode->outOfDate (i->second->mRenderItem);
        }
    }

    void onMouseLostFocus ()
    {
        if (!mBook)
            return;

        dirtyFocusItem ();

        mFocusItem = 0;
        mItemActive = false;
    }

    void onMouseMove (int left, int top)
    {
        if (!mBook)
            return;

        left -= mCroppedParent->getAbsoluteLeft ();
        top  -= mCroppedParent->getAbsoluteTop  ();

        Style * hit = mBook->hitTestWithMargin (left, mViewTop + top);

        if (mLastDown == MyGUI::MouseButton::None)
        {
            if (hit != mFocusItem)
            {
                dirtyFocusItem ();

                mFocusItem = hit;
                mItemActive = false;

                dirtyFocusItem ();
            }
        }
        else
        if (mFocusItem != 0)
        {
            bool newItemActive = hit == mFocusItem;

            if (newItemActive != mItemActive)
            {
                mItemActive = newItemActive;

                dirtyFocusItem ();
            }
        }
    }

    void onMouseButtonPressed (int left, int top, MyGUI::MouseButton id)
    {
        if (!mBook)
            return;

        // work around inconsistency in MyGUI where the mouse press coordinates aren't
        // transformed by the current Layer (even though mouse *move* events are).
        MyGUI::IntPoint pos (left, top);
#if MYGUI_VERSION < MYGUI_DEFINE_VERSION(3,2,3)
        pos = mNode->getLayer()->getPosition(left, top);
#endif
        pos.left -= mCroppedParent->getAbsoluteLeft ();
        pos.top  -= mCroppedParent->getAbsoluteTop  ();

        if (mLastDown == MyGUI::MouseButton::None)
        {
            mFocusItem = mBook->hitTestWithMargin (pos.left, mViewTop + pos.top);
            mItemActive = true;

            dirtyFocusItem ();

            mLastDown = id;
        }
    }

    void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id)
    {
        if (!mBook)
            return;

        // work around inconsistency in MyGUI where the mouse release coordinates aren't
        // transformed by the current Layer (even though mouse *move* events are).
        MyGUI::IntPoint pos (left, top);
#if MYGUI_VERSION < MYGUI_DEFINE_VERSION(3,2,3)
        pos = mNode->getLayer()->getPosition(left, top);
#endif

        pos.left -= mCroppedParent->getAbsoluteLeft ();
        pos.top  -= mCroppedParent->getAbsoluteTop  ();

        if (mLastDown == id)
        {
            Style * item = mBook->hitTestWithMargin (pos.left, mViewTop + pos.top);

            bool clicked = mFocusItem == item;

            mItemActive = false;

            dirtyFocusItem ();

            mLastDown = MyGUI::MouseButton::None;

            if (clicked && mLinkClicked && item && item->mInteractiveId != 0)
                mLinkClicked (item->mInteractiveId);
        }
    }

    void showPage (TypesetBook::Ptr book, size_t newPage)
    {
        std::shared_ptr <TypesetBookImpl> newBook = std::dynamic_pointer_cast <TypesetBookImpl> (book);

        if (mBook != newBook)
        {
            mFocusItem = nullptr;
            mItemActive = 0;

            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            {
                if (mNode != nullptr)
                    i->second->destroyDrawItem (mNode);
                i->second.reset();
            }

            mActiveTextFormats.clear ();

            if (newBook != nullptr)
            {
                createActiveFormats (newBook);

                mBook = newBook;
                setPage (newPage);

                if (newPage < mBook->mPages.size ())
                {
                    mViewTop = mBook->mPages [newPage].first;
                    mViewBottom = mBook->mPages [newPage].second;
                }
                else
                {
                    mViewTop = 0;
                    mViewBottom = 0;
                }
            }
            else
            {
                mBook.reset ();
                resetPage ();
                mViewTop = 0;
                mViewBottom = 0;
            }
        }
        else
        if (mBook && isPageDifferent (newPage))
        {
            if (mNode != nullptr)
                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                    mNode->outOfDate(i->second->mRenderItem);

            setPage (newPage);

            if (newPage < mBook->mPages.size ())
            {
                mViewTop = mBook->mPages [newPage].first;
                mViewBottom = mBook->mPages [newPage].second;
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
        PageDisplay * this_;

        CreateActiveFormat (PageDisplay * this_) : this_ (this_) {}

        void operator () (Section const & section, Line const & line, Run const & run) const
        {
            MyGUI::IFont* Font = run.mStyle->mFont;

            ActiveTextFormats::iterator j = this_->mActiveTextFormats.find (Font);

            if (j == this_->mActiveTextFormats.end ())
            {
                std::unique_ptr<TextFormat> textFormat(new TextFormat (Font, this_));

                textFormat->mTexture = Font->getTextureFont ();

                j = this_->mActiveTextFormats.insert (std::make_pair (Font, std::move(textFormat))).first;
            }

            j->second->mCountVertex += run.mPrintableChars * 6;
        }
    };

    void createActiveFormats (std::shared_ptr <TypesetBookImpl> newBook)
    {
        newBook->visitRuns (0, 0x7FFFFFFF, CreateActiveFormat (this));

        if (mNode != nullptr)
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                i->second->createDrawItem (mNode);
    }

    void setVisible (bool newVisible) final
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
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate(i->second->mRenderItem);
        }
    }

    void createDrawItem(MyGUI::ITexture* texture, MyGUI::ILayerNode* node) final
    {
        mNode = node;

        for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->createDrawItem (node);
    }

    struct RenderRun
    {
        PageDisplay * this_;
        GlyphStream &glyphStream;

        RenderRun (PageDisplay * this_, GlyphStream &glyphStream) :
            this_(this_), glyphStream (glyphStream)
        {
        }

        void operator () (Section const & section, Line const & line, Run const & run) const
        {
            bool isActive = run.mStyle->mInteractiveId && (run.mStyle == this_->mFocusItem);

            MyGUI::Colour colour = isActive ? (this_->mItemActive ? run.mStyle->mActiveColour: run.mStyle->mHotColour) : run.mStyle->mNormalColour;

            glyphStream.reset(static_cast<float>(section.mRect.left + line.mRect.left + run.mLeft), static_cast<float>(line.mRect.top), colour);

            Utf8Stream stream (run.mRange);

            while (!stream.eof ())
            {
                Utf8Stream::UnicodeChar code_point = stream.consume ();

                if (ucsCarriageReturn (code_point))
                    continue;

                if (!ucsSpace (code_point))
                    glyphStream.emitGlyph (code_point);
                else
                    glyphStream.emitSpace (code_point);
            }
        }
    };

    /*
        queue up rendering operations for this text format
    */
    void doRender(TextFormat & textFormat)
    {
        if (!mVisible)
            return;

        MyGUI::Vertex* vertices = textFormat.mRenderItem->getCurrentVertexBuffer();

        RenderXform renderXform (mCroppedParent, textFormat.mRenderItem->getRenderTarget()->getInfo());

        GlyphStream glyphStream(textFormat.mFont, static_cast<float>(mCoord.left), static_cast<float>(mCoord.top - mViewTop),
                                  -1 /*mNode->getNodeDepth()*/, vertices, renderXform);

        int visit_top    = (std::max) (mViewTop,    mViewTop + int (renderXform.clipTop   ));
        int visit_bottom = (std::min) (mViewBottom, mViewTop + int (renderXform.clipBottom));

        mBook->visitRuns (visit_top, visit_bottom, textFormat.mFont, RenderRun (this, glyphStream));

        textFormat.mRenderItem->setLastVertexCount(glyphStream.end () - vertices);
    }

    // ISubWidget should not necessarily be a drawitem
    // in this case, it is not...
    void doRender() final { }

    void _updateView () final
    {
        _checkMargin();

        if (mNode != nullptr)
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate (i->second->mRenderItem);
    }

    void _correctView() final
    {
        _checkMargin ();

        if (mNode != nullptr)
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate (i->second->mRenderItem);

    }

    void destroyDrawItem() final
    {
        for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->destroyDrawItem (mNode);

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

    void showPage (TypesetBook::Ptr book, size_t page) final
    {
        mPageDisplay->showPage (book, page);
    }

    void adviseLinkClicked (std::function <void (InteractiveId)> linkClicked) final
    {
        mPageDisplay->mLinkClicked = linkClicked;
    }

    void unadviseLinkClicked () final
    {
        mPageDisplay->mLinkClicked = std::function <void (InteractiveId)> ();
    }

protected:

    void initialiseOverride() final
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

    void onMouseLostFocus(Widget* _new) final
    {
        // NOTE: MyGUI also fires eventMouseLostFocus for widgets that are about to be destroyed (if they had focus).
        // Child widgets may already be destroyed! So be careful.
        mPageDisplay->onMouseLostFocus ();
    }

    void onMouseMove(int left, int top) final
    {
        mPageDisplay->onMouseMove (left, top);
    }

    void onMouseButtonPressed (int left, int top, MyGUI::MouseButton id) final
    {
        mPageDisplay->onMouseButtonPressed (left, top, id);
    }

    void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id) final
    {
        mPageDisplay->onMouseButtonReleased (left, top, id);
    }

    PageDisplay* mPageDisplay;
};

void BookPage::registerMyGUIComponents ()
{
    MyGUI::FactoryManager & factory = MyGUI::FactoryManager::getInstance();

    factory.registerFactory<BookPageImpl>("Widget");
    factory.registerFactory<PageDisplay>("BasisSkin");
}

static bool ucsLineBreak (int codePoint)
{
    return codePoint == '\n';
}

static bool ucsCarriageReturn (int codePoint)
{
    return codePoint == '\r';
}

static bool ucsSpace (int codePoint)
{
    switch (codePoint)
    {
    case 0x0020: // SPACE
    case 0x00A0: // NO-BREAK SPACE
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

static bool ucsBreakingSpace (int codePoint)
{
    switch (codePoint)
    {
    case 0x0020: // SPACE
  //case 0x00A0: // NO-BREAK SPACE
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
  //case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
        return true;
    default:
        return false;
    }
}

}
