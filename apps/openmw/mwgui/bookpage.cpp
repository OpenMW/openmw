#include "bookpage.hpp"

#include "MyGUI_FontManager.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_TextureUtility.h"
#include "MyGUI_FactoryManager.h"

#include <platform/stdint.h>

#include <components/misc/utf8stream.hpp>

namespace MWGui
{
    struct TypesetBook;
    struct PageDisplay;
    struct BookPage;
}

using namespace MyGUI;
using namespace MWGui;

static bool ucs_space (int code_point);
static bool ucs_line_break (int code_point);
static bool ucs_breaking_space (int code_point);

struct IBookTypesetter::IStyle { virtual ~IStyle () {} };

struct MWGui::TypesetBook : ITypesetBook
{
    typedef std::vector <uint8_t> content;
    typedef std::list <content> contents;
    typedef utf8_stream::point utf8_point;
    typedef std::pair <utf8_point, utf8_point> range;

    struct style : IBookTypesetter::IStyle
    {
        IFont*         Font;
        Colour         HotColour;
        Colour         ActiveColour;
        Colour         NormalColour;
        interactive_id InteractiveId;

        bool match (IFont* tstFont, Colour tstHotColour, Colour tstActiveColour, Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return (Font == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool match (char const * tstFont, Colour tstHotColour, Colour tstActiveColour, Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return (Font->getResourceName ()   == tstFont) &&
                   partal_match (tstHotColour, tstActiveColour, tstNormalColour, tstInteractiveId);
        }

        bool partal_match (Colour tstHotColour, Colour tstActiveColour, Colour tstNormalColour, intptr_t tstInteractiveId)
        {
            return
                (HotColour                  == tstHotColour     ) &&
                (ActiveColour               == tstActiveColour  ) &&
                (NormalColour               == tstNormalColour  ) &&
                (InteractiveId              == tstInteractiveId ) ;
        }
    };

    typedef std::list <style> styles;

    struct run
    {
        style*      Style;
        range       Range;
        int         Left, Right;
        int         PrintableChars;
    };

    typedef std::vector <run> runs;

    struct line
    {
        runs Runs;
        IntRect Rect;
    };

    typedef std::vector <line> lines;

    struct section
    {
        lines Lines;
        IntRect Rect;
    };

    typedef std::vector <section> sections;

    typedef std::pair <int, int> page;
    typedef std::vector <page> pages;

    pages Pages;
    sections Sections;
    contents Contents;
    styles Styles;
    IntRect Rect;

    virtual ~TypesetBook () {}

    range addContent (IBookTypesetter::utf8_span Text)
    {
        auto i = Contents.insert (Contents.end (), content (Text.first, Text.second));

        if (i->size () == 0)
            return range (utf8_point (NULL), utf8_point (NULL));

        utf8_point begin = &i->front ();
        utf8_point end   = &i->front () + i->size ();

        return range (begin, end);
    }

    int pageCount () const { return Pages.size (); }

    std::pair <int, int> getSize () const
    {
        return std::make_pair (Rect.width (), Rect.height ());
    }

    template <typename visitor>
    void visit_runs (int top, int bottom, IFont* Font, visitor const & Visitor) const
    {
        for (auto i = Sections.begin (); i != Sections.end (); ++i)
        {
            if (top >= Rect.bottom || bottom <= i->Rect.top)
                continue;

            for (auto j = i->Lines.begin (); j != i->Lines.end (); ++j)
            {
                if (top >= j->Rect.bottom || bottom <= j->Rect.top)
                    continue;

                for (auto k = j->Runs.begin (); k != j->Runs.end (); ++k)
                    if (!Font || k->Style->Font == Font)
                        Visitor (*i, *j, *k);
            }
        }
    }

    template <typename visitor>
    void visit_runs (int top, int bottom, visitor const & Visitor) const
    {
        visit_runs (top, bottom, NULL, Visitor);
    }

    style * hitTest (int left, int top)
    {
        for (auto i = Sections.begin (); i != Sections.end (); ++i)
        {
            if (top < i->Rect.top || top >= i->Rect.bottom)
                continue;

            auto left1 = left - i->Rect.left;

            for (auto j = i->Lines.begin (); j != i->Lines.end (); ++j)
            {
                if (top < j->Rect.top || top >= j->Rect.bottom)
                    continue;

                auto left2 = left1 - j->Rect.left;

                for (auto k = j->Runs.begin (); k != j->Runs.end (); ++k)
                {
                    if (left2 < k->Left || left2 >= k->Right)
                        continue;

                    return k->Style;
                }
            }
        }

        return nullptr;
    }

    IFont* affectedFont (style* Style)
    {
        for (auto i = Styles.begin (); i != Styles.end (); ++i)
            if (&*i == Style)
                return i->Font;
        return NULL;
    }

    struct Typesetter;
};

struct TypesetBook::Typesetter : IBookTypesetter
{
    typedef TypesetBook book;
    typedef std::shared_ptr <book> book_ptr;

