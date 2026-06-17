#include "journalbooks.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/misc/utf8stream.hpp>
#include <components/settings/values.hpp>

#include "textcolours.hpp"

namespace
{
    struct AddContent
    {
        std::shared_ptr<MWGui::BookTypesetter> mTypesetter;
        MWGui::BookTypesetter::Style* mBodyStyle;

        explicit AddContent(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* bodyStyle)
            : mTypesetter(std::move(typesetter))
            , mBodyStyle(bodyStyle)
        {
        }
    };

    struct AddSpan : AddContent
    {
        explicit AddSpan(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* bodyStyle)
            : AddContent(std::move(typesetter), bodyStyle)
        {
        }

        void operator()(const MWDialogue::Topic* topic, size_t begin, size_t end)
        {
            MWGui::BookTypesetter::Style* style = mBodyStyle;

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            if (topic)
                style = mTypesetter->createHotStyle(mBodyStyle, textColours.journalLink, textColours.journalLinkOver,
                    textColours.journalLinkPressed, MWGui::TypesetBook::InteractiveId(topic));

            mTypesetter->write(style, begin, end);
        }
    };

    struct AddEntry
    {
        std::shared_ptr<MWGui::BookTypesetter> mTypesetter;
        MWGui::BookTypesetter::Style* mBodyStyle;

        AddEntry(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* bodyStyle)
            : mTypesetter(std::move(typesetter))
            , mBodyStyle(bodyStyle)
        {
        }

        void operator()(MWGui::JournalViewModel::Entry const& entry)
        {
            mTypesetter->addContent(entry.body());

            entry.visitSpans(AddSpan(mTypesetter, mBodyStyle));
        }
    };

    struct AddJournalEntry : AddEntry
    {
        bool mAddHeader;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        explicit AddJournalEntry(std::shared_ptr<MWGui::BookTypesetter> typesetter,
            MWGui::BookTypesetter::Style* bodyStyle, MWGui::BookTypesetter::Style* headerStyle, bool addHeader)
            : AddEntry(std::move(typesetter), bodyStyle)
            , mAddHeader(addHeader)
            , mHeaderStyle(headerStyle)
        {
        }

        void operator()(MWGui::JournalViewModel::JournalEntry const& entry)
        {
            if (mAddHeader)
            {
                mTypesetter->write(mHeaderStyle, entry.timestamp());
                mTypesetter->lineBreak();
            }

            AddEntry::operator()(entry);

            mTypesetter->sectionBreak(30);
        }
    };

    struct AddTopicEntry : AddEntry
    {
        const MWGui::TypesetBook::Content* mContentId;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        explicit AddTopicEntry(std::shared_ptr<MWGui::BookTypesetter> typesetter,
            MWGui::BookTypesetter::Style* bodyStyle, MWGui::BookTypesetter::Style* headerStyle,
            const MWGui::TypesetBook::Content* contentId)
            : AddEntry(std::move(typesetter), bodyStyle)
            , mContentId(contentId)
            , mHeaderStyle(headerStyle)
        {
        }

        void operator()(MWGui::JournalViewModel::TopicEntry const& entry)
        {
            mTypesetter->write(mBodyStyle, entry.source());
            mTypesetter->write(mBodyStyle, 0, 3); // begin

            AddEntry::operator()(entry);

            mTypesetter->selectContent(mContentId);
            mTypesetter->write(mBodyStyle, 2, 3); // end quote

            mTypesetter->sectionBreak(30);
        }
    };

    struct AddTopicName : AddContent
    {
        AddTopicName(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* style)
            : AddContent(std::move(typesetter), style)
        {
        }

        void operator()(std::string_view topicName)
        {
            mTypesetter->write(mBodyStyle, topicName);
            mTypesetter->sectionBreak();
        }
    };

    struct AddQuestName : AddContent
    {
        AddQuestName(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* style)
            : AddContent(std::move(typesetter), style)
        {
        }

        void operator()(std::string_view topicName)
        {
            mTypesetter->write(mBodyStyle, topicName);
            mTypesetter->sectionBreak();
        }
    };
}

namespace MWGui
{

