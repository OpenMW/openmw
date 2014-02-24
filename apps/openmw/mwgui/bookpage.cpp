#include "bookpage.hpp"

#include "MyGUI_FontManager.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_TextureUtility.h"
#include "MyGUI_FactoryManager.h"

#include <platform/stdint.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <components/misc/utf8stream.hpp>

namespace MWGui
{
struct TypesetBookImpl;
struct PageDisplay;
struct BookPageImpl;

static bool ucsSpace (int codePoint);
static bool ucsLineBreak (int codePoint);
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

        bool match (MyGUI::IFont* tstFont, MyGUI::Colour tstHotColour, MyGUI::Colour tstActiveColour,
                    MyGUI::Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return (mFont == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool match (char const * tstFont, MyGUI::Colour tstHotColour, MyGUI::Colour tstActiveColour,
                    MyGUI::Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return (mFont->getResourceName ()   == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool partal_match (MyGUI::Colour tstHotColour, MyGUI::Colour tstActiveColour,
                           MyGUI::Colour tstNormalColour, intptr_t tstInteractiveId)
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
            return Range (Utf8Point (NULL), Utf8Point (NULL));

        Utf8Point begin = &i->front ();
        Utf8Point end   = &i->front () + i->size ();

        return Range (begin, end);
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
        visitRuns (top, bottom, NULL, visitor);
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
        return NULL;
    }

    struct Typesetter;
};

struct TypesetBookImpl::Typesetter : BookTypesetter
{
    typedef TypesetBookImpl Book;
    typedef boost::shared_ptr <Book> BookPtr;

    int mPageWidth;
    int mPageHeight;

    BookPtr mBook;
    Section * mSection;
    Line * mLine;
    Run * mRun;

    std::vector <Alignment> mSectionAlignment;

    Book::Content const * mCurrentContent;
    Alignment mCurrentAlignment;

    Typesetter (size_t width, size_t height) :
        mPageWidth (width), mPageHeight(height),
        mSection (NULL), mLine (NULL), mRun (NULL),
        mCurrentAlignment (AlignLeft),
        mCurrentContent (NULL)
    {
        mBook = boost::make_shared <Book> ();
    }

    virtual ~Typesetter ()
    {
    }

    Style * createStyle (char const * fontName, Colour fontColour)
    {
        if (strcmp(fontName, "") == 0)
            return createStyle(MyGUI::FontManager::getInstance().getDefaultFont().c_str(), fontColour);

        for (Styles::iterator i = mBook->mStyles.begin (); i != mBook->mStyles.end (); ++i)
            if (i->match (fontName, fontColour, fontColour, fontColour, 0))
                return &*i;

        StyleImpl & style = *mBook->mStyles.insert (mBook->mStyles.end (), StyleImpl ());

        style.mFont = MyGUI::FontManager::getInstance().getByName(fontName);
        style.mHotColour = fontColour;
        style.mActiveColour = fontColour;
        style.mNormalColour = fontColour;
        style.mInteractiveId = 0;
                
        return &style;
    }

    Style* createHotStyle (Style* baseStyle, Colour normalColour, Colour hoverColour, Colour activeColour, InteractiveId id, bool unique)
    {
        StyleImpl* BaseStyle = dynamic_cast <StyleImpl*> (baseStyle);

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

        writeImpl (dynamic_cast <StyleImpl*> (style), range.first, range.second);
    }

    intptr_t addContent (Utf8Span text, bool select)
    {
        Contents::iterator i = mBook->mContents.insert (mBook->mContents.end (), Content (text.first, text.second));

        if (select)
            mCurrentContent = &(*i);

        return reinterpret_cast <intptr_t> (&(*i));
    }

    void selectContent (intptr_t contentHandle)
    {
        mCurrentContent = reinterpret_cast <Content const *> (contentHandle);
    }

    void write (Style * style, size_t begin, size_t end)
    {
        assert (mCurrentContent != NULL);
        assert (end <= mCurrentContent->size ());
        assert (begin <= mCurrentContent->size ());

        Utf8Point begin_ = &mCurrentContent->front () + begin;
        Utf8Point end_   = &mCurrentContent->front () + end  ;

        writeImpl (dynamic_cast <StyleImpl*> (style), begin_, end_);
    }
    
    void lineBreak (float margin)
    {
        assert (margin == 0); //TODO: figure out proper behavior here...

        mRun = NULL;
        mLine = NULL;
    }
    
    void sectionBreak (float margin)
    {
        if (mBook->mSections.size () > 0)
        {
            mRun = NULL;
            mLine = NULL;
            mSection = NULL;

            if (mBook->mRect.bottom < (mBook->mSections.back ().mRect.bottom + margin))
                mBook->mRect.bottom = (mBook->mSections.back ().mRect.bottom + margin);
        }
    }

    void setSectionAlignment (Alignment sectionAlignment)
    {
        if (mSection != NULL)
            mSectionAlignment.back () = sectionAlignment;
        mCurrentAlignment = sectionAlignment;
    }

    TypesetBook::Ptr complete ()
    {
        int curPageStart = 0;
        int curPageStop  = 0;

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

            if (sectionHeight <= mPageHeight)
            {
                if (sectionHeight > spaceLeft)
                {
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
                //split section
            }
        }

        if (curPageStart != curPageStop)
            mBook->mPages.push_back (Page (curPageStart, curPageStop));

        return mBook;
    }

    void writeImpl (StyleImpl * style, Utf8Stream::Point _begin, Utf8Stream::Point _end)
    {
        int line_height = style->mFont->getDefaultHeight ();

        Utf8Stream stream (_begin, _end);

        while (!stream.eof ())
        {
            if (ucsLineBreak (stream.peek ()))
            {
                stream.consume ();
                mLine = NULL, mRun = NULL;
                continue;
            }

            int word_width = 0;
            int word_height = 0;
            int space_width = 0;
            int character_count = 0;

            Utf8Stream::Point lead = stream.current ();

            while (!stream.eof () && !ucsLineBreak (stream.peek ()) && ucsBreakingSpace (stream.peek ()))
            {
                MyGUI::GlyphInfo* gi = style->mFont->getGlyphInfo (stream.peek ());
                if (gi)
                    space_width += gi->advance + gi->bearingX;
                stream.consume ();
            }

            Utf8Stream::Point origin = stream.current ();

            while (!stream.eof () && !ucsLineBreak (stream.peek ()) && !ucsBreakingSpace (stream.peek ()))
            {
                MyGUI::GlyphInfo* gi = style->mFont->getGlyphInfo (stream.peek ());
                if (gi)
                    word_width += gi->advance + gi->bearingX;
                word_height = line_height;
                ++character_count;
                stream.consume ();
            }

            Utf8Stream::Point extent = stream.current ();

            if (lead == extent)
                break;

            int left = mLine ? mLine->mRect.right : 0;

            if (left + space_width + word_width > mPageWidth)
            {
                mLine = NULL, mRun = NULL;

                append_run (style, origin, extent, extent - origin, word_width, mBook->mRect.bottom + word_height);
            }
            else
            {
                int top = mLine ? mLine->mRect.top : mBook->mRect.bottom;

                append_run (style, lead, extent, extent - origin, left + space_width + word_width, top + word_height);
            }
        }
    }

    void append_run (StyleImpl * style, Utf8Stream::Point begin, Utf8Stream::Point end, int pc, int right, int bottom)
    {
        if (mSection == NULL)
        {
            mBook->mSections.push_back (Section ());
            mSection = &mBook->mSections.back ();
            mSection->mRect = MyGUI::IntRect (0, mBook->mRect.bottom, 0, mBook->mRect.bottom);
            mSectionAlignment.push_back (mCurrentAlignment);
        }

        if (mLine == NULL)
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

        if (mRun == NULL || mRun->mStyle != style || mRun->mRange.second != begin)
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
    return boost::make_shared <TypesetBookImpl::Typesetter> (pageWidth, pageHeight);
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
            clipTop    = croppedParent->_getMarginTop ();
            clipLeft   = croppedParent->_getMarginLeft ();
            clipRight  = croppedParent->getWidth () - croppedParent->_getMarginRight ();
            clipBottom = croppedParent->getHeight () - croppedParent->_getMarginBottom ();

            absoluteLeft = croppedParent->getAbsoluteLeft();
            absoluteTop  = croppedParent->getAbsoluteTop();
            leftOffset   = renderTargetInfo.leftOffset;
            topOffset    = renderTargetInfo.topOffset;

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
            mZ(Z), mOrigin (left, top),
            mFont (font), mVertices (vertices),
            mRenderXform (renderXform)
        {
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
            MyGUI::GlyphInfo* gi = mFont->getGlyphInfo (ch);

            if (!gi)
                return;

            MyGUI::FloatRect vr;

            vr.left = mCursor.left + gi->bearingX;
            vr.top = mCursor.top + gi->bearingY;
            vr.right = vr.left + gi->width;
            vr.bottom = vr.top + gi->height;

            MyGUI::FloatRect tr = gi->uvRect;

            if (mRenderXform.clip (vr, tr))
                quad (vr, tr);

            mCursor.left += gi->bearingX + gi->advance;
        }

        void emitSpace (wchar_t ch)
        {
            MyGUI::GlyphInfo* gi = mFont->getGlyphInfo (ch);

            if (gi)
                mCursor.left += gi->bearingX + gi->advance;
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

class PageDisplay : public MyGUI::ISubWidgetText
{
    MYGUI_RTTI_DERIVED(PageDisplay)
protected:

    typedef TypesetBookImpl::Section Section;
    typedef TypesetBookImpl::Line    Line;
    typedef TypesetBookImpl::Run     Run;

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
            mTexture (NULL),
            mRenderItem (NULL),
            mDisplay (display),
            mCountVertex (0)
        {
        }

        void createDrawItem (MyGUI::ILayerNode* node)
        {
            assert (mRenderItem == NULL);

            if (mTexture != NULL)
            {
                mRenderItem = node->addToRenderItem(mTexture, false, false);
                mRenderItem->addDrawItem(this, mCountVertex);
            }
        }

        void destroyDrawItem (MyGUI::ILayerNode* node)
        {
            assert (mTexture != NULL ? mRenderItem != NULL : mRenderItem == NULL);

            if (mTexture != NULL)
            {
                mRenderItem->removeDrawItem (this);
                mRenderItem = NULL;
            }
        }

        void doRender() { mDisplay->doRender (*this); }

        // this isn't really a sub-widget, its just a "drawitem" which
        // should have its own interface
        void createDrawItem(MyGUI::ITexture* _texture, MyGUI::ILayerNode* _node) {}
        void destroyDrawItem() {};
    };

public:

    typedef TypesetBookImpl::StyleImpl Style;
    typedef std::map <TextFormat::Id, TextFormat*> ActiveTextFormats;

    int mViewTop;
    int mViewBottom;

    Style* mFocusItem;
    bool mItemActive;
    MyGUI::MouseButton mLastDown;
    boost::function <void (intptr_t)> mLinkClicked;


    boost::shared_ptr <TypesetBookImpl> mBook;
    size_t mPage;

    MyGUI::ILayerNode* mNode;
    ActiveTextFormats mActiveTextFormats;

    PageDisplay ()
    {
        mPage = -1;
        mViewTop = 0;
        mViewBottom = 0;
        mFocusItem = NULL;
        mItemActive = false;
        mNode = NULL;
    }

    void dirtyFocusItem ()
    {
        if (mFocusItem != 0)
        {
            MyGUI::IFont* Font = mBook->affectedFont (mFocusItem);

            ActiveTextFormats::iterator i = mActiveTextFormats.find (Font);

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

        Style * Hit = mBook->hitTest (left, mViewTop + top);

        if (mLastDown == MyGUI::MouseButton::None)
        {
            if (Hit != mFocusItem)
            {
                dirtyFocusItem ();

                mFocusItem = Hit;
                mItemActive = false;

                dirtyFocusItem ();
            }
        }
        else
        if (mFocusItem != 0)
        {
            bool newItemActive = Hit == mFocusItem;

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

        left -= mCroppedParent->getAbsoluteLeft ();
        top  -= mCroppedParent->getAbsoluteTop  ();

        if (mLastDown == MyGUI::MouseButton::None)
        {
            mFocusItem = mBook->hitTest (left, mViewTop + top);
            mItemActive = true;

            dirtyFocusItem ();

            mLastDown = id;
        }
    }

    void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id)
    {
        if (!mBook)
            return;

        left -= mCroppedParent->getAbsoluteLeft ();
        top  -= mCroppedParent->getAbsoluteTop  ();

        if (mLastDown == id)
        {
            Style * mItem = mBook->hitTest (left, mViewTop + top);

            bool clicked = mFocusItem == mItem;

            mItemActive = false;

            dirtyFocusItem ();

            mLastDown = MyGUI::MouseButton::None;

            if (clicked && mLinkClicked && mItem && mItem->mInteractiveId != 0)
                mLinkClicked (mItem->mInteractiveId);
        }
    }

    void showPage (TypesetBook::Ptr book, size_t newPage)
    {
        boost::shared_ptr <TypesetBookImpl> newBook = boost::dynamic_pointer_cast <TypesetBookImpl> (book);

        if (mBook != newBook)
        {
            mFocusItem = nullptr;
            mItemActive = 0;

            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            {
                if (mNode != NULL)
                    i->second->destroyDrawItem (mNode);
                delete i->second;
            }

            mActiveTextFormats.clear ();

            if (newBook != NULL)
            {
                createActiveFormats (newBook);

                mBook = newBook;
                mPage = newPage;

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
                mPage = -1;
                mViewTop = 0;
                mViewBottom = 0;
            }
        }
        else
        if (mBook && mPage != newPage)
        {
            if (mNode != NULL)
                for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                    mNode->outOfDate(i->second->mRenderItem);

            mPage = newPage;

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
                TextFormat * textFormat = new TextFormat (Font, this_);

                textFormat->mTexture = Font->getTextureFont ();

                j = this_->mActiveTextFormats.insert (std::make_pair (Font, textFormat)).first;
            }

            j->second->mCountVertex += run.mPrintableChars * 6;
        }
    };

    void createActiveFormats (boost::shared_ptr <TypesetBookImpl> newBook)
    {
        newBook->visitRuns (0, 0x7FFFFFFF, CreateActiveFormat (this));

        if (mNode != NULL)
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                i->second->createDrawItem (mNode);
    }

    void setVisible (bool newVisible)
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

    void createDrawItem(MyGUI::ITexture* texture, MyGUI::ILayerNode* node)
    {
        //test ();

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

            glyphStream.reset (section.mRect.left + line.mRect.left + run.mLeft, line.mRect.top, colour);

            Utf8Stream stream (run.mRange);

            while (!stream.eof ())
            {
                Utf8Stream::UnicodeChar code_point = stream.consume ();

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

        GlyphStream glyphStream (textFormat.mFont, mCoord.left, mCoord.top-mViewTop,
                                  -1 /*mNode->getNodeDepth()*/, vertices, renderXform);

        int visit_top    = (std::max) (mViewTop,    mViewTop + int (renderXform.clipTop   ));
        int visit_bottom = (std::min) (mViewBottom, mViewTop + int (renderXform.clipBottom));

        mBook->visitRuns (visit_top, visit_bottom, textFormat.mFont, RenderRun (this, glyphStream));

        textFormat.mRenderItem->setLastVertexCount(glyphStream.end () - vertices);
    }

    // ISubWidget should not necessarily be a drawitem
    // in this case, it is not...
    void doRender() { }

    void _updateView ()
    {
    }

    void _correctView()
    {
        _checkMargin ();

        if (mNode != NULL)
            for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate (i->second->mRenderItem);

    }

    void destroyDrawItem()
    {
        for (ActiveTextFormats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->destroyDrawItem (mNode);

        mNode = NULL;
    }
};


class BookPageImpl : public BookPage
{
MYGUI_RTTI_DERIVED(BookPage)
public:

    void showPage (TypesetBook::Ptr book, size_t page)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
            pd->showPage (book, page);
        else
            throw std::runtime_error ("The main sub-widget for a BookPage must be a PageDisplay.");
    }

    void adviseLinkClicked (boost::function <void (InteractiveId)> linkClicked)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->mLinkClicked = linkClicked;
        }
    }

    void unadviseLinkClicked ()
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->mLinkClicked = boost::function <void (InteractiveId)> ();
        }
    }

protected:
    void onMouseLostFocus(Widget* _new)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseLostFocus ();
        }
        else
            Widget::onMouseLostFocus (_new);
    }

    void onMouseMove(int left, int top)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseMove (left, top);
        }
        else
            Widget::onMouseMove (left, top);
    }

    void onMouseButtonPressed (int left, int top, MyGUI::MouseButton id)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseButtonPressed (left, top, id);
        }
        else
            Widget::onMouseButtonPressed (left, top, id);
    }

    void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseButtonReleased (left, top, id);
        }
        else
            Widget::onMouseButtonReleased (left, top, id);
    }
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