    int mPageWidth;
    int mPageHeight;

    book_ptr Book;
    section * Section;
    line * Line;
    run * Run;

    std::vector <alignment> mSectionAlignment;

    book::content const * mCurrentContent;
    alignment mCurrentAlignment;

    Typesetter (size_t Width, size_t Height) :
        mPageWidth (Width), mPageHeight(Height),
        Section (NULL), Line (NULL), Run (NULL),
        mCurrentAlignment (alignLeft),
        mCurrentContent (NULL)
    {
        Book = std::make_shared <book> ();
    }

    virtual ~Typesetter ()
    {
    }

    IStyle * createStyle (char const * FontName, Colour FontColour)
    {
        for (auto i = Book->Styles.begin (); i != Book->Styles.end (); ++i)
            if (i->match (FontName, FontColour, FontColour, FontColour, 0))
                return &*i;

        auto & Style = *Book->Styles.insert (Book->Styles.end (), style ());

        Style.Font = FontManager::getInstance().getByName(FontName);
        Style.HotColour = FontColour;
        Style.ActiveColour = FontColour;
        Style.NormalColour = FontColour;
        Style.InteractiveId = 0;
                
        return &Style;
    }

    IStyle* createHotStyle (IStyle * _BaseStyle, coulour NormalColour, coulour HoverColour, coulour ActiveColour, interactive_id Id, bool Unique)
    {
        auto BaseStyle = dynamic_cast <style*> (_BaseStyle);

        if (!Unique)
            for (auto i = Book->Styles.begin (); i != Book->Styles.end (); ++i)
                if (i->match (BaseStyle->Font, HoverColour, ActiveColour, NormalColour, Id))
                    return &*i;

        auto & Style = *Book->Styles.insert (Book->Styles.end (), style ());

        Style.Font = BaseStyle->Font;
        Style.HotColour = HoverColour;
        Style.ActiveColour = ActiveColour;
        Style.NormalColour = NormalColour;
        Style.InteractiveId = Id;

        return &Style;
    }

    void write (IStyle * _Style, utf8_span Text)
    {
        auto text = Book->addContent (Text);

        write_impl (dynamic_cast <style*> (_Style), text.first, text.second);
    }

    intptr_t add_content (utf8_span Text, bool Select)
    {
        auto i = Book->Contents.insert (Book->Contents.end (), content (Text.first, Text.second));

        if (Select)
            mCurrentContent = &(*i);

        return reinterpret_cast <intptr_t> (&(*i));
    }

    void select_content (intptr_t contentHandle)
    {
        mCurrentContent = reinterpret_cast <content const *> (contentHandle);
    }

    void write (IStyle * Style, size_t Begin, size_t End)
    {
        assert (mCurrentContent != NULL);
        assert (End <= mCurrentContent->size ());
        assert (Begin <= mCurrentContent->size ());

        utf8_point begin = &mCurrentContent->front () + Begin;
        utf8_point end   = &mCurrentContent->front () + End  ;

        write_impl (dynamic_cast <style*> (Style), begin, end);
    }
    
    void lineBreak (float margin)
    {
        assert (margin == 0); //TODO: figure out proper behavior here...

        Run = NULL;
        Line = NULL;
    }
    
    void sectionBreak (float margin)
    {
        if (Book->Sections.size () > 0)
        {
            Run = NULL;
            Line = NULL;
            Section = NULL;

            if (Book->Rect.bottom < (Book->Sections.back ().Rect.bottom + margin))
                Book->Rect.bottom = (Book->Sections.back ().Rect.bottom + margin);
        }
    }

    void setSectionAlignment (alignment sectionAlignment)
    {
        if (Section != NULL)
            mSectionAlignment.back () = sectionAlignment;
        mCurrentAlignment = sectionAlignment;
    }

