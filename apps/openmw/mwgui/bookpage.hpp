#ifndef MWGUI_BOOKPAGE_HPP
#define MWGUI_BOOKPAGE_HPP

#include "MyGUI_Colour.h"
#include "MyGUI_Widget.h"
#include "MyGUI_FontManager.h"

#include <functional>
#include <memory>
#include <stdint.h>

#include <components/settings/settings.hpp>
#include <components/widgets/widgets.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    /// A formatted and paginated document to be used with
    /// the book page widget.
    struct TypesetBook
    {
        typedef std::shared_ptr <TypesetBook> Ptr;
        typedef intptr_t InteractiveId;

        /// Returns the number of pages in the document.
        virtual size_t pageCount () const = 0;

        /// Return the area covered by the document. The first
        /// integer is the maximum with of any line. This is not
        /// the largest coordinate of the right edge of any line,
        /// it is the largest distance from the left edge to the
        /// right edge. The second integer is the height of all
        /// text combined prior to pagination.
        virtual std::pair <unsigned int, unsigned int> getSize () const = 0;

        virtual ~TypesetBook() = default;
    };

    struct GlyphInfo
    {
        char codePoint;
        float width;
        float height;
        float advance;
        float bearingX;
        float bearingY;
        bool charFound;
        MyGUI::FloatRect uvRect;

        GlyphInfo(MyGUI::IFont* font, MyGUI::Char ch)
        {
            static const int fontHeight = MWBase::Environment::get().getWindowManager()->getFontHeight();

            MyGUI::GlyphInfo* gi = font->getGlyphInfo(ch);
            if (gi)
            {
                const float scale = font->getDefaultHeight() / (float) fontHeight;

                codePoint = gi->codePoint;
                bearingX = (int) gi->bearingX / scale;
                bearingY = (int) gi->bearingY / scale;
                width = (int) gi->width / scale;
                height = (int) gi->height / scale;
                advance = (int) gi->advance / scale;
                uvRect = gi->uvRect;
                charFound = true;
            }
            else
            {
                codePoint = 0;
                bearingX = 0;
                bearingY = 0;
                width = 0;
                height = 0;
                advance = 0;
                charFound = false;
            }
        }
    };

    /// A factory class for creating a typeset book instance.
    struct BookTypesetter
    {
        typedef std::shared_ptr <BookTypesetter> Ptr;
        typedef TypesetBook::InteractiveId InteractiveId;
        typedef MyGUI::Colour Colour;
        typedef uint8_t const * Utf8Point;
        typedef std::pair <Utf8Point, Utf8Point> Utf8Span;

        virtual ~BookTypesetter() = default;

        enum Alignment {
            AlignLeft   = -1,
            AlignCenter = 0,
            AlignRight  = +1
        };

        /// Styles are used to control the character level formatting
        /// of text added to a typeset book. Their lifetime is equal
        /// to the lifetime of the book-typesetter instance that created
        /// them.
        struct Style;

        /// A factory function for creating the default implementation of a book typesetter
        static Ptr create (int pageWidth, int pageHeight);

        /// Create a simple text style consisting of a font and a text color.
        virtual Style* createStyle (const std::string& fontName, const Colour& colour, bool useBookFont=true) = 0;

        /// Create a hyper-link style with a user-defined identifier based on an
        /// existing style. The unique flag forces a new instance of this style
        /// to be created even if an existing instance is present.
        virtual Style* createHotStyle (Style * BaseStyle, const Colour& NormalColour, const Colour& HoverColour,
                                       const Colour& ActiveColour, InteractiveId Id, bool Unique = true) = 0;

        /// Insert a line break into the document. Newline characters in the input
        /// text have the same affect. The margin parameter adds additional space
        /// before the next line of text.
        virtual void lineBreak (float margin = 0) = 0;

        /// Insert a section  break into the document. This causes a new section
        /// to begin when additional text is inserted. Pagination attempts to keep
        /// sections together on a single page. The margin parameter adds additional space
        /// before the next line of text.
        virtual void sectionBreak (int margin = 0) = 0;

        /// Changes the alignment for the current section of text.
        virtual void setSectionAlignment (Alignment sectionAlignment) = 0;

        // Layout a block of text with the specified style into the document.
        virtual void write (Style * Style, Utf8Span Text) = 0;

        /// Adds a content block to the document without laying it out. An
        /// identifier is returned that can be used to refer to it. If select
        /// is true, the block is activated to be references by future writes.
        virtual intptr_t addContent (Utf8Span Text, bool Select = true) = 0;

        /// Select a previously created content block for future writes.
        virtual void selectContent (intptr_t contentHandle) = 0;

        /// Layout a span of the selected content block into the document
        /// using the specified style.
        virtual void write (Style * Style, size_t Begin, size_t End) = 0;

        /// Finalize the document layout, and return a pointer to it.
        virtual TypesetBook::Ptr complete () = 0;
    };

    /// An interface to the BookPage widget.
    class BookPage : public MyGUI::Widget
    {
    MYGUI_RTTI_DERIVED(BookPage)
    public:

        typedef TypesetBook::InteractiveId InteractiveId;
        typedef std::function <void (InteractiveId)> ClickCallback;

        /// Make the widget display the specified page from the specified book.
        virtual void showPage (TypesetBook::Ptr Book, size_t Page) = 0;

        /// Set the callback for a clicking a hyper-link in the document.
        virtual void adviseLinkClicked (ClickCallback callback) = 0;

        /// Clear the hyper-link click callback.
        virtual void unadviseLinkClicked () = 0;

        /// Register the widget and associated sub-widget with MyGUI. Should be
        /// called once near the beginning of the program.
        static void registerMyGUIComponents ();
    };
}

#endif // MWGUI_BOOKPAGE_HPP
