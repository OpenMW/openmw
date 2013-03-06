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
}

using namespace MyGUI;
using namespace MWGui;

static bool ucs_space (int code_point);
static bool ucs_line_break (int code_point);
static bool ucs_breaking_space (int code_point);

struct BookTypesetter::Style { virtual ~Style () {} };

struct MWGui::TypesetBookImpl : TypesetBook
{
    typedef std::vector <uint8_t> content;
    typedef std::list <content> contents;
    typedef utf8_stream::point utf8_point;
    typedef std::pair <utf8_point, utf8_point> range;

    struct StyleImpl : BookTypesetter::Style
    {
        IFont*         mFont;
        Colour         mHotColour;
        Colour         mActiveColour;
        Colour         mNormalColour;
        interactive_id mInteractiveId;

        bool match (IFont* tstFont, Colour tstHotColour, Colour tstActiveColour, Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return (mFont == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool match (char const * tstFont, Colour tstHotColour, Colour tstActiveColour, Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return (mFont->getResourceName ()   == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool partal_match (Colour tstHotColour, Colour tstActiveColour, Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return
                (mHotColour                  == tstHotColour     ) &&
                (mActiveColour               == tstActiveColour  ) &&
                (mNormalColour               == tstNormalColour  ) &&
                (mInteractiveId              == tstInteractiveId ) ;
        }
    };

    typedef std::list <StyleImpl> styles;

    struct Run
    {
        StyleImpl*  mStyle;
        range       mRange;
        int         mLeft, mRight;
        int         mPrintableChars;
    };

    typedef std::vector <Run> runs;

    struct Line
    {
        runs mRuns;
        IntRect mRect;
    };

    typedef std::vector <Line> lines;

    struct Section
    {
        lines mLines;
        IntRect mRect;
    };

    typedef std::vector <Section> sections;

    typedef std::pair <int, int> page;
    typedef std::vector <page> pages;

    pages mPages;
    sections mSections;
    contents mContents;
    styles mStyles;
    IntRect mRect;

    virtual ~TypesetBookImpl () {}

    range addContent (BookTypesetter::utf8_span Text)
    {
        contents::iterator i = mContents.insert (mContents.end (), content (Text.first, Text.second));

        if (i->size () == 0)
            return range (utf8_point (NULL), utf8_point (NULL));

        utf8_point begin = &i->front ();
        utf8_point end   = &i->front () + i->size ();

        return range (begin, end);
    }

    size_t pageCount () const { return mPages.size (); }

    std::pair <int, int> getSize () const
    {
        return std::make_pair (mRect.width (), mRect.height ());
    }

    template <typename visitor>
    void visitRuns (int top, int bottom, IFont* Font, visitor const & Visitor) const
    {
        for (sections::const_iterator i = mSections.begin (); i != mSections.end (); ++i)
        {
            if (top >= mRect.bottom || bottom <= i->mRect.top)
                continue;

            for (lines::const_iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
            {
                if (top >= j->mRect.bottom || bottom <= j->mRect.top)
                    continue;

                for (runs::const_iterator k = j->mRuns.begin (); k != j->mRuns.end (); ++k)
                    if (!Font || k->mStyle->mFont == Font)
                        Visitor (*i, *j, *k);
            }
        }
    }

    template <typename visitor>
    void visitRuns (int top, int bottom, visitor const & Visitor) const
    {
        visitRuns (top, bottom, NULL, Visitor);
    }

    StyleImpl * hitTest (int left, int top) const
    {
        for (sections::const_iterator i = mSections.begin (); i != mSections.end (); ++i)
        {
            if (top < i->mRect.top || top >= i->mRect.bottom)
                continue;

            int left1 = left - i->mRect.left;

            for (lines::const_iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
            {
                if (top < j->mRect.top || top >= j->mRect.bottom)
                    continue;

                int left2 = left1 - j->mRect.left;

                for (runs::const_iterator k = j->mRuns.begin (); k != j->mRuns.end (); ++k)
                {
                    if (left2 < k->mLeft || left2 >= k->mRight)
                        continue;

                    return k->mStyle;
                }
            }
        }

        return nullptr;
    }

    IFont* affectedFont (StyleImpl* Style)
    {
        for (styles::iterator i = mStyles.begin (); i != mStyles.end (); ++i)
            if (&*i == Style)
                return i->mFont;
        return NULL;
    }

    struct Typesetter;
};

struct TypesetBookImpl::Typesetter : BookTypesetter
{
    typedef TypesetBookImpl book;
    typedef boost::shared_ptr <book> book_ptr;

    int mPageWidth;
    int mPageHeight;

    book_ptr mBook;
    Section * mSection;
    Line * mLine;
    Run * mRun;

    std::vector <alignment> mSectionAlignment;

    book::content const * mCurrentContent;
    alignment mCurrentAlignment;

    Typesetter (size_t Width, size_t Height) :
        mPageWidth (Width), mPageHeight(Height),
        mSection (NULL), mLine (NULL), mRun (NULL),
        mCurrentAlignment (alignLeft),
        mCurrentContent (NULL)
    {
        mBook = boost::make_shared <book> ();
    }

    virtual ~Typesetter ()
    {
    }

    Style * createStyle (char const * FontName, Colour FontColour)
    {
        for (styles::iterator i = mBook->mStyles.begin (); i != mBook->mStyles.end (); ++i)
            if (i->match (FontName, FontColour, FontColour, FontColour, 0))
                return &*i;

        StyleImpl & style = *mBook->mStyles.insert (mBook->mStyles.end (), StyleImpl ());

        style.mFont = FontManager::getInstance().getByName(FontName);
        style.mHotColour = FontColour;
        style.mActiveColour = FontColour;
        style.mNormalColour = FontColour;
        style.mInteractiveId = 0;
                
        return &style;
    }

    Style* createHotStyle (Style* _BaseStyle, coulour NormalColour, coulour HoverColour, coulour ActiveColour, interactive_id Id, bool Unique)
    {
        StyleImpl* BaseStyle = dynamic_cast <StyleImpl*> (_BaseStyle);

        if (!Unique)
            for (styles::iterator i = mBook->mStyles.begin (); i != mBook->mStyles.end (); ++i)
                if (i->match (BaseStyle->mFont, HoverColour, ActiveColour, NormalColour, Id))
                    return &*i;

        StyleImpl & style = *mBook->mStyles.insert (mBook->mStyles.end (), StyleImpl ());

        style.mFont = BaseStyle->mFont;
        style.mHotColour = HoverColour;
        style.mActiveColour = ActiveColour;
        style.mNormalColour = NormalColour;
        style.mInteractiveId = Id;

        return &style;
    }

    void write (Style * _Style, utf8_span Text)
    {
        range text = mBook->addContent (Text);

        write_impl (dynamic_cast <StyleImpl*> (_Style), text.first, text.second);
    }

    intptr_t add_content (utf8_span Text, bool Select)
    {
        contents::iterator i = mBook->mContents.insert (mBook->mContents.end (), content (Text.first, Text.second));

        if (Select)
            mCurrentContent = &(*i);

        return reinterpret_cast <intptr_t> (&(*i));
    }

    void select_content (intptr_t contentHandle)
    {
        mCurrentContent = reinterpret_cast <content const *> (contentHandle);
    }

    void write (Style * style, size_t begin, size_t end)
    {
        assert (mCurrentContent != NULL);
        assert (end <= mCurrentContent->size ());
        assert (begin <= mCurrentContent->size ());

        utf8_point begin_ = &mCurrentContent->front () + begin;
        utf8_point end_   = &mCurrentContent->front () + end  ;

        write_impl (dynamic_cast <StyleImpl*> (style), begin_, end_);
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

    void setSectionAlignment (alignment sectionAlignment)
    {
        if (mSection != NULL)
            mSectionAlignment.back () = sectionAlignment;
        mCurrentAlignment = sectionAlignment;
    }

    TypesetBook::ptr complete ()
    {
        int curPageStart = 0;
        int curPageStop  = 0;

        std::vector <alignment>::iterator sa = mSectionAlignment.begin ();
        for (sections::iterator i = mBook->mSections.begin (); i != mBook->mSections.end (); ++i, ++sa)
        {
            // apply alignment to individual lines...
            for (lines::iterator j = i->mLines.begin (); j != i->mLines.end (); ++j)
            {
                int width = j->mRect.width ();
                int excess = mPageWidth - width;

                switch (*sa)
                {
                default:
                case alignLeft:   j->mRect.left = 0;        break;
                case alignCenter: j->mRect.left = excess/2; break;
                case alignRight:  j->mRect.left = excess;   break;
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

                    mBook->mPages.push_back (page (curPageStart, curPageStop));

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
            mBook->mPages.push_back (page (curPageStart, curPageStop));

        return mBook;
    }

    void write_impl (StyleImpl * Style, utf8_stream::point _begin, utf8_stream::point _end)
    {
        int line_height = Style->mFont->getDefaultHeight ();

        utf8_stream stream (_begin, _end);

        while (!stream.eof ())
        {
            if (ucs_line_break (stream.peek ()))
            {
                stream.consume ();
                mLine = NULL, mRun = NULL;
                continue;
            }

            int word_width = 0;
            int word_height = 0;
            int space_width = 0;
            int character_count = 0;

            utf8_stream::point lead = stream.current ();

            while (!stream.eof () && !ucs_line_break (stream.peek ()) && ucs_breaking_space (stream.peek ()))
            {
                GlyphInfo* gi = Style->mFont->getGlyphInfo (stream.peek ());
                space_width += gi->advance;
                stream.consume ();
            }

            utf8_stream::point origin = stream.current ();

            while (!stream.eof () && !ucs_line_break (stream.peek ()) && !ucs_breaking_space (stream.peek ()))
            {
                GlyphInfo* gi = Style->mFont->getGlyphInfo (stream.peek ());
                word_width += gi->advance + gi->bearingX;
                word_height = line_height;
                ++character_count;
                stream.consume ();
            }

            utf8_stream::point extent = stream.current ();

            if (lead == extent)
                break;

            int left = mLine ? mLine->mRect.right : 0;

            if (left + space_width + word_width > mPageWidth)
            {
                mLine = NULL, mRun = NULL;

                append_run (Style, origin, extent, extent - origin, word_width, mBook->mRect.bottom + word_height);
            }
            else
            {
                int top = mLine ? mLine->mRect.top : mBook->mRect.bottom;

                append_run (Style, lead, extent, extent - origin, left + space_width + word_width, top + word_height);
            }
        }
    }

    void append_run (StyleImpl * style, utf8_stream::point begin, utf8_stream::point end, int pc, int right, int bottom)
    {
        if (mSection == NULL)
        {
            mBook->mSections.push_back (Section ());
            mSection = &mBook->mSections.back ();
            mSection->mRect = IntRect (0, mBook->mRect.bottom, 0, mBook->mRect.bottom);
            mSectionAlignment.push_back (mCurrentAlignment);
        }

        if (mLine == NULL)
        {
            mSection->mLines.push_back (Line ());
            mLine = &mSection->mLines.back ();
            mLine->mRect = IntRect (0, mSection->mRect.bottom, 0, mBook->mRect.bottom);
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

BookTypesetter::ptr BookTypesetter::create (int pageWidth, int pageHeight)
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

        RenderXform (ICroppedRectangle* croppedParent, RenderTargetInfo const & renderTargetInfo)
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

        bool clip (FloatRect & vr, FloatRect & tr)
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

        FloatPoint operator () (FloatPoint pt)
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
        IFont* mFont;
        FloatPoint mOrigin;
        FloatPoint mCursor;
        Vertex* mVertices;
        RenderXform mRenderXform;
        MyGUI::VertexColourType mVertexColourType;

        GlyphStream (IFont* font, float left, float top, float Z,
                      Vertex* vertices, RenderXform const & renderXform) :
            mZ(Z), mOrigin (left, top),
            mFont (font), mVertices (vertices),
            mRenderXform (renderXform)
        {
            mVertexColourType = RenderManager::getInstance().getVertexFormat();
        }

        ~GlyphStream ()
        {
        }

        Vertex* end () const { return mVertices; }

        void reset (float left, float top, Colour colour)
        {
            mC = texture_utility::toColourARGB(colour) | 0xFF000000;
            texture_utility::convertColour(mC, mVertexColourType);

            mCursor.left = mOrigin.left + left;
            mCursor.top = mOrigin.top + top;
        }

        void emitGlyph (wchar_t ch)
        {
            GlyphInfo* gi = mFont->getGlyphInfo (ch);

            FloatRect vr;

            vr.left = mCursor.left + gi->bearingX;
            vr.top = mCursor.top + gi->bearingY;
            vr.right = vr.left + gi->width;
            vr.bottom = vr.top + gi->height;

            FloatRect tr = gi->uvRect;

            if (mRenderXform.clip (vr, tr))
                quad (vr, tr);

            mCursor.left += gi->bearingX + gi->advance;
        }

        void emitSpace (wchar_t ch)
        {
            GlyphInfo* gi = mFont->getGlyphInfo (ch);

            mCursor.left += gi->bearingX + gi->advance;
        }

    private:

        void quad (const FloatRect& vr, const FloatRect& tr)
        {
            vertex (vr.left, vr.top, tr.left, tr.top);
            vertex (vr.right, vr.top, tr.right, tr.top);
            vertex (vr.left, vr.bottom, tr.left, tr.bottom);
            vertex (vr.right, vr.top, tr.right, tr.top);
            vertex (vr.left, vr.bottom, tr.left, tr.bottom);
            vertex (vr.right, vr.bottom, tr.right, tr.bottom);
        }

        void vertex (float X, float Y, float U, float V)
        {
            FloatPoint pt = mRenderXform (FloatPoint (X, Y));

            mVertices->x = pt.left;
            mVertices->y = pt.top ;
            mVertices->z = mZ;
            mVertices->u = U;
            mVertices->v = V;
            mVertices->colour = mC;

            ++mVertices;
        }
    };
}

class MWGui::PageDisplay : public ISubWidgetText
{
    MYGUI_RTTI_DERIVED(PageDisplay)
protected:

    typedef TypesetBookImpl::Section Section;
    typedef TypesetBookImpl::Line    Line;
    typedef TypesetBookImpl::Run     Run;

    struct TextFormat : ISubWidget
    {
        typedef IFont* id;

        id mFont;
        int mCountVertex;
        ITexture* mTexture;
        RenderItem* mRenderItem;
        PageDisplay * mDisplay;

        TextFormat (IFont* Id, PageDisplay * Display) :
            mFont (Id),
            mTexture (NULL),
            mRenderItem (NULL),
            mDisplay (Display),
            mCountVertex (0)
        {
        }

        void createDrawItem (ILayerNode* node)
        {
            assert (mRenderItem == NULL);

            if (mTexture != NULL)
            {
                mRenderItem = node->addToRenderItem(mTexture, false, false);
                mRenderItem->addDrawItem(this, mCountVertex);
            }
        }

        void destroyDrawItem (ILayerNode* node)
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
        void createDrawItem(ITexture* _texture, ILayerNode* _node) {}
        void destroyDrawItem() {};
    };

public:

    typedef TypesetBookImpl::StyleImpl style;
    typedef std::map <TextFormat::id, TextFormat*> active_text_formats;

    int mViewTop;
    int mViewBottom;

    style* mFocusItem;
    bool mItemActive;
    MouseButton mLastDown;
    boost::function <void (intptr_t)> mLinkClicked;


    boost::shared_ptr <TypesetBookImpl> mBook;
    size_t mPage;

    ILayerNode* mNode;
    active_text_formats mActiveTextFormats;

    PageDisplay ()
    {
        mPage = -1;
    }

    void dirtyFocusItem ()
    {
        if (mFocusItem != 0)
        {
            IFont* Font = mBook->affectedFont (mFocusItem);

            active_text_formats::iterator i = mActiveTextFormats.find (Font);

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

    void onMouseMove (int _left, int _top)
    {
        if (!mBook)
            return;

        _left -= mCroppedParent->getAbsoluteLeft ();
        _top  -= mCroppedParent->getAbsoluteTop  ();

        style * Hit = mBook->hitTest (_left, mViewTop + _top);

        if (mLastDown == MouseButton::None)
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

    void onMouseButtonPressed (int _left, int _top, MouseButton _id)
    {
        if (!mBook)
            return;

        _left -= mCroppedParent->getAbsoluteLeft ();
        _top  -= mCroppedParent->getAbsoluteTop  ();

        if (mLastDown == MouseButton::None)
        {
            mFocusItem = mBook->hitTest (_left, mViewTop + _top);
            mItemActive = true;

            dirtyFocusItem ();

            mLastDown = _id;
        }
    }

    void onMouseButtonReleased(int _left, int _top, MouseButton _id)
    {
        if (!mBook)
            return;

        _left -= mCroppedParent->getAbsoluteLeft ();
        _top  -= mCroppedParent->getAbsoluteTop  ();

        if (mLastDown == _id)
        {
            style * mItem = mBook->hitTest (_left, mViewTop + _top);

            bool clicked = mFocusItem == mItem;

            mItemActive = false;

            dirtyFocusItem ();

            mLastDown = MouseButton::None;

            if (clicked && mLinkClicked && mItem && mItem->mInteractiveId != 0)
                mLinkClicked (mItem->mInteractiveId);
        }
    }

    void showPage (TypesetBook::ptr _Book, size_t newPage)
    {
        boost::shared_ptr <TypesetBookImpl> newBook = boost::dynamic_pointer_cast <TypesetBookImpl> (_Book);

        if (mBook != newBook)
        {
            mFocusItem = nullptr;
            mItemActive = 0;

            for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
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
                for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
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

    struct createActiveFormat
    {
        PageDisplay * this_;

        createActiveFormat (PageDisplay * this_) : this_ (this_) {}

        void operator () (Section const & Section, Line const & Line, Run const & Run) const
        {
            IFont* Font = Run.mStyle->mFont;

            active_text_formats::iterator j = this_->mActiveTextFormats.find (Font);

            if (j == this_->mActiveTextFormats.end ())
            {
                TextFormat * textFormat = new TextFormat (Font, this_);

                textFormat->mTexture = Font->getTextureFont ();

                j = this_->mActiveTextFormats.insert (std::make_pair (Font, textFormat)).first;
            }

            j->second->mCountVertex += Run.mPrintableChars * 6;
        }
    };

    void createActiveFormats (boost::shared_ptr <TypesetBookImpl> newBook)
    {
        newBook->visitRuns (0, 0x7FFFFFFF, createActiveFormat (this));

        if (mNode != NULL)
            for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
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
            mLastDown = MouseButton::None;
            mFocusItem = nullptr;
            mItemActive = 0;
        }

        if (nullptr != mNode)
        {
            for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate(i->second->mRenderItem);
        }
    }

    void createDrawItem(ITexture* _texture, ILayerNode* _node)
    {
        //test ();

        mNode = _node;

        for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->createDrawItem (_node);
    }

    struct renderRun
    {
        PageDisplay * this_;
        GlyphStream &glyphStream;

        renderRun (PageDisplay * this_, GlyphStream &glyphStream) :
            this_(this_), glyphStream (glyphStream)
        {
        }

        void operator () (Section const & Section, Line const & Line, Run const & Run) const
        {
            bool isActive = Run.mStyle->mInteractiveId && (Run.mStyle == this_->mFocusItem);

            Colour colour = isActive ? (this_->mItemActive ? Run.mStyle->mActiveColour: Run.mStyle->mHotColour) : Run.mStyle->mNormalColour;

            glyphStream.reset (Section.mRect.left + Line.mRect.left + Run.mLeft, Line.mRect.top, colour);

            utf8_stream stream (Run.mRange);

            while (!stream.eof ())
            {
                utf8_stream::unicode_char code_point = stream.consume ();

                if (!ucs_space (code_point))
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

        Vertex* vertices = textFormat.mRenderItem->getCurrentVertexBuffer();

        RenderXform renderXform (mCroppedParent, textFormat.mRenderItem->getRenderTarget()->getInfo());

        GlyphStream glyphStream (textFormat.mFont, mCoord.left, mCoord.top-mViewTop,
                                  -1 /*mNode->getNodeDepth()*/, vertices, renderXform);

        int visit_top    = (std::max) (mViewTop,    mViewTop + int (renderXform.clipTop   ));
        int visit_bottom = (std::min) (mViewBottom, mViewTop + int (renderXform.clipBottom));

        mBook->visitRuns (visit_top, visit_bottom, textFormat.mFont, renderRun (this, glyphStream));

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
            for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate (i->second->mRenderItem);

    }

    void destroyDrawItem()
    {
        for (active_text_formats::iterator i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->destroyDrawItem (mNode);

        mNode = NULL;
    }
};


class MWGui::BookPageImpl : public BookPage
{
MYGUI_RTTI_DERIVED(BookPage)
public:


    void showPage (TypesetBook::ptr Book, size_t Page)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
            pd->showPage (Book, Page);
        else
            throw std::runtime_error ("The main sub-widget for a BookPage must be a PageDisplay.");
    }

    void adviseLinkClicked (boost::function <void (interactive_id)> linkClicked)
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
            pd->mLinkClicked = boost::function <void (interactive_id)> ();
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

    void onMouseMove(int _left, int _top)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseMove (_left, _top);
        }
        else
            Widget::onMouseMove (_left, _top);
    }

    void onMouseButtonPressed (int _left, int _top, MouseButton _id)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseButtonPressed (_left, _top, _id);
        }
        else
            Widget::onMouseButtonPressed (_left, _top, _id);
    }

    void onMouseButtonReleased(int _left, int _top, MouseButton _id)
    {
        if (PageDisplay* pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseButtonReleased (_left, _top, _id);
        }
        else
            Widget::onMouseButtonReleased (_left, _top, _id);
    }
};

void BookPage::registerMyGUIComponents ()
{
    FactoryManager & factory = FactoryManager::getInstance();

    factory.registerFactory<BookPageImpl>("Widget");
    factory.registerFactory<PageDisplay>("BasisSkin");
}

static bool ucs_line_break (int code_point)
{
    return code_point == '\n';
}

static bool ucs_space (int code_point)
{
    switch (code_point)
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

static bool ucs_breaking_space (int code_point)
{
    switch (code_point)
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