    ITypesetBook::ptr complete ()
    {
        int curPageStart = 0;
        int curPageStop  = 0;

        auto sa = mSectionAlignment.begin ();
        for (auto i = Book->Sections.begin (); i != Book->Sections.end (); ++i, ++sa)
        {
            // apply alignment to individual lines...
            for (auto j = i->Lines.begin (); j != i->Lines.end (); ++j)
            {
                auto width = j->Rect.width ();
                auto excess = mPageWidth - width;

                switch (*sa)
                {
                default:
                case alignLeft:   j->Rect.left = 0;        break;
                case alignCenter: j->Rect.left = excess/2; break;
                case alignRight:  j->Rect.left = excess;   break;
                }

                j->Rect.right = j->Rect.left + width;
            }

            if (curPageStop == curPageStart)
            {
                curPageStart = i->Rect.top;
                curPageStop  = i->Rect.top;
            }

            auto spaceLeft = mPageHeight - (curPageStop - curPageStart);
            auto sectionHeight = i->Rect.height ();

            if (sectionHeight <= mPageHeight)
            {
                if (sectionHeight > spaceLeft)
                {
                    assert (curPageStart != curPageStop);

                    Book->Pages.push_back (page (curPageStart, curPageStop));

                    curPageStart = i->Rect.top;
                    curPageStop = i->Rect.bottom;
                }
                else
                    curPageStop = i->Rect.bottom;
            }
            else
            {
                //split section
            }
        }

        if (curPageStart != curPageStop)
            Book->Pages.push_back (page (curPageStart, curPageStop));

        return Book;
    }

    void write_impl (style * Style, utf8_stream::point _begin, utf8_stream::point _end)
    {
        auto line_height = Style->Font->getDefaultHeight ();

        utf8_stream stream (_begin, _end);

        while (!stream.eof ())
        {
            if (ucs_line_break (stream.peek ()))
            {
                stream.consume ();
                Line = NULL, Run = NULL;
                continue;
            }

            int word_width = 0;
            int word_height = 0;
            int space_width = 0;
            int character_count = 0;

            auto lead = stream.current ();

            while (!stream.eof () && !ucs_line_break (stream.peek ()) && ucs_breaking_space (stream.peek ()))
            {
                auto gi = Style->Font->getGlyphInfo (stream.peek ());
                space_width += gi->advance;
                stream.consume ();
            }

            auto origin = stream.current ();

            while (!stream.eof () && !ucs_line_break (stream.peek ()) && !ucs_breaking_space (stream.peek ()))
            {
                auto gi = Style->Font->getGlyphInfo (stream.peek ());
                word_width += gi->advance + gi->bearingX;
                word_height = line_height;
                ++character_count;
                stream.consume ();
            }

            auto extent = stream.current ();

            if (lead == extent)
                break;

            int left = Line ? Line->Rect.right : 0;

            if (left + space_width + word_width > mPageWidth)
            {
                Line = NULL, Run = NULL;

                append_run (Style, origin, extent, extent - origin, word_width, Book->Rect.bottom + word_height);
            }
            else
            {
                int top = Line ? Line->Rect.top : Book->Rect.bottom;

                append_run (Style, lead, extent, extent - origin, left + space_width + word_width, top + word_height);
            }
        }
    }

    void append_run (style * Style, utf8_stream::point begin, utf8_stream::point end, int pc, int right, int bottom)
    {
        if (Section == NULL)
        {
            Book->Sections.push_back (section ());
            Section = &Book->Sections.back ();
            Section->Rect = IntRect (0, Book->Rect.bottom, 0, Book->Rect.bottom);
            mSectionAlignment.push_back (mCurrentAlignment);
        }

        if (Line == NULL)
        {
            Section->Lines.push_back (line ());
            Line = &Section->Lines.back ();
            Line->Rect = IntRect (0, Section->Rect.bottom, 0, Book->Rect.bottom);
        }

        if (Book->Rect.right < right)
            Book->Rect.right = right;

        if (Book->Rect.bottom < bottom)
            Book->Rect.bottom = bottom;

        if (Section->Rect.right < right)
            Section->Rect.right = right;

        if (Section->Rect.bottom < bottom)
            Section->Rect.bottom = bottom;

        if (Line->Rect.right < right)
            Line->Rect.right = right;

        if (Line->Rect.bottom < bottom)
            Line->Rect.bottom = bottom;

        if (Run == NULL || Run->Style != Style || Run->Range.second != begin)
        {
            auto left = Run ? Run->Right : Line->Rect.left;

            Line->Runs.push_back (run ());
            Run = &Line->Runs.back ();
            Run->Style = Style;
            Run->Left = left;
            Run->Right = right;
            Run->Range.first = begin;
            Run->Range.second = end;
            Run->PrintableChars = pc;
          //Run->Locale = Locale;
        }
        else
        {
            Run->Right = right;
            Run->Range.second = end;
            Run->PrintableChars += pc;
        }
    }
};

IBookTypesetter::ptr IBookTypesetter::create (int pageWidth, int pageHeight)
{
    return std::make_shared <TypesetBook::Typesetter> (pageWidth, pageHeight);
}

namespace
{
    struct render_xform
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