    int getCyrillicIndexPageCount()
    {
        // For small font size split alphabet to two columns (2x15 characers), for big font size split it to three
        // colums (3x10 characters).
        return Settings::gui().mFontSize < 18 ? 2 : 3;
    }

    JournalBooks::JournalBooks(std::shared_ptr<JournalViewModel> model, ToUTF8::FromType encoding)
        : mModel(std::move(model))
        , mEncoding(encoding)
        , mIndexPagesCount(0)
    {
    }

    std::shared_ptr<TypesetBook> JournalBooks::createEmptyJournalBook()
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        typesetter->write(header, "You have no journal entries!");
        typesetter->lineBreak();
        typesetter->write(body, "You should have gone though the starting quest and got an initial quest.");

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createJournalBook()
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        mModel->visitJournalEntries({}, AddJournalEntry(typesetter, body, header, true));

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createTopicBook(const MWDialogue::Topic& topic)
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        mModel->visitTopicName(topic, AddTopicName(typesetter, header));

        const TypesetBook::Content* contentId = typesetter->addContent(", \"");

        mModel->visitTopicEntries(topic, AddTopicEntry(typesetter, body, header, contentId));

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createQuestBook(std::string_view questName)
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        AddQuestName addName(typesetter, header);
        addName(questName);

        mModel->visitJournalEntries(questName, AddJournalEntry(typesetter, body, header, false));

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createTopicIndexBook()
    {
        bool isRussian = (mEncoding == ToUTF8::WINDOWS_1251);

        std::shared_ptr<BookTypesetter> typesetter
            = isRussian ? createCyrillicJournalIndex() : createLatinJournalIndex();

        return typesetter->complete();
    }

    std::shared_ptr<BookTypesetter> JournalBooks::createLatinJournalIndex()
    {
        std::shared_ptr<BookTypesetter> typesetter = BookTypesetter::create(92, 260);

        typesetter->setSectionAlignment(BookTypesetter::AlignCenter);

        // Latin journal index always has two columns for now.
        mIndexPagesCount = 2;

        char ch = 'A';
        std::string buffer;

        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);
        for (int i = 0; i < 26; ++i)
        {
            buffer = "( ";
            buffer += ch;
            buffer += " )";

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            BookTypesetter::Style* style = typesetter->createHotStyle(body, textColours.journalTopic,
                textColours.journalTopicOver, textColours.journalTopicPressed, Utf8Stream::UnicodeChar(ch));

            if (i == 13)
                typesetter->sectionBreak();

            typesetter->write(style, buffer);
            typesetter->lineBreak();

            ch++;
        }

        return typesetter;
    }

    std::shared_ptr<BookTypesetter> JournalBooks::createCyrillicJournalIndex()
    {
        std::shared_ptr<BookTypesetter> typesetter = BookTypesetter::create(92, 260);

        typesetter->setSectionAlignment(BookTypesetter::AlignCenter);

        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        // for small font size split alphabet to two columns (2x15 characers), for big font size split it to three
        // colums (3x10 characters).
        mIndexPagesCount = getCyrillicIndexPageCount();
        int sectionBreak = 30 / mIndexPagesCount;

        unsigned char ch[3] = { 0xd0, 0x90, 0x00 }; // CYRILLIC CAPITAL A is a 0xd090 in UTF-8

        std::string buffer;

        for (int i = 0; i < 32; ++i)
        {
            buffer = "( ";
            buffer += ch[0];
            buffer += ch[1];
            buffer += " )";

            Utf8Stream stream(ch, ch + 2);
            Utf8Stream::UnicodeChar first = stream.peek();

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            BookTypesetter::Style* style = typesetter->createHotStyle(
                body, textColours.journalTopic, textColours.journalTopicOver, textColours.journalTopicPressed, first);

            ch[1]++;

            // Words can not be started with these characters
            if (i == 26 || i == 28)
                continue;

            if (i % sectionBreak == 0)
                typesetter->sectionBreak();

            typesetter->write(style, buffer);
            typesetter->lineBreak();
        }

        return typesetter;
    }

    std::shared_ptr<BookTypesetter> JournalBooks::createTypesetter()
    {
        // TODO: determine page size from layout...
        return BookTypesetter::create(240, 320);
    }

}