        render_xform (ICroppedRectangle* croppedParent, RenderTargetInfo const & renderTargetInfo)
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

    struct glyph_stream
    {
        float Z;
        uint32_t C;
        IFont* mFont;
        FloatPoint mOrigin;
        FloatPoint mCursor;
        Vertex* mVertices;
        render_xform mRenderXform;
        MyGUI::VertexColourType mVertexColourType;

        glyph_stream (IFont* font, float left, float top, float Z,
                      Vertex* vertices, render_xform const & renderXform) :
            Z(Z), mOrigin (left, top),
            mFont (font), mVertices (vertices),
            mRenderXform (renderXform)
        {
            mVertexColourType = RenderManager::getInstance().getVertexFormat();
        }

        ~glyph_stream ()
        {
        }

        Vertex* end () const { return mVertices; }

        void reset (float left, float top, Colour colour)
        {
            C = texture_utility::toColourARGB(colour) | 0xFF000000;
            texture_utility::convertColour(C, mVertexColourType);

            mCursor.left = mOrigin.left + left;
            mCursor.top = mOrigin.top + top;
        }

        void emit_glyph (wchar_t ch)
        {
            auto gi = mFont->getGlyphInfo (ch);

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
        
        void emit_space (wchar_t ch)
        {
            auto gi = mFont->getGlyphInfo (ch);

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
            auto pt = mRenderXform (FloatPoint (X, Y));

            mVertices->x = pt.left;
            mVertices->y = pt.top ;
            mVertices->z = Z;
            mVertices->u = U;
            mVertices->v = V;
            mVertices->colour = C;

            ++mVertices;
        }
    };
}

class MWGui::PageDisplay : public ISubWidgetText
{
    MYGUI_RTTI_DERIVED(PageDisplay)
protected:

    typedef TypesetBook::section section;
    typedef TypesetBook::line    line;
    typedef TypesetBook::run     run;

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

    typedef TypesetBook::style style;

    int view_top;
    int view_bottom;

    style* mFocusItem;
    bool mItemActive;
    std::function <void (intptr_t)> mLinkClicked;


    std::shared_ptr <TypesetBook> mBook;
    size_t mPage;

    ILayerNode* mNode;
    std::map <TextFormat::id, TextFormat*> mActiveTextFormats;

    PageDisplay ()
    {
        mPage = -1;
    }

    MouseButton mLastDown;

    void dirtyFocusItem ()
    {
        if (mFocusItem != 0)
        {
            auto Font = mBook->affectedFont (mFocusItem);

            auto i = mActiveTextFormats.find (Font);

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

        auto Hit = mBook->hitTest (_left, view_top + _top);

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
            auto newItemActive = Hit == mFocusItem;

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
            mFocusItem = mBook->hitTest (_left, view_top + _top);
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
            auto mItem = mBook->hitTest (_left, view_top + _top);

            auto clicked = mFocusItem == mItem;

            mItemActive = false;

            dirtyFocusItem ();

            mLastDown = MouseButton::None;

            if (clicked && mLinkClicked && mItem && mItem->InteractiveId != 0)
                mLinkClicked (mItem->InteractiveId);
        }
    }

    void showPage (ITypesetBook::ptr _Book, size_t newPage)
    {
        auto newBook = std::dynamic_pointer_cast <TypesetBook> (_Book);

        if (mBook != newBook)
        {
            mFocusItem = nullptr;
            mItemActive = 0;

            for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
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

                if (newPage < mBook->Pages.size ())
                {
                    view_top = mBook->Pages [newPage].first;
                    view_bottom = mBook->Pages [newPage].second;
                }
                else
                {
                    view_top = 0;
                    view_bottom = 0;
                }
            }
            else
            {
                mBook.reset ();
                mPage = -1;
                view_top = 0;
                view_bottom = 0;
            }
        }
        else
        if (mBook && mPage != newPage)
        {
            if (mNode != NULL)
                for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                    mNode->outOfDate(i->second->mRenderItem);

            mPage = newPage;

            if (newPage < mBook->Pages.size ())
            {
                view_top = mBook->Pages [newPage].first;
                view_bottom = mBook->Pages [newPage].second;
            }
            else
            {
                view_top = 0;
                view_bottom = 0;
            }
        }
    }

    void createActiveFormats (std::shared_ptr <TypesetBook> newBook)
    {
        newBook->visit_runs (0, 0x7FFFFFFF, [this] (section const & Section, line const & Line, run const & Run) {

            auto Font = Run.Style->Font;

            auto j = mActiveTextFormats.find (Font);

            if (j == mActiveTextFormats.end ())
            {
                auto textFormat = new TextFormat (Font, this);

                textFormat->mTexture = Font->getTextureFont ();

                j = mActiveTextFormats.insert (std::make_pair (Font, textFormat)).first;
            }

            j->second->mCountVertex += Run.PrintableChars * 6;

        });

        if (mNode != NULL)
            for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
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
            for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate(i->second->mRenderItem);
        }
    }

    void createDrawItem(ITexture* _texture, ILayerNode* _node)
    {
        //test ();

        mNode = _node;
            
        for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->createDrawItem (_node);
    }

    /*
        queue up rendering operations for this text format
    */
    void doRender(TextFormat & textFormat)
    {
        if (!mVisible)
            return;

        auto vertices = textFormat.mRenderItem->getCurrentVertexBuffer();

        render_xform renderXform (mCroppedParent, textFormat.mRenderItem->getRenderTarget()->getInfo());

        glyph_stream glyphStream (textFormat.mFont, mCoord.left, mCoord.top-view_top,
                                  -1 /*mNode->getNodeDepth()*/, vertices, renderXform);

        auto visit_top    = (std::max) (view_top,    view_top + int (renderXform.clipTop   ));
        auto visit_bottom = (std::min) (view_bottom, view_top + int (renderXform.clipBottom));

        mBook->visit_runs (visit_top, visit_bottom, textFormat.mFont,
            [this, &glyphStream] (section const & Section, line const & Line, run const & Run)
            {
                bool isActive = Run.Style->InteractiveId && (Run.Style == mFocusItem);

                Colour colour = isActive ? (mItemActive ? Run.Style->ActiveColour: Run.Style->HotColour) : Run.Style->NormalColour;

                glyphStream.reset (Section.Rect.left + Line.Rect.left + Run.Left, Line.Rect.top, colour);

                utf8_stream stream (Run.Range);

                while (!stream.eof ())
                {
                    auto code_point = stream.consume ();

                    if (!ucs_space (code_point))
                        glyphStream.emit_glyph (code_point);
                    else
                        glyphStream.emit_space (code_point);
                }
            }
       );

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
            for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
                mNode->outOfDate (i->second->mRenderItem);

    }

    void destroyDrawItem()
    {
        for (auto i = mActiveTextFormats.begin (); i != mActiveTextFormats.end (); ++i)
            i->second->destroyDrawItem (mNode);

        mNode = NULL;
    }
};


class MWGui::BookPage : public IBookPage
{
MYGUI_RTTI_DERIVED(BookPage)
public:


    void showPage (ITypesetBook::ptr Book, size_t Page)
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
            pd->showPage (Book, Page);
        else
            throw std::runtime_error ("The main sub-widget for a BookPage must be a PageDisplay.");
    }

    void adviseLinkClicked (std::function <void (interactive_id)> linkClicked)
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->mLinkClicked = linkClicked;
        }
    }

    void unadviseLinkClicked ()
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->mLinkClicked = std::function <void (interactive_id)> ();
        }
    }

protected:
    void onMouseLostFocus(Widget* _new)
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseLostFocus ();
        }
        else
            Widget::onMouseLostFocus (_new);
    }

    void onMouseMove(int _left, int _top)
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseMove (_left, _top);
        }
        else
            Widget::onMouseMove (_left, _top);
    }

    void onMouseButtonPressed (int _left, int _top, MouseButton _id)
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseButtonPressed (_left, _top, _id);
        }
        else
            Widget::onMouseButtonPressed (_left, _top, _id);
    }

    void onMouseButtonReleased(int _left, int _top, MouseButton _id)
    {
        if (auto pd = dynamic_cast <PageDisplay*> (getSubWidgetText ()))
        {
            pd->onMouseButtonReleased (_left, _top, _id);
        }
        else
            Widget::onMouseButtonReleased (_left, _top, _id);
    }
};

void IBookPage::registerMyGUIComponents ()
{
    auto & factory = FactoryManager::getInstance();

    factory.registerFactory<BookPage>("Widget");
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
